---
bump: major
---

### Removed
- `pmm::detail::kServiceNameLegacyRoot` service domain constant.
- `service/legacy_root` domain record from the forest registry bootstrap.
- `reserved_root_offset` field from `ForestDomainRegistry`.
- Legacy-root migration branch in `validate_or_bootstrap_forest_registry_unlocked()`
  that reconstructed a registry from a pre-registry `hdr->root_offset`.
- `create_forest_registry_root_unlocked( legacy_root_offset )` helper
  (folded into `bootstrap_forest_registry_unlocked()`).

### Changed
- `set_root`/`get_root` now back onto the canonical `service/domain_root`
  registry record instead of the removed `service/legacy_root`.
- `validate_bootstrap_invariants_unlocked()` now requires `service/domain_root`
  to be present and drops the `reserved_root_offset == 0` assertion.
- Persisted `ForestDomainRegistry` layout changed
  (`reserved_root_offset` removed); old images are not migrated.

### Notes
- Breaking change: images created before this revision cannot be loaded.
  The compatibility audit entry for `legacy_root_offset` is deleted and
  replaced by a note on the canonical `service/domain_root` record.
