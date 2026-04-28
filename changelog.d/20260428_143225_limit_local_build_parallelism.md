---
bump: patch
---

### Changed
- Local builds now cap parallelism at 3 jobs by default (issue #371) so a
  single contributor cannot saturate every core on a shared workstation.
  `CMakeLists.txt` exposes `PMM_LOCAL_BUILD_JOBS` (default `3`, `0` =
  unbounded), routes it into the MSVC `/MP` flag, and configures
  `CMAKE_JOB_POOLS` so the Ninja generator caps compile/link jobs in the
  generated build graph. The cap is suppressed when the `CI` environment
  variable is set or `PMM_LOCAL_BUILD_JOBS=0`. Helper scripts
  (`scripts/test.sh`, `scripts/test.bat`, `scripts/demo.sh`,
  `scripts/demo.bat`) pass `-j 3` to `cmake --build` and `ctest`,
  overridable via `PMM_JOBS`. GitHub Actions sets `CI=true` automatically,
  so CI keeps using full parallelism via `CMAKE_BUILD_PARALLEL_LEVEL` /
  `CTEST_PARALLEL_LEVEL`.
- PR-triggered workflows (`ci.yml`, `docs-consistency.yml`,
  `repo-guard.yml`) now use `pull_request.types: [opened, synchronize]` so
  CI/CD only re-runs on actual commits, not on `reopened` /
  `ready_for_review` lifecycle events (issue #371 follow-up comment).
