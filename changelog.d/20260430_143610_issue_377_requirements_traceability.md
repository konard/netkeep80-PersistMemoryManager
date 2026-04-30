---
bump: minor
---

### Added
- Anchor-based requirements catalog: every entry in `req/*.md` is now a `##`-level heading with a stable `ttt-xxx` id used as a cross-file anchor.
- New CI guard `scripts/check-requirements-traceability.py` validating id format, id-to-file mapping, presence of downstream markers, internal anchor link resolvability, and that every `req:` annotation in `include/pmm/**.h` and `tests/**.cpp` references a known requirement id. Wired into `.github/workflows/docs-consistency.yml`.
- `req:` annotations on existing PMM anchor blocks in `include/pmm/*.h` and `tests/*.cpp` linking implementation/verification anchors to the requirements they cover.
- `req/README.md` documenting the requirements catalog format and traceability rules.

### Changed
- `scripts/check-include-anchor-comments.sh` now accepts optional `req: <id>[, <id>...]` lines inside PMM anchor comments while keeping the existing strictness on heading shape and depth.
