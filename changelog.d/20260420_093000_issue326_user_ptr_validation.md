---
bump: patch
---

### Fixed
- Hardened user-pointer to block-header reconstruction so forged aligned payload pointers are rejected before
  mutation paths can treat them as allocated blocks.
