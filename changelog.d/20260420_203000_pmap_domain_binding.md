---
bump: major
---

### Changed
- Moved `pmap` root ownership from the local `_root_idx` field to type-scoped `container/pmap/...` forest-domain bindings.
- `pmap` objects now carry only domain identity, so independent maps and different `_K`/`_V` node layouts do not share one root accidentally.

### Removed
- Removed `pmap::_root_idx` as a public/local root model.
