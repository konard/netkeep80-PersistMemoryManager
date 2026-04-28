---
bump: patch
---

### Changed
- Local builds now cap parallelism at 3 jobs by default (issue #371) so a
  single contributor cannot saturate every core on a shared workstation.
  `CMakeLists.txt` exposes `PMM_LOCAL_BUILD_JOBS` (default `3`, `0` =
  unbounded) and routes it into the MSVC `/MP` flag when the `CI`
  environment variable is unset. `scripts/test.bat` and `scripts/demo.bat`
  pass `-j 3` to `cmake --build` and `ctest`, overridable via `PMM_JOBS`.
  GitHub Actions sets `CI=true` automatically, so CI keeps using full
  parallelism via `CMAKE_BUILD_PARALLEL_LEVEL` / `CTEST_PARALLEL_LEVEL`.
