---
bump: major
---

### Added
- `pmm/arena_internals.h` consolidates the internal arena primitives:
  - `detail::bytes_to_granules_checked<AT>()` returns `std::optional<GranuleCount<AT>>`; overflow is never encoded as `0` granules.
  - `detail::checked_add()`, `detail::checked_mul()`, `detail::fits_range()`, `detail::round_up_checked()`, `detail::checked_granule_offset<AT>()`, `detail::byte_off_to_idx_checked<AT>()`, `detail::max_arena_size<AT>()` give overflow-safe arithmetic / bounds / index↔offset conversion.
  - `detail::ArenaView<AT>` and `detail::ConstArenaView<AT>` pair `base` and `ManagerHeader<AT>*`, replacing loose `(base, hdr)` plumbing in allocator / verifier / recovery / layout APIs. `valid_block` / `fits` use `checked_granule_offset` so an overflowing index never triggers undefined behaviour before the bounds check.
  - `detail::for_each_physical_block<AT>(arena, fn)` (and the matching `for_each_physical_block_mut<AT>` for repair paths) is the single physical-chain walker. The traversal is bounded by `total_size / granule_size + 1`, rejects backward / non-progressing `next_offset`, propagates `false` returned from the callback as walker failure, and supports `WalkControl{Continue, StopOk, Fail}` for verify-style callbacks.
  - `detail::compute_growth()` and `detail::compute_growth_for_traits<AT>()` form the single growth policy honouring `grow_numerator` / `grow_denominator` / `max_memory_gb` **and** the maximum addressable arena size for the chosen `AddressTraits`. Overflow-safe, rounded up to `granule_size` via `round_up_checked`.
  - `detail::InitGuard` makes `PersistMemoryManager::create()` transactional — failed bootstrap now leaves `_initialized` at `false`.

### Changed
- `PersistMemoryManager::allocate()` rejects `allocate(0)` with `PmmError::InvalidSize` and `allocate(huge)` with `PmmError::Overflow`. The size-to-granule conversion happens exactly once via `bytes_to_granules_checked`. No tiny fallback allocation on overflow.
- `PersistMemoryTypedApi::reallocate_typed` runs the same checked granule path once and uses it for in-place grow / shrink and the out-of-place fallback.
- `AllocatorPolicy::allocate_from_block` rejects `data_gran == 0` (returns `nullptr`) instead of clamping to one granule. The check enforces the `bytes_to_granules_checked` contract end-to-end.
- `AllocatorPolicy::repair_linked_list`, `rebuild_free_tree`, `recompute_counters` now run on the shared bounded walker (`for_each_physical_block_mut` / `for_each_physical_block`); `verify_block_states`, `verify_linked_list`, `verify_counters`, `verify_free_tree` already used the const walker. The recovery code paths called from `load()` therefore terminate on cyclic or backward `next_offset` corruption.
- `PersistMemoryManager::for_each_block()` propagates walker failure to its caller instead of silently returning `true` on a corrupt chain.
- `validate_block_index` / `validate_block_header_full` use overflow-safe arithmetic. `is_valid_user_offset_unlocked` uses `checked_granule_offset` + `fits_range` instead of open-coded `byte_off + size`.
- `do_expand()` uses `compute_growth_for_traits<AT>()` so the growth target is also capped by the maximum representable arena size for `AT`. The `byte_off → index` conversion goes through `byte_off_to_idx_checked<AT>()`.
- Storage backend contract changed from `expand(additional_bytes)` to `resize_to(new_total_size)`. `HeapStorage`, `MMapStorage` and `StaticStorage` no longer run their own growth formula.
- `HeapStorage<AT>` uses `posix_memalign` / `_aligned_malloc` and `round_up_checked` for size rounding, guaranteeing `base_ptr() % AT::granule_size == 0` (fixes potential under-alignment for `LargeAddressTraits`, granule = 64).
- `PersistMemoryManager` requires `ConfigT::grow_numerator`, `ConfigT::grow_denominator`, `ConfigT::max_memory_gb` directly via `static_assert`; the SFINAE detection helpers are removed.
- `verify()` and `load(VerifyResult&)` always terminate on a corrupted physical chain — the bounded walker rejects cycles and backward links instead of looping.

### Removed
- `AddressTraits::bytes_to_granules()`, `detail::bytes_to_granules_t<AT>()`, `detail::required_block_granules_t<AT>()` — the old overflow-as-zero / clamp-to-one helpers. Callers go through `detail::bytes_to_granules_checked<AT>()` and `detail::kBlockHeaderGranules_t<AT>`.
- `AllocatorPolicy::allocate_from_block(base, hdr, blk_idx, user_size)` legacy signature.
- `AllocatorPolicy` `(base, hdr)` overloads of allocator / verifier / recovery / layout entry points.
- Manual `while (idx != no_block) { idx = get_next_offset(...); }` loops in `repair_linked_list`, `rebuild_free_tree`, `recompute_counters`, and the public/internal `for_each_block` paths.
- Storage backend `expand(additional_bytes)`.
- Compatibility SFINAE shims for `grow_numerator` / `grow_denominator` / `max_memory_gb`.

### Tests
- `tests/test_issue373_arena_internals.cpp` covers the new helpers (`round_up_checked`, `checked_granule_offset`), walker `WalkControl` / failure semantics, `ArenaView`, `compute_growth`, transactional `InitGuard`, alignment.
- `tests/test_issue373_corruption.cpp` covers real-path failures: cyclic / backward physical chains terminate, `verify()` reports corruption without hanging, recovery (`repair_linked_list` / `rebuild_free_tree`) terminates on a cyclic chain, `reallocate_typed` overflow returns `PmmError::Overflow`, `compute_growth_for_traits` caps by the addressable-arena limit, `do_expand` goes through `compute_growth`, `ArenaView::valid_block` rejects overflowing indices, `validate_block_index` rejects out-of-range and overflow-prone indices, `byte_off_to_idx_checked` rejects misalignment and overflow, and `allocate_from_block(arena, idx, 0)` returns `nullptr` instead of allocating one granule.
- `tests/test_issue87_phase1.cpp`, `test_issue144_code_review.cpp`, `test_issue160_deduplication.cpp`, `test_issue166_deduplication.cpp`, `test_issue213_overflow.cpp` migrated from the removed `AddressTraits::bytes_to_granules` / `bytes_to_granules_t` / `required_block_granules_t` helpers to `bytes_to_granules_checked` / `byte_off_to_idx_checked`.
- `tests/test_issue43_phase2_persistence.cpp`, `test_issue87_phase5.cpp`, `test_issue87_phase7.cpp`, `test_issue235_code_review.cpp` updated for the `resize_to` contract and the strict config requirement.
