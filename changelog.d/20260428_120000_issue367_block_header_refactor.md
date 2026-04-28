---
bump: major
---

### Changed
- Replaced `detail::BlockFieldLayout`/`TreeNode<AT>`/`Block<AT> : TreeNode<AT>`/`BlockStateBase<AT> : private Block<AT>` with a single `BlockHeader<AT>` standard-layout, trivially-copyable struct (`include/pmm/block_header.h`). `Block<AT>` is now a type alias for `BlockHeader<AT>`. Hot paths read and write block-header fields directly instead of going through `read_block_field` / `write_block_field` per-field `memcpy` machinery.
- Reworked `FreeBlock`, `AllocatedBlock`, `FreeBlockRemovedAVL`, `FreeBlockNotInAVL`, `SplittingBlock`, `CoalescingBlock` as non-owning typed views over `BlockHeader<AT>&`. State transitions return new value-typed views instead of pointers and operate on the header by reference.
- Bumped the persistent image format to version 2 (`detail::kCurrentImageVersion`); legacy unversioned images are no longer migrated and are rejected with `PmmError::UnsupportedImageVersion` on `load`/`verify`.
- Regenerated `single_include/pmm/pmm.h` and the preset single-header files from the refactored sources.

### Added
- `include/pmm/block_header.h` defines `BlockHeader<AT>`, `BlockLayoutContract<AT>`, and `detail::block_header_at<AT>()`.
- Checked block-state cast APIs `FreeBlock<AT>::can_cast_from_raw` / `try_cast_from_raw` and `AllocatedBlock<AT>::can_cast_from_raw` / `try_cast_from_raw` for boundaries where `cast_from_raw` preconditions are not already proven by the caller (the unchecked `cast_from_raw` is documented as precondition-only and remains UB on invalid input in release builds).
- `tests/test_block_header.cpp` covers the new layout/contract/access/state-transition surface, the checked cast boundaries, and the breaking image-version policy.

### Removed
- `include/pmm/block_field.h` and `include/pmm/tree_node.h` (deleted).
- `detail::BlockFieldLayout`, `BlockFieldTraits`, `BlockFieldByteAccess`, `read_block_field`, `write_block_field`, `block_field_value_t`, `block_field_offset_v`, `block_tree_slot_size_v`, `block_layout_size_v`, `PMM_BLOCK_FIELD`, `PMM_BLOCK_INDEX_FIELD`, and the per-field `BlockWeightField`/`BlockLeftOffsetField`/etc. tags.
- `TreeNode<AT>`, `BlockTreeNodeProxy<AT>`, the `BlockStateBase<AT> : private Block<AT>` inheritance, and the `state_from_raw<T>()` reinterpret-cast inheritance model.
- `BlockTreeAccess<AT>` adapter — production code uses direct `BlockHeader<AT>&` field access; the adapter was only ever exercised by tests.
- Obsolete tests pinned to the deleted machinery (`test_block_state.cpp`, `test_issue87_abstraction.cpp`, `test_issue87_phase2.cpp`, `test_issue87_phase3.cpp`).
