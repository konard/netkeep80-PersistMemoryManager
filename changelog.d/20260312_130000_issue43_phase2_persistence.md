---
bump: minor
---

### Added
- CRC32 checksum for persisted images: `save_manager()` computes and stores CRC32, `load_manager_from_file()` verifies it (Phase 2.1)
- `compute_crc32()` and `compute_image_crc32<AT>()` utility functions in `pmm::detail` (Phase 2.1)
- `crc32` field in `ManagerHeader` (4 bytes from `_reserved`) (Phase 2.1)
- Atomic save via write-then-rename pattern with platform-specific support (Phase 2.2)
- `MMapStorage::expand()` now extends the mapping via `munmap`/`ftruncate`/`mmap` (Phase 2.3)

### Changed
- `save_manager()` now writes to a temporary `.tmp` file and atomically renames on success (Phase 2.2)
- `load_manager_from_file()` now verifies CRC32 before restoring state; corrupted images are rejected (Phase 2.1)
- `ManagerHeader::_reserved` reduced from 8 to 4 bytes (4 bytes used for `crc32`) (Phase 2.1)
