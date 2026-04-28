---
bump: major
---

### Changed

- `BlockHeader<AT>` layout: `avl_height` is now a compact `std::uint8_t` and
  `node_type` is a strongly typed `enum class NodeType : std::uint8_t`. Both
  fields live at the very end of the header so future single-byte fields can
  be packed next to them without moving existing offsets.
- `BlockStateBase::set_avl_height_of` and the `tree_node` setters now take
  `std::uint8_t` for the AVL height, matching the new field type.
- `BlockStateBase::init_fields` now requires an explicit `NodeType` parameter
  so callers always commit to the high-level kind of the block they are
  initializing.
- `weight` is now the cached size of the block in granules. For a free block
  it is the total block size including the header; for an allocated block it
  is the payload size. The AVL free-tree reads this cached value directly via
  `BlockState::get_weight` instead of recomputing the size from
  `next_offset - own_idx`.
- State of a block is determined exclusively by `node_type`. The allocator
  and the free-tree no longer infer free state from `weight == 0` and instead
  consult the `is_free`, `is_allocated`, `is_mutable`,
  `can_be_deleted_from_pap`, and `participates_in_free_tree` helpers in
  `block_header.h`.

### Added

- `pmm::NodeType` enumerates all current physical and logical node kinds:
  `Free`, `ManagerHeader`, `Generic`, `ReadOnlyLocked`, `PStringView`,
  `PString`, `PArray`, `PMap`, `PPtr`. Adding a new persistent object type
  only requires registering its properties in the centralized `is_*` helpers
  and providing a `node_type_for<T>` specialization.
- `pmm::node_type_for<T>` trait (with specializations for `pstring`,
  `pstringview`, `parray`, `pmap`, `pptr`) lets the typed allocation paths
  tag each block with its logical kind. `allocate_typed<T>` /
  `create_typed<T>` / `reallocate_typed<T>` now stamp
  `node_type_for_v<T>` onto the block instead of leaving every typed block
  as `Generic`.
- `pmm::detail::physical_block_total_granules<AT>()` and
  `cached_block_total_granules<AT>()` split the previous combined helper so
  validation and repair paths can compare cached `weight` against the
  physical block span derived from neighbours.
- `verify_block_states` cross-checks every free block's cached `weight`
  against the physical span computed from neighbours and reports
  `BlockStateInconsistent` on mismatch, catching corrupted-cache cases that
  the AVL ordering invariant alone cannot see.
- New requirements DR-013 (weight semantics), DR-014 (`avl_height`), DR-015
  (`NodeType`), DR-016 (centralized property helpers), and DR-017 (free-tree
  uses cached `weight`) plus a `NodeType` properties table in
  `req/06_data_requirements.md`.

### Removed

- The legacy `kNodeReadWrite` / `kNodeReadOnly` constants are gone — all
  call sites use `NodeType::ReadOnlyLocked` (or another `NodeType` value)
  through the typed API.
- The legacy invariant "free block means `weight == 0`" is gone. Tests that
  encoded that invariant have been rewritten to assert state through
  `node_type` and the `is_*` helpers.

### Fixed

- `is_allocated(NodeType)` is now a closed-world `switch` over every known
  enum value. A corrupted `node_type` byte no longer slips through as an
  allocated state — it is now neither `is_free` nor `is_allocated`.
- `NodeType::Free` is no longer reported as user/PAP-deletable. Both
  `can_be_deleted_from_pap(NodeType::Free)` and the explicit
  `is_allocated(nt) && can_be_deleted_from_pap(nt)` guard inside
  `deallocate_unlocked()` now refuse to act on a free block, closing a
  release-mode double-free / stale-pointer hole that became reachable once
  free blocks started carrying a non-zero cached `weight`.
- AVL height is now assigned to the `uint8_t` storage with an explicit cast
  and a debug-build range assertion instead of an implicit narrowing
  conversion from `int16_t`.
- Removed an unreachable `(void)base;` in
  `AllocatorPolicy::compute_total_block_granules`.
