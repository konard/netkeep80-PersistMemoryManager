# PersistMemoryManager Architecture

## Overview

[PersistMemoryManager](../include/pmm/persist_memory_manager.h#pmm-persistmemorymanager) is a header-only C++20 library for persistent address space management.
All metadata is stored inside the managed memory region, which allows saving and restoring
a memory image from a file or shared memory. Interaction with data is done through
persistent typed pointers `pptr<T, ManagerT>`.

The library is fully static: there are no instances, and all API is accessed through static
methods on the manager type. Multiple independent managers with the same configuration can
coexist through the `InstanceId` template parameter (multiton pattern).

The canonical high-level model of PMM as a linear persistent address space plus an intrusive
AVL-forest is documented in [pmm_avl_forest.md](pmm_avl_forest.md). The canonical semantics
of [BlockHeader](../include/pmm/block_header.h#pmm-blockheader) fields (the only physical block-header layout, which embeds the AVL slot as its prefix) is documented in [block_and_treenode_semantics.md](block_and_treenode_semantics.md).
The frozen invariant set with traceability to code and tests is in [core_invariants.md](core_invariants.md).
This document focuses on the low-level layout, invariants, and algorithms.

---

## Architecture layers

```
┌───────────────────────────────────────────────────────────────────────┐
│  PersistMemoryManager<ConfigT, InstanceId>                            │
│  (static public API: create / load / destroy)                         │
│  allocate_typed / deallocate_typed (pptr<T>)                          │
│  for_each_block / for_each_free_block                                 │
│  pstringview<ManagerT> / pmap<_K,_V,ManagerT>                        │
├───────────────────────────────────────────────────────────────────────┤
│  avl_tree_mixin.h (detail::avl_*)                                     │
│  (shared AVL: rotate, rebalance, insert, remove, find, iterators)    │
│  used by pmap, pstringview, AvlFreeTree (via BlockPPtr)              │
├───────────────────────────────────────────────────────────────────────┤
│  AllocatorPolicy<FreeBlockTreeT, AddressTraitsT>                      │
│  (best-fit via AVL tree, splitting, coalescing,                       │
│   repair_linked_list, rebuild_free_tree,                              │
│   recompute_counters)                                                 │
├───────────────────────────────────────────────────────────────────────┤
│  BlockState machine (block_state.h)                                   │
│  Allocator / free-tree FSM: Free → Allocated → Free                   │
│  (not a general forest-node lifecycle; pmap/pstringview do not use it)│
├───────────────────────────────────────────────────────────────────────┤
│  Block<AddressTraitsT> raw memory layout                              │
│  (BlockHeader<A> direct fields, granule indices)                   │
├───────────────────────────────────────────────────────────────────────┤
│  StorageBackend: HeapStorage / MMapStorage / StaticStorage            │
│  LockPolicy: NoLock / SharedMutexLock                                 │
└───────────────────────────────────────────────────────────────────────┘
```

---

## Managed memory region structure

```
[Block<A>_0][[ManagerHeader]]              ← granules 0–5 (2 + 4 granules)
[Block<A>_1][user_data_1...][padding]
...
[Block<A>_N][free space....]
```

[ManagerHeader](../include/pmm/types.h#pmm-detail-managerheader) is stored as the user data of `Block<A>_0` (granule 0, byte offset 32).
The memory layout is homogeneous: every region is a block. `Block<A>_0` has
`root_offset=0` (its own index) and `weight=kManagerHeaderGranules`.

User blocks start at granule 6 (byte offset 96 from the buffer start) for
`DefaultAddressTraits`. The exact layout depends on `address_traits::granule_size`.

---

## ManagerHeader

Located inside `Block<A>_0` at byte offset `sizeof(Block<AddressTraitsT>)` from the
buffer start. Contains:

| Field | Type | Description |
|-------|------|-------------|
| `magic` | `uint64_t` | Magic number `"PMM_V098"` for validation |
| `total_size` | `uint64_t` | Total managed region size in bytes |
| `used_size` | `uint32_t` | Used size in granules |
| `block_count` | `uint32_t` | Total block count |
| `free_count` | `uint32_t` | Free block count |
| `alloc_count` | `uint32_t` | Allocated block count |
| `first_block_offset` | `uint32_t` | Granule index of the first block |
| `last_block_offset` | `uint32_t` | Granule index of the last block |
| `free_tree_root` | `uint32_t` | Root of the AVL free block tree (granule index) |
| `owns_memory` | `bool` | Runtime-only: true if manager owns the buffer |
| `image_version` | `uint8_t` | Persistent image layout version |
| `granule_size` | `uint16_t` | Granule size at creation time; checked on `load()` |
| `prev_total_size` | `uint64_t` | Runtime-only: previous buffer size after `expand()` |
| `crc32` | `uint32_t` | CRC32 checksum used by file save/load helpers |
| `root_offset` | `uint32_t` | Forest registry root granule index |

Version policy (`detail::kCurrentImageVersion == 2`, no migration by design):

- `image_version == 2`: current layout; `load()` / `verify()` accept it directly.
- `image_version == 0` or `image_version == 1`: unsupported legacy image; `load()` /
  `verify()` reject it with `PmmError::UnsupportedImageVersion` and record
  `HeaderCorruption` / `Aborted`.
- Any other value: unsupported format; same rejection behaviour.

This is a deliberate, breaking change introduced by the issue #367 refactor. There is
no migration path from previous image formats — old images must be recreated.

---

## Block layout: `BlockHeader<AddressTraitsT>`

`BlockHeader<AT>` is the **single physical block-header layout**. `Block<AT>` is a type
alias for `BlockHeader<AT>`. Every block (header + data) is aligned to the granule size.
The header is 32 bytes = 2 granules (for `DefaultAddressTraits`) and is placed immediately
before the user data.

`BlockHeader<AT>` is one physical record with two semantic groups:

1. **AVL/state slot prefix** — `weight`, `left_offset`, `right_offset`, `parent_offset`,
   `root_offset`, `avl_height`, `node_type`. Used by the free-tree and by other forest
   domains that index allocated blocks (`pstringview`, `pmap`).
2. **Linear PAP links** — `prev_offset`, `next_offset`. Define the block's physical
   neighbours in the linear address space; used by split / coalesce / repair.

Binary layout for `BlockHeader<DefaultAddressTraits>` (asserted via `BlockLayoutContract`
and `static_assert` in `include/pmm/block_header.h`, verified in
`tests/test_block_header.cpp`):

| Field | Bytes | Type | Description |
|-------|-------|------|-------------|
| `weight` | 0–3 | `uint32_t` | User data size in granules (0 = free block) |
| `left_offset` | 4–7 | `uint32_t` | Left child AVL node (granule index) |
| `right_offset` | 8–11 | `uint32_t` | Right child AVL node (granule index) |
| `parent_offset` | 12–15 | `uint32_t` | Parent AVL node (granule index) |
| `root_offset` | 16–19 | `uint32_t` | 0 = free block; own index = allocated block |
| `avl_height` | 20–21 | `int16_t` | AVL subtree height (0 = not in tree) |
| `node_type` | 22–23 | `uint16_t` | 0=`kNodeReadWrite`, 1=`kNodeReadOnly` (permanently locked) |
| `prev_offset` | 24–27 | `uint32_t` | Previous block granule index (`kNoBlock` = none) |
| `next_offset` | 28–31 | `uint32_t` | Next block granule index (`kNoBlock` = last) |

Field sizes above are for `DefaultAddressTraits` (`uint32_t` index, 16-byte granule).
For `SmallAddressTraits` (`uint16_t`) and `LargeAddressTraits` (`uint64_t`), the field
types change accordingly.

**Key invariants:**
- `weight == 0` → free block; `root_offset == 0`.
- `weight > 0` → allocated block; `root_offset == own_granule_index`.
- `node_type == kNodeReadOnly` → permanently locked; cannot be freed via `deallocate()`.

---

## Algorithms

### Memory allocation (`allocate` / `allocate_typed`)

```
1. Compute required_block_granules = kBlockHeaderGranules + ceil(user_size / kGranuleSize)
2. Search AVL free block tree for best-fit (smallest block >= required_block_granules) — O(log n)
3. If found:
   a. Remove from AVL tree
   b. If the block is significantly larger (splitting possible):
      - Initialize new free block from the remainder
      - Insert into linked list and AVL tree
   c. Mark block as allocated (set weight and root_offset)
   d. Update ManagerHeader counters
   e. Return pointer to user data
4. If not found: expand storage backend by growth ratio and retry
```

### Memory deallocation (`deallocate` / `deallocate_typed`)

```
1. If pointer is null or block is permanently locked: no-op
2. Locate Block<A> from user pointer — O(1)
3. If block is allocated:
   - Set weight = 0, root_offset = 0 (mark as free)
   - Update ManagerHeader counters
   - Attempt coalescing with adjacent free blocks
   - Insert resulting block into AVL tree
```

### Free block coalescing

When a block is freed, adjacent blocks are checked. If they are free, they are merged
into one larger block:

```
[Block (free)][free space][Block (free)] → [Block (free)][larger free space]
```

Coalescing always checks both the previous and next block.

### Block splitting

When allocating, if the found free block is significantly larger than needed, it is split:

```
[Block (allocated)][user data][Block (free)][remaining free space...]
```

Minimum size for the new free block: `sizeof(Block<A>) + kMinBlockSize`.

### Storage backend expansion

When memory is exhausted:

```
1. Allocate a new buffer of size max(old_size * grow_numerator / grow_denominator,
                                    old_size + required_bytes)
2. Copy the contents of the old buffer to the new buffer
3. Extend or add a free block at the end of the new buffer
4. Update the singleton to point to the new buffer
5. Keep the old buffer for cleanup on destroy()
```

---

## Persistence

All inter-block references are stored as **granule indices** — offsets from the buffer
start, not absolute pointers. This enables:

1. Saving the memory image to a file (`fwrite` the entire region).
2. Loading it at a different base address (`mmap` or `malloc`).
3. Using in shared memory segments.

On `load()`, the library validates the magic number, image version, total size, and granule
size, then calls `repair_linked_list()`, `recompute_counters()`, and `rebuild_free_tree()`
to restore a consistent state.

`pptr<T>` stores a granule index (2, 4, or 8 bytes depending on `address_traits`),
making it persistent: after loading the image at a different address, the same index
points to the same data.

---

## Alignment

All blocks are aligned to the granule size. User data starts immediately after the
`Block<A>` header with no additional padding:

```
[Block<A> header (2 granules)][user_data (aligned to granule size)]
```

`HeapStorage<A>` allocates the arena base via `posix_memalign` (POSIX) or
`_aligned_malloc` (Windows) so that `reinterpret_cast<uintptr_t>(base) % A::granule_size == 0`,
which matters in particular for `LargeAddressTraits` where `granule_size == 64` exceeds the
default `malloc` alignment guarantee on most platforms.

---

## Arena internals (`pmm/arena_internals.h`)

The physical-arena core is built on a small, explicit set of primitives. Allocation,
verification, repair, growth and initialization use this single layer instead of
duplicated `(base, hdr)` plumbing.

### Checked granule arithmetic

- [`detail::bytes_to_granules_checked<AT>(bytes)`](../include/pmm/arena_internals.h#pmm-detail-checkedarithmetic)
  returns `std::optional<GranuleCount<AT>>`. `0` bytes returns `0` granules; arithmetic
  overflow or a result that does not fit in `AT::index_type` returns `nullopt`.
  Overflow is **never** encoded as `0` granules.
- `detail::checked_add`, `detail::checked_mul`, `detail::round_up_checked`,
  `detail::checked_granule_offset<AT>(idx)`, `detail::fits_range(off, len, total)` are
  the overflow-safe building blocks used by the rest of the arena layer.

### `ArenaView<AT>` / `ArenaAddress<AT>` (canonical address layer)

[`detail::ArenaView<AT>`](../include/pmm/arena_internals.h#pmm-detail-arenaview) pairs the
arena `base` pointer and `ManagerHeader<AT>*` so they cannot be mismatched between two
backends. Internal allocator / verifier / recovery / layout APIs accept `ArenaView<AT>` (or
`ConstArenaView<AT>` for read-only paths) instead of a loose `(base, hdr)` pair. `valid_block`
and `fits` use `checked_granule_offset`, so an out-of-range index never triggers undefined
overflow before the bounds check runs.

Issue #375 promoted `BasicArenaView` to the **canonical address layer** of PMM. The same
type, exposed under the aliases `ArenaAddress<AT>` / `ConstArenaAddress<AT>`, owns every
checked conversion between persistent indices, raw pointers, block headers, and user pointers:
`granule_offset`, `valid_block`, `block`, `block_unchecked`, `try_user_ptr`,
`try_user_idx_from_raw`, `try_block_idx_from_user_idx`, `try_user_idx_from_block_idx`. Manager
helpers (`make_pptr_from_raw`, `block_raw_ptr_from_pptr`, `raw_user_ptr_from_pptr`, etc.)
are thin wrappers that delegate to this layer. Null conventions are explicit and never
silently conflated:

- user index `0` (`detail::kNullIdx_v<AT>`) is the persistent null pointer / null user
  pointer;
- `AT::no_block` is the physical block-list null;
- invalid conversions return `nullptr` / `std::nullopt` — never a sentinel object that the
  caller could write through.

Containers and tree access follow the same model: `parray<T>::ensure_capacity` and
`pstring::ensure_capacity` grow exclusively via `ManagerT::reallocate_typed<T>`, which may
take an in-place shrink/grow path or fall back to a fresh allocation that copies only live
elements/bytes. AVL/tree access for user-facing `pptr<T>` is split into checked
(`ManagerT::try_tree_node` returning `BlockHeader<AT>*` / `nullptr` and setting
`PmmError::InvalidPointer`) and unchecked (`ManagerT::tree_node_unchecked` returning a
`BlockHeader<AT>&` under a strict precondition); the previous ref-returning `tree_node(pptr)`
that quietly returned a thread-local sentinel is gone.

This refactor is **size-budgeted**: production source LOC (`single_include/pmm/pmm.h`) must
not exceed the committed baseline in `scripts/source-loc-baseline.txt`. The
`scripts/check-source-loc-budget.sh` CI gate fails the build on regressions and on the
forbidden patterns the refactor removed (sentinel `BlockHeader`, manual
`allocate -> memcpy -> deallocate` paths inside `parray`/`pstring`, ref-returning
`tree_node(pptr)`).

### Walker `for_each_physical_block`

[`detail::for_each_physical_block<AT>(arena, fn)`](../include/pmm/arena_internals.h#pmm-detail-blockwalker)
is the single physical-chain walker. It bounds the traversal to `total_size / granule_size + 1`
steps, validates every block index against the arena, and rejects backwards or non-progressing
`next_offset` (so cyclic or backward-pointing chains terminate quickly with a failure).
Callbacks may return `bool` (`false` propagates as walker failure) or `WalkControl`
(`StopOk` / `Continue` / `Fail`) when the caller needs to distinguish early stop from
corruption. `verify_linked_list`, `verify_counters`, `verify_block_states`,
`verify_free_tree`, `recompute_counters` and `for_each_block` all share this walker; no
hand-rolled `while (idx != no_block)` loop remains in the verifier / recovery paths.

### Growth policy `compute_growth`

[`detail::compute_growth(current, min_required, gran, num, den, max_gb)`](../include/pmm/arena_internals.h#pmm-detail-growthpolicy)
is the only growth policy. It honours `Config::grow_numerator`, `Config::grow_denominator`
and `Config::max_memory_gb`, picks the larger of the ratio target and `current + min_required`,
rounds the result up to `granule_size` via `round_up_checked`, and returns `nullopt` on any
overflow or cap violation. Storage backends (`HeapStorage`, `MMapStorage`, `StaticStorage`)
expose `resize_to(new_total_size)` and do not run their own growth formula. `do_expand`
calls `compute_growth` exactly once and then `backend.resize_to(*target)`.

### Transactional `create()`

[`detail::InitGuard`](../include/pmm/arena_internals.h#pmm-detail-initguard) makes
`PersistMemoryManager::create(...)` transactional: every `create` path grabs an `InitGuard`
on `_initialized`. If any phase (`init_layout`, `bootstrap_forest_registry_unlocked`,
`validate_bootstrap_invariants_unlocked`) fails, the guard's destructor stores
`_initialized = false` so no public allocation API ever observes a partially initialized
manager. The successful path calls `guard.commit()` after every invariant is established.

### Public allocation error contract

- `allocate(0)` → `nullptr`, `last_error() == PmmError::InvalidSize`.
- `allocate(size)` with `size` that overflows on the byte-to-granule conversion or above
  `index_type::max` granules → `nullptr`, `last_error() == PmmError::Overflow` (no tiny
  fallback allocation).
- `reallocate_typed` runs `bytes_to_granules_checked` exactly once and reports the same
  errors; in-place grow / shrink and out-of-place fallback all share that one checked
  granule count. The source `pptr` is validated through `try_checked_block_from_pptr`
  before any block access, so below-header offsets and stale pptrs (after
  `deallocate_typed`) return a null `pptr<T>` with `PmmError::InvalidPointer`.
- `verify()` and `load(VerifyResult&)` always terminate, even on a corrupted physical
  chain: the bounded walker rejects cycles and backward links instead of looping.

---

## Thread safety

Thread safety is controlled by the `lock_policy` configuration field:

- **[SharedMutexLock](../include/pmm/config.h#pmm-config-sharedmutexlock)**: uses `std::shared_mutex`.
  - Read operations: `shared_lock` (concurrent execution allowed).
  - Write operations: `unique_lock` (exclusive access).
- **[NoLock](../include/pmm/config.h#pmm-config-nolock)**: no-op locks (zero overhead, single-threaded use only).

---

## Storage backends

| Backend | Description | Use case |
|---------|-------------|----------|
| `HeapStorage<A>` | Dynamically allocated via `std::malloc` / `std::realloc` | General purpose |
| `StaticStorage<A, Size>` | Fixed compile-time buffer (no dynamic allocation) | Embedded systems |
| `MMapStorage<A>` | Memory-mapped file (`mmap` / `MapViewOfFile`) | File-backed persistence |

---

## Address traits

The address space is parameterized by `AddressTraits<IndexT, GranuleSz>`:

| Predefined alias | `index_type` | Granule | Max size | `sizeof(pptr<T>)` |
|-----------------|-------------|---------|----------|-------------------|
| `TinyAddressTraits` | `uint8_t` | 8 B | ~2 KB | 1 byte |
| `SmallAddressTraits` | `uint16_t` | 16 B | ~1 MB | 2 bytes |
| `DefaultAddressTraits` | `uint32_t` | 16 B | 64 GB | 4 bytes |
| `LargeAddressTraits` | `uint64_t` | 64 B | Petabyte+ | 8 bytes |

Choosing a smaller `index_type` reduces `pptr<T>` size at the cost of a lower address
space limit.

---

## Lock policies

| Policy | Description |
|--------|-------------|
| `config::NoLock` | No-op locks; zero overhead; single-threaded only |
| `config::SharedMutexLock` | `std::shared_mutex`; `shared_lock` for reads, `unique_lock` for writes |

---

## Persistent data structures

### `pstringview<ManagerT>`

An interned read-only persistent string. Multiple calls with the same content return the
same [pptr](../include/pmm/pptr.h#pmm-pptr) (deduplication). Uses the built-in AVL slot of the
[BlockHeader](../include/pmm/block_header.h#pmm-blockheader) of each allocated block (fields
`weight`, `left_offset`, `right_offset`, `parent_offset`, `avl_height`) as AVL tree links —
no separate AVL node allocations. Blocks are permanently locked via
`lock_block_permanent()`.

```
forest domain: system/symbols (persistent root in registry)
│
├── [Block_A][pstringview: length=5, str="hello"]
│     left_offset → Block_B
│     right_offset → Block_C
│
├── [Block_B][pstringview: length=3, str="abc"]
│
└── [Block_C][pstringview: length=5, str="world"]
```

### `pmap<_K, _V, ManagerT>`

A persistent AVL tree dictionary. The [pmap](../include/pmm/pmap.h#pmm-pmap) object is a typed facade over a
type-scoped `container/pmap/<type>/<binding>` forest domain; the AVL root is stored in
that domain binding while the object stores only the binding identity. Each node is an
allocated block in PAP containing `pmap_node<_K, _V>`. The built-in AVL slot of the
[BlockHeader](../include/pmm/block_header.h#pmm-blockheader) of each block (fields `weight`,
`left_offset`, `right_offset`, `parent_offset`, `avl_height`) serves as AVL tree links.
Nodes are **not** permanently locked (unlike [pstringview](../include/pmm/pstringview.h#pmm-pstringview)), so
they can be freed.

The `<type>` segment of the domain name is a stable fingerprint derived from `sizeof`,
`alignof`, and a small fixed set of standard `<type_traits>` categories — not from
compiler-specific spellings such as `__PRETTY_FUNCTION__` or `__FUNCSIG__`. Applications
that need absolute control over persistent type identity (for example to keep two
structurally identical PODs in distinct bindings) specialize `pmm::pmap_type_identity<T>`
with a fixed ASCII tag.

```
forest domain: container/pmap/<type>/<binding> (persistent root in registry)
│
├── [Block][pmap_node: key=42, value=100]
│     left_offset → Block with key=10
│     right_offset → Block with key=99
│
├── [Block][pmap_node: key=10, value=200]
│
└── [Block][pmap_node: key=99, value=300]
```

### Shared AVL operations (`avl_tree_mixin.h`)

All persistent containers ([pstringview](../include/pmm/pstringview.h#pmm-pstringview), [pmap](../include/pmm/pmap.h#pmm-pmap)) and the free block tree
([AvlFreeTree](../include/pmm/free_block_tree.h#pmm-avlfreetree)) share a single AVL implementation via free template functions in
`pmm::detail`. This eliminates ~250 lines of previously duplicated code through
C++ template metaprogramming.

#### Core AVL functions

- `avl_height(p)` — get height (0 if null)
- `avl_update_height(p)` — recompute height from children
- `avl_balance_factor(p)` — `height(left) - height(right)`
- `avl_rotate_right(y, root_idx, update_node)` — right rotation with custom node update
- `avl_rotate_left(x, root_idx, update_node)` — left rotation with custom node update
- `avl_rebalance_up(p, root_idx, update_node)` — walk up the tree, fixing balance
- `avl_insert(new_node, root_idx, go_left, resolve, update_node)` — insert and rebalance
- `avl_remove(target, root_idx, update_node)` — BST removal with in-order successor
- `avl_find(root_idx, compare_three_way, resolve)` — generic search with custom comparison
- `avl_min_node(p)` / `avl_max_node(p)` — subtree min/max
- `avl_inorder_successor(cur)` — next node in sorted order
- `avl_init_node(p)` — initialize node (left/right/parent = `no_block` sentinel, height = 1)
- `avl_subtree_count(p)` — recursive subtree size
- `avl_clear_subtree(p, dealloc)` — recursive deallocation with custom callback

All functions are parameterized by `PPtr` (the persistent pointer type) and `IndexType`,
making them usable with any `pptr<T, ManagerT>` specialization.

#### Custom node update callbacks (`NodeUpdateFn`)

All rotation and rebalancing functions accept an optional `NodeUpdateFn` callback
invoked after structural changes. This enables different containers to maintain
different node invariants:

- **[pmap](../include/pmm/pmap.h#pmm-pmap), [pstringview](../include/pmm/pstringview.h#pmm-pstringview)**: use default [AvlUpdateHeightOnly](../include/pmm/avl_tree_mixin.h#pmm-detail-avlupdateheightonly) (height field only)
- **[AvlFreeTree](../include/pmm/free_block_tree.h#pmm-avlfreetree)**: uses [AvlUpdateHeightOnly](../include/pmm/avl_tree_mixin.h#pmm-detail-avlupdateheightonly) via [BlockPPtr](../include/pmm/avl_tree_mixin.h#pmm-detail-blockpptr) adapter

#### `BlockPPtr<AT>` adapter (for free block tree)

`BlockPPtr<AddressTraitsT>` is a lightweight adapter wrapping raw `(base_ptr, block_index)`
pairs, making them behave like [pptr](../include/pmm/pptr.h#pmm-pptr) for the shared AVL functions. This enables
[AvlFreeTree](../include/pmm/free_block_tree.h#pmm-avlfreetree) to reuse shared rotation, rebalancing, and min_node operations instead
of maintaining ~120 lines of duplicate code.

- `BlockPPtrManagerTag<AT>` — provides `address_traits` for template resolution
- `pptr_make(BlockPPtr, idx)` — specialization propagating `base_ptr`

The shared AVL functions resolve a `BlockPPtr<AT>` to a `BlockHeader<AT>&` via
`detail::block_header_at<AT>` and read/write its AVL-slot data members
(`left_offset`, `right_offset`, `parent_offset`, `avl_height`, `weight`) directly — no
separate `TreeNode` proxy.

#### `AvlInorderIterator<NodePPtr>`

A shared in-order AVL tree iterator template that replaces identical iterator structs
previously duplicated in AVL-based containers. Provides `operator*`, `operator++`
(via `avl_inorder_successor`), and comparison operators.

---

## Data structures diagram

```
buffer_start (= Block<A>_0)
│
├── Block<A>_0 (granule 0, allocated — holds ManagerHeader)
│     weight = kManagerHeaderGranules
│     root_offset = 0 (own index)
│     prev_offset = kNoBlock
│     next_offset ──────────────────────────┐
│   [ManagerHeader inside (32 bytes offset)]│
│     magic = "PMM_V098"                    │
│     first_block_offset = 0 (self)         │
│     free_tree_root ──────────────────┐    │
│                                      │    │
├── Block<A>_free ◄────────────────────┘◄───┘ (granule 6, free block)
│     weight = 0
│     root_offset = 0
│     prev_offset = 0 (Block<A>_0)
│     next_offset ────────────────────────┐
│     [free space...]                     │
│                                         │
├── Block<A>_user ◄───────────────────────┘ (allocated user block)
│     weight > 0
│     root_offset = own_granule_idx
│     [user data...]
│
└── (end of managed region)
```

---

## Block state machine

Blocks transition between two correct states and several transient states. See
[`docs/atomic_writes.md`](atomic_writes.md) for the full state diagram and crash
recovery analysis.

### Correct states

| State | `weight` | `root_offset` | In AVL tree |
|-------|----------|---------------|-------------|
| [FreeBlock](../include/pmm/block_state.h#pmm-freeblock) | 0 | 0 | Yes |
| [AllocatedBlock](../include/pmm/block_state.h#pmm-allocatedblock) | >0 | own index | No |

### Forbidden states

| State | `weight` | `root_offset` | Valid? |
|-------|----------|---------------|--------|
| Free block not in AVL | 0 | 0 | Transient only |
| weight=0, root_offset≠0 | 0 | ≠0 | Never |
| weight>0, root_offset=0 | >0 | 0 | Never |
| weight>0 and in AVL | >0 | own idx | Never |

---

## Configuration composition

The full type of a manager is determined by composing four independent policies:

```cpp
// Example: custom multi-threaded manager backed by MMapStorage
struct MyConfig {
    using address_traits  = pmm::DefaultAddressTraits;
    using storage_backend = pmm::MMapStorage<pmm::DefaultAddressTraits>;
    using free_block_tree = pmm::AvlFreeTree<pmm::DefaultAddressTraits>;
    using lock_policy     = pmm::config::SharedMutexLock;
    static constexpr std::size_t granule_size     = 16;
    static constexpr std::size_t max_memory_gb    = 64;
    static constexpr std::size_t grow_numerator   = 5;
    static constexpr std::size_t grow_denominator = 4;
};

using MyManager = pmm::PersistMemoryManager<MyConfig, 0>;
```

---

## Multiton pattern

Each unique `(ConfigT, InstanceId)` pair is a distinct static manager with completely
independent storage. This allows multiple persistent heaps in a single process:

```cpp
using Cache  = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 0>;
using Buffer = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 1>;

Cache::create(64 * 1024);
Buffer::create(32 * 1024);

Cache::pptr<int>  cp = Cache::allocate_typed<int>();
Buffer::pptr<int> bp = Buffer::allocate_typed<int>();

*cp = 42;
*bp = 100;

Cache::deallocate_typed(cp);
Buffer::deallocate_typed(bp);
Cache::destroy();
Buffer::destroy();
```

`Cache::pptr<int>` and `Buffer::pptr<int>` are **different types** — the compiler
prevents accidental cross-manager pointer use.
