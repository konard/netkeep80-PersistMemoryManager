---
bump: major
---

### Added
- `pmm/arena_internals.h` consolidates new internal primitives:
  - `detail::bytes_to_granules_checked<AT>()` returns `std::optional<GranuleCount<AT>>` so overflow is no longer encoded as `0` granules.
  - `detail::checked_add()`, `detail::checked_mul()`, `detail::fits_range()` for overflow-safe arena bounds.
  - `detail::ArenaView<AT>` non-owning wrapper that pairs `base` and `ManagerHeader<AT>*`, replacing loose `(base, hdr)` plumbing.
  - `detail::for_each_physical_block<AT>()` is the single physical-block-chain walker; it detects cycles via a step bound, validates `prev_offset`, and rejects backward `next_offset`.
  - `detail::compute_growth()` is the single growth policy honouring `grow_numerator` / `grow_denominator` / `max_memory_gb`, with overflow-safe rounding to `granule_size`.
  - `detail::InitGuard` makes `PersistMemoryManager::create()` transactional — failed bootstrap now leaves `_initialized` at `false`.

### Changed
- `PersistMemoryManager::allocate()` rejects `allocate(0)` with `PmmError::InvalidSize` and `allocate(huge)` with `PmmError::Overflow`. The size-to-granule conversion happens exactly once via `bytes_to_granules_checked`.
- `AllocatorPolicy::allocate_from_block()` now accepts a checked `data_gran` instead of `user_size`. Callers (`allocate_unlocked`, `reallocate_typed`) compute `data_gran` once and pass it through.
- `PersistMemoryManager::create(...)` uses `InitGuard` so any failure during layout / forest bootstrap / invariant validation leaves `_initialized` at `false` and updates `_last_error`.
- `ManagerLayoutOps::do_expand()` now uses `detail::compute_growth()` instead of an open-coded `old_size / 4 + min_need` formula. The config's `grow_numerator` / `grow_denominator` / `max_memory_gb` are honoured uniformly.
- `HeapStorage<AT>` uses `posix_memalign` (POSIX) or `_aligned_malloc` (Windows) so the base pointer is aligned to `AT::granule_size`. This fixes potential under-alignment for `LargeAddressTraits` (granule_size == 64).

### Removed
- The single-overload `AllocatorPolicy::allocate_from_block(base, hdr, blk_idx, user_size)` is gone. Callers must pass a checked `data_gran`.
