---
bump: patch
---

### Fixed
- Made `save_manager()` write a locked snapshot without mutating the live CRC field, and flush the temp file plus parent directory for crash-durable saves.
