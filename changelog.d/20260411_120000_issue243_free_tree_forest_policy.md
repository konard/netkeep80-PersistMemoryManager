---
bump: patch
---

### Changed
- Align `AvlFreeTree` with the general forest model (Issue #243):
  - Document free-tree as a specialized forest-policy with explicit ordering semantics
  - Clarify that `weight` serves as a state discriminator (not sort key) in the free-tree domain
  - Add `kForestDomainName` tag to `AvlFreeTree` for forest-policy identification
  - Update `TreeNode` field comments to use forest-model terminology
  - Synchronize `block_and_treenode_semantics.md` and `pmm_avl_forest.md` with free-tree policy
  - Add new canonical document: `docs/free_tree_forest_policy.md`
