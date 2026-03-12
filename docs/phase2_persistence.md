# Phase 2: Persistence and Reliability (Issue #43)

This document describes the persistence and reliability improvements implemented in Phase 2 of the development plan.

## 2.1 CRC32 Checksum for Persisted Images

**Problem:** `save_manager()` / `load_manager_from_file()` perform raw binary copying. File corruption goes undetected, and loading a corrupted image silently corrupts the heap.

**Solution:**
- Added `crc32` field to `ManagerHeader` (4 bytes taken from `_reserved`, which went from 8 to 4 bytes)
- Added `compute_crc32()` — standard CRC32 (ISO 3309 / Ethernet polynomial `0xEDB88320`)
- Added `compute_image_crc32<AddressTraitsT>()` — computes CRC32 over the full managed region, treating the `crc32` field as zero bytes
- `save_manager()` now computes and stores CRC32 in the header before writing
- `load_manager_from_file()` verifies CRC32 after reading; mismatches cause load to fail
- Backward compatibility: images with `crc32 == 0` (saved before Phase 2.1) are accepted without verification

**Files:** `include/pmm/types.h`, `include/pmm/io.h`

## 2.2 Atomic Save (Write-then-Rename)

**Problem:** If the process crashes during `fwrite()`, the target file is left in a corrupted state.

**Solution:**
- `save_manager()` now writes to a temporary file (`filename.tmp`)
- After successful write + `fflush()`, performs atomic `rename()` to the target filename
- On Windows: uses `MoveFileExA` with `MOVEFILE_REPLACE_EXISTING`
- On POSIX: uses `std::rename()` (atomic when source and dest are on the same filesystem)
- If any step fails, the temporary file is cleaned up

**Files:** `include/pmm/io.h`

## 2.3 MMapStorage `expand()` Support

**Problem:** `MMapStorage::expand()` always returned `false`. Persistent databases backed by mmap files could not grow.

**Solution:**
- `expand()` now uses `munmap` + `ftruncate` + `mmap` on POSIX to extend the mapping
- On Windows: `UnmapViewOfFile` + `SetFilePointer`/`SetEndOfFile` + `MapViewOfFile`
- `expand(0)` is a no-op that returns `true`
- `expand()` on an unmapped storage returns `false`

**Files:** `include/pmm/mmap_storage.h`

## Tests

All Phase 2 improvements are covered by `tests/test_issue43_phase2_persistence.cpp`:
- CRC32 known-value test (standard test vector "123456789" = 0xCBF43926)
- Save/load round-trip with CRC32 verification (2.1)
- Corruption detection (flipped byte causes load failure) (2.1)
- Backward compatibility (crc32==0 images still load) (2.1)
- `compute_image_crc32` ignores the crc32 field value (2.1)
- No `.tmp` file remains after successful save (2.2)
- Second save replaces first save atomically (2.2)
- MMapStorage expand basic, zero, not-open, and multi-expand (2.3)
