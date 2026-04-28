---
bump: major
---

### Added
- `pmm/arena_internals.h` consolidates the internal arena primitives:
  - `detail::bytes_to_granules_checked<AT>()` returns `std::optional<GranuleCount<AT>>`; overflow is never encoded as `0` granules.
  - `detail::checked_add()`, `detail::checked_mul()`, `detail::fits_range()`, `detail::round_up_checked()`, `detail::checked_granule_offset<AT>()` give overflow-safe arithmetic / bounds / index→offset conversion.
  - `detail::ArenaView<AT>` and `detail::ConstArenaView<AT>` pair `base` and `ManagerHeader<AT>*`, replacing loose `(base, hdr)` plumbing in allocator / verifier / recovery / layout APIs. `valid_block` / `fits` use `checked_granule_offset` so an overflowing index never triggers undefined behaviour before the bounds check.
  - `detail::for_each_physical_block<AT>(arena, fn)` is the single physical-chain walker. The traversal is bounded by `total_size / granule_size + 1`, rejects backward / non-progressing `next_offset`, propagates `false` returned from the callback as walker failure, and supports `WalkControl{Continue, StopOk, Fail}` for verify-style callbacks.
  - `detail::compute_growth()` is the single growth policy honouring `grow_numerator` / `grow_denominator` / `max_memory_gb`, overflow-safe and rounded up to `granule_size` via `round_up_checked`.
  - `detail::InitGuard` makes `PersistMemoryManager::create()` transactional — failed bootstrap now leaves `_initialized` at `false`.

### Changed
- `PersistMemoryManager::allocate()` rejects `allocate(0)` with `PmmError::InvalidSize` and `allocate(huge)` with `PmmError::Overflow`. The size-to-granule conversion happens exactly once via `bytes_to_granules_checked`. No tiny fallback allocation on overflow.
- `PersistMemoryTypedApi::reallocate_typed` runs the same checked granule path once and uses it for in-place grow / shrink and the out-of-place fallback. The legacy `bytes_to_granules_t` clamp-to-1 fallback is gone.
- `AllocatorPolicy::allocate_from_block`, `coalesce`, `realloc_shrink`, `realloc_grow`, `repair_linked_list`, `recompute_counters`, `rebuild_free_tree` all take `detail::ArenaView<AT>`. `verify_block_states`, `verify_linked_list`, `verify_counters`, `verify_free_tree` take `detail::ConstArenaView<AT>` and run on the shared `for_each_physical_block` walker; the hand-rolled `while (idx != no_block)` loops are removed from verifier / recovery / `for_each_block` paths.
- Storage backend contract changed from `expand(additional_bytes)` to `resize_to(new_total_size)`. `HeapStorage`, `MMapStorage` and `StaticStorage` no longer run their own growth formula; `do_expand` calls `compute_growth` once and `backend.resize_to(*target)`.
- `HeapStorage<AT>` uses `posix_memalign` / `_aligned_malloc` and `round_up_checked` for size rounding, guaranteeing `base_ptr() % AT::granule_size == 0` (fixes potential under-alignment for `LargeAddressTraits`, granule = 64).
- `PersistMemoryManager` requires `ConfigT::grow_numerator`, `ConfigT::grow_denominator`, `ConfigT::max_memory_gb` directly via `static_assert`; the SFINAE detection helpers (`detail::config_grow_numerator` / `_denominator` / `_max_memory_gb`) are removed. All bundled configs already define these fields.
- `verify()` and `load(VerifyResult&)` always terminate on a corrupted physical chain — the bounded walker rejects cycles and backward links instead of looping.

### Removed
- `AllocatorPolicy::allocate_from_block(base, hdr, blk_idx, user_size)` — replaced by the `ArenaView`-based signature taking a checked `data_gran`.
- `AllocatorPolicy` `(base, hdr)` overloads of allocator / verifier / recovery / layout entry points.
- Storage backend `expand(additional_bytes)`.
- Compatibility SFINAE shims for `grow_numerator` / `grow_denominator` / `max_memory_gb`.

### Tests
- `tests/test_issue373_arena_internals.cpp` covers the new helpers (`round_up_checked`, `checked_granule_offset`), walker `WalkControl` / failure semantics, `ArenaView`, `compute_growth`, transactional `InitGuard`, alignment.
- `tests/test_issue373_corruption.cpp` covers real-path failures: cyclic / backward physical chains terminate, `verify()` reports corruption without hanging, `reallocate_typed` overflow path returns `PmmError::Overflow` without a tiny fallback, `compute_growth` cap, real `do_expand` going through `compute_growth`, and `ArenaView::valid_block` rejecting overflowing indices.
- `tests/test_issue43_phase2_persistence.cpp`, `test_issue87_phase5.cpp`, `test_issue87_phase7.cpp`, `test_issue235_code_review.cpp` updated for the `resize_to` contract and the strict config requirement.
