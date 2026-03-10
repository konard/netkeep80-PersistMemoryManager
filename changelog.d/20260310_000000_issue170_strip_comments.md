---
bump: minor
---

### Added
- `scripts/strip-comments.py`: Python helper that strips C/C++ comments from a
  source file while preserving string literals and line structure.
- `--strip-comments` flag for `scripts/generate-single-headers.sh`: when passed,
  also generates `single_include/pmm/pmm_no_comments.h` — a comment-free variant
  of `pmm.h` that is ~42 % smaller in line count and ~56 % smaller in byte size,
  suitable for embedded or size-critical environments.
- `tests/test_issue170_sh_no_comments.cpp`: self-sufficiency test confirming that
  `pmm_no_comments.h` compiles and runs correctly without any other PMM headers.
- CI (`single-headers` job) now validates `pmm_no_comments.h` freshness alongside
  the other single-header files.
