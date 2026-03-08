---
bump: minor
---

### Added
- `EmbeddedStaticConfig<N>` — new configuration for embedded systems without heap: `StaticStorage<N, DefaultAddressTraits>` + `NoLock`, 16B granule, no dynamic expansion (Issue #146)
- `EmbeddedStaticHeap<N>` — new preset alias for `PersistMemoryManager<EmbeddedStaticConfig<N>, 0>`, ready for bare-metal / RTOS use (Issue #146)
- `kMinGranuleSize = 4` constant — minimum supported granule size (architecture word size), enforced by `static_assert` in all configs (Issue #146)
- `static_assert` rules to all existing configs: `granule_size >= kMinGranuleSize` and `granule_size` is a power of 2 (Issue #146)
- `single_include/pmm/pmm_embedded_static_heap.h` — self-contained single-header preset file for `EmbeddedStaticHeap` (Issue #146)
- `tests/test_issue146_configs.cpp` — comprehensive tests for all config rules, `EmbeddedStaticHeap` lifecycle, and preset properties (Issue #146)
- `tests/test_issue146_sh_embedded_static.cpp` — single-header self-containment test for `pmm_embedded_static_heap.h` (Issue #146)

### Changed
- `include/pmm/manager_configs.h` — updated documentation to describe architecture rules and constraints; added `static_assert` guards to all existing configs (Issue #146)
- `include/pmm/pmm_presets.h` — updated documentation and added `EmbeddedStaticHeap` preset (Issue #146)
- `scripts/generate-single-headers.sh` — regenerates all 5 single-header files including the new `pmm_embedded_static_heap.h` (Issue #146)
- All single-header files in `single_include/pmm/` regenerated to include updated `manager_configs.h` with new configs and `static_assert` rules (Issue #146)
