---
bump: patch
---

### Changed
- Made `ForestDomainOps` the single value-based protocol wrapper for AVL-backed forest domains and moved `pmap` onto it.
- Routed symbol and legacy root access through the canonical forest-domain root-index helpers.
