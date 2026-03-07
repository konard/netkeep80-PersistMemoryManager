# Changelog

All notable changes to PersistMemoryManager are documented here.

## [Unreleased] — Issue #102: Full Legacy API Removal

### Breaking Changes

- **Removed `include/pmm/legacy_manager.h`** — singleton-based `PersistMemoryManager<>` API removed entirely.
- **Removed `pptr<T, void>` / `pptr<T>` (single-arg)** — `ManagerT` is now mandatory. Use `Mgr::pptr<T>` instead.
- **Removed `p.get()`, `*p`, `p->x`** — replaced by `p.resolve(mgr)`.
- **Removed `reallocate_typed()`** — use manual alloc-copy-dealloc pattern.
- **Removed `pmm::save()` / `pmm::load_from_file()`** — use `pmm::save_manager()` / `pmm::load_manager_from_file()`.
- **Removed `pmm::get_stats()` / `MemoryStats` / `pmm::get_manager_info()`** — use `pmm.block_count()`, `pmm.free_block_count()`, `pmm.alloc_block_count()`.
- **Removed `validate()` from public API** — use `pmm.is_initialized()`.
- **Removed `block_data_size_bytes()`** — not in new API.
- **Removed `PMMConfig` / `DefaultConfig`** — use `manager_configs.h` configurations.
- **Removed `pmm::for_each_block()`** — not in new API.
- **Removed `config.h` `PMMConfig`** — lock policies (`NoLock`, `SharedMutexLock`) kept.

### Migration

See [docs/MIGRATION_GUIDE.md](docs/MIGRATION_GUIDE.md) for complete migration instructions.

### Updated Files

#### Tests (rewritten for new API)
- `tests/test_reallocate.cpp` — rewritten with manual alloc-copy-dealloc pattern
- `tests/test_block_modernization.cpp` — removed legacy save/load, get_stats, validate
- `tests/test_issue73_refactoring.cpp` — removed PersistMemoryManager<> singleton usage
- `tests/test_issue83_constants.cpp` — removed DefaultConfig/PMMConfig usage
- `tests/test_issue87_abstraction.cpp` — removed legacy API in Parts A and C
- `tests/test_issue87_phase4.cpp` — removed legacy_manager.h dependency
- `tests/test_issue87_phase6.cpp` — removed legacy_manager.h, added io.h
- `tests/test_thread_safety.cpp` — PersistMemoryManager<> → MultiThreadedHeap
- `tests/test_shared_mutex.cpp` — PersistMemoryManager<> → MultiThreadedHeap
- `tests/test_scenarios_issue34.cpp` — singleton → SingleThreadedHeap instance
- `tests/test_issue100.cpp` — removed pptr<T,void> backward compat tests
- `tests/test_issue97_presets.cpp` — pmm::pptr<T> → MgrType::pptr<T>

#### Examples (rewritten for new API)
- `examples/basic_usage.cpp` — legacy_manager.h → pmm_presets.h
- `examples/benchmark.cpp` — PersistMemoryManager<> → SingleThreadedHeap
- `examples/stress_test.cpp` — PersistMemoryManager<> → SingleThreadedHeap
- `examples/persistence_demo.cpp` — pmm::save/load_from_file → pmm::save_manager/load_manager_from_file
- `examples/new_api_usage.cpp` — pmm::pptr<T> → Manager::pptr<T>

#### Documentation
- `docs/MIGRATION_GUIDE.md` — new migration guide

### Test Results

All 28 tests pass (100%) after this change.

---

## Previous Releases

### Issue #100 — Infrastructure Preparation

- Added `AbstractPersistMemoryManager` — new instance-based manager template
- Added `manager_concept.h` — `is_persist_memory_manager<T>` concept/trait
- Added `static_manager_factory.h` — `StaticPersistMemoryManager<ConfigT, Tag>`
- Added `manager_configs.h` — `CacheManagerConfig`, `PersistentDataConfig`, `EmbeddedManagerConfig`, `IndustrialDBConfig`
- Added `pptr<T, ManagerT>` — two-parameter persistent pointer (ManagerT mandatory)
- Added `Manager::pptr<T>` nested alias

### Issue #97 — Presets and New IO API

- Added `pmm_presets.h` — ready-made manager configurations
- Added `pmm::save_manager()` / `pmm::load_manager_from_file()`
- Added typed allocation API: `allocate_typed<T>()`, `resolve<T>()`, `resolve_at<T>()`

### Issue #87 — AbstractPersistMemoryManager Architecture

- Added `AddressTraits`, `LinkedListNode`, `TreeNode`, `Block`
- Added `AvlFreeTree` — AVL-tree based free block management
- Added `HeapStorage`, `StaticStorage`, `MMapStorage` backends
- Added `AllocatorPolicy`

### Issue #83 — Constants

- Added `config::kGranuleSize`, `kMinBlockSize`, `kMinMemorySize`
- Added `config::kDefaultGrowNumerator` / `kDefaultGrowDenominator`

### Issue #75 — BlockHeader_0 sentinel

- `alloc_block_count()` now always includes `BlockHeader_0` (returns >= 1)
