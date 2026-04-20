---
bump: patch
---

### Changed
- Narrow `block_state.h` docstring and surrounding docs (`atomic_writes.md`,
  `architecture.md`) to describe the block FSM strictly as the allocator /
  free-tree physical mutation protocol. Clarify explicitly that `pmap` and
  `pstringview` operate on already-allocated blocks and do not traverse the
  `FreeBlock ↔ AllocatedBlock` transitions defined here — this is not a
  general forest-node lifecycle.
