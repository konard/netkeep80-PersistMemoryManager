---
bump: patch
---

### Changed
- Made `ForestDomainViewOps`/`ForestDomainOps` the canonical read-only/mutable protocol surfaces for
  AVL-backed forest domains and moved `pmap` onto them.
- Routed symbol and legacy root access through the canonical forest-domain root-index helpers.
