---
bump: minor
---

### Added
- Add `single_include/pmm/pmm.h` — a new single-header file bundling the full PMM library
  without any specific preset specialization, allowing users to drop a single file into their
  project and use any configuration or define their own `PersistMemoryManager<MyConfig>`

### Changed
- Reorganize single-header preset files (`pmm_embedded_heap.h`, `pmm_single_threaded_heap.h`,
  `pmm_multi_threaded_heap.h`, `pmm_industrial_db_heap.h`, `pmm_embedded_static_heap.h`,
  `pmm_small_embedded_static_heap.h`, `pmm_large_db_heap.h`) to be thin wrappers that
  include `pmm.h` and define only their specific preset alias — eliminating ~36k lines of
  duplicated code from the release and making preset files serve as clear usage examples
- Update `scripts/generate-single-headers.sh` to generate `pmm.h` and all thin preset files
- Update CI `single-headers` check to verify `pmm.h` is up to date alongside preset files
- Update tests that use multiple presets to include all required preset headers explicitly
  instead of relying on one preset file containing all definitions
