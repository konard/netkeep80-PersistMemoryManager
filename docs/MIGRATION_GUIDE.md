# Migration Guide: Legacy API to New AbstractPersistMemoryManager API

**Issue #102:** Full transition to new PersistMemoryManager API with legacy code removal.

---

## Overview

The legacy singleton-based `PersistMemoryManager<>` API has been removed. All code must use the new instance-based `AbstractPersistMemoryManager` API via `pmm_presets.h`.

---

## Quick Reference

| Legacy (removed) | New API |
|---|---|
| `#include "pmm/legacy_manager.h"` | `#include "pmm/pmm_presets.h"` |
| `PersistMemoryManager<>::create(mem, size)` | `Mgr pmm; pmm.create(size)` |
| `PersistMemoryManager<>::allocate_typed<T>(n)` | `pmm.allocate_typed<T>(n)` |
| `PersistMemoryManager<>::deallocate_typed(p)` | `pmm.deallocate_typed(p)` |
| `PersistMemoryManager<>::reallocate_typed(p, n)` | Manual alloc-copy-dealloc |
| `pmm::pptr<T>` (single arg, void default) | `Mgr::pptr<T>` |
| `p.get()` / `*p` / `p->x` | `p.resolve(mgr)` |
| `pmm::save("file")` | `pmm::save_manager(pmm, "file")` |
| `pmm::load_from_file("file", mem, size)` | `pmm::load_manager_from_file(pmm, "file")` |
| `pmm::get_stats()` / `get_manager_info()` | `pmm.block_count()`, `pmm.free_block_count()`, `pmm.alloc_block_count()` |
| `PersistMemoryManager<>::validate()` | `pmm.is_initialized()` |
| `PersistMemoryManager<>::dump_stats(cout)` | `pmm.free_size()`, `pmm.used_size()`, `pmm.total_size()` |
| `block_data_size_bytes(offset)` | Not available (use block-level stats) |

---

## Available Presets (`pmm_presets.h`)

```cpp
namespace pmm::presets {
    using SingleThreadedHeap    = AbstractPersistMemoryManager<..., NoLock>;
    using MultiThreadedHeap     = AbstractPersistMemoryManager<..., SharedMutexLock>;
    using EmbeddedStatic4K      = AbstractPersistMemoryManager<..., StaticStorage<4096>>;
    using PersistentFileMapped  = AbstractPersistMemoryManager<..., MMapStorage>;
    using IndustrialDB          = AbstractPersistMemoryManager<..., SharedMutexLock>;
}
```

---

## Step-by-Step Migration

### 1. Replace include

```cpp
// Before:
#include "pmm/legacy_manager.h"

// After:
#include "pmm/pmm_presets.h"
using Mgr = pmm::presets::SingleThreadedHeap;  // or MultiThreadedHeap, etc.
```

### 2. Replace singleton create/destroy

```cpp
// Before:
void* mem = std::malloc(size);
pmm::PersistMemoryManager<>::create(mem, size);
// ... use ...
pmm::PersistMemoryManager<>::destroy();
std::free(mem);

// After (HeapStorage manages its own memory):
Mgr pmm;
pmm.create(size);  // HeapStorage allocates internally
// ... use ...
pmm.destroy();     // HeapStorage frees internally
```

### 3. Replace pptr usage

```cpp
// Before:
pmm::pptr<int> p = PersistMemoryManager<>::allocate_typed<int>();
*p = 42;           // operator* (removed)
int* raw = p.get(); // .get() (removed)
p->field = 1;      // operator-> (removed)

// After:
Mgr::pptr<int> p = pmm.allocate_typed<int>();
*p.resolve(pmm) = 42;
int* raw = p.resolve(pmm);
p.resolve(pmm)->field = 1;
```

### 4. Replace reallocate_typed

```cpp
// Before:
p = PersistMemoryManager<>::reallocate_typed(p, new_count);

// After (manual alloc-copy-dealloc):
Mgr::pptr<T> new_p = pmm.allocate_typed<T>(new_count);
if (!new_p.is_null()) {
    std::memcpy(new_p.resolve(pmm), p.resolve(pmm), old_count * sizeof(T));
    pmm.deallocate_typed(p);
    p = new_p;
}
```

### 5. Replace save/load

```cpp
// Before:
pmm::save("file.dat");
pmm::load_from_file("file.dat", mem, size);

// After:
pmm::save_manager(pmm, "file.dat");
pmm::load_manager_from_file(pmm, "file.dat");
```

### 6. Replace statistics

```cpp
// Before:
auto stats = pmm::get_stats();
stats.total_blocks;
stats.free_blocks;
stats.allocated_blocks;

// After:
pmm.block_count();        // total blocks
pmm.free_block_count();   // free blocks
pmm.alloc_block_count();  // allocated blocks (includes BlockHeader_0)
pmm.total_size();         // total memory size
pmm.free_size();          // free memory bytes
pmm.used_size();          // used memory bytes
```

### 7. Replace validate

```cpp
// Before:
bool ok = PersistMemoryManager<>::validate();

// After:
bool ok = pmm.is_initialized();
```

---

## Thread Safety

Use `MultiThreadedHeap` for thread-safe access:

```cpp
using Mgr = pmm::presets::MultiThreadedHeap;
Mgr pmm;
pmm.create(32 * 1024 * 1024);

// Thread-safe: all operations protected by SharedMutexLock
auto p = pmm.allocate_typed<int>();
*p.resolve(pmm) = 42;
pmm.deallocate_typed(p);
```

---

## Persistence: Save/Load Round-trip

```cpp
using Mgr = pmm::presets::SingleThreadedHeap;

// Save:
Mgr pmm1;
pmm1.create(size);
auto p = pmm1.allocate_typed<int>();
*p.resolve(pmm1) = 42;
uint32_t saved_offset = p.offset();  // save for reconstruction after load
pmm::save_manager(pmm1, "data.pmm");
pmm1.destroy();

// Load:
Mgr pmm2;
pmm2.create(size);
pmm::load_manager_from_file(pmm2, "data.pmm");
Mgr::pptr<int> q(saved_offset);  // reconstruct from saved granule index
assert(*q.resolve(pmm2) == 42);
```

---

## Breaking Changes Summary

1. **`pptr<T, void>` removed** — `pmm::pptr<T>` with single argument no longer compiles. Use `Mgr::pptr<T>` (manager-bound pptr).
2. **`p.get()`, `*p`, `p->x` removed** — Use `p.resolve(mgr)` instead.
3. **`reallocate_typed()` removed** — Use manual alloc-copy-dealloc pattern.
4. **Singleton `PersistMemoryManager<>` removed** — Use RAII instance `Mgr pmm`.
5. **`pmm::save()` / `pmm::load_from_file()` removed** — Use `pmm::save_manager()` / `pmm::load_manager_from_file()`.
6. **`pmm::get_stats()` / `MemoryStats` removed** — Use individual counter methods.
7. **`validate()` removed from public API** — Use `pmm.is_initialized()`.
8. **`block_data_size_bytes()` removed** — Not in new API.
9. **`PMMConfig` / `DefaultConfig` removed** — Use `manager_configs.h` configs.
10. **`legacy_manager.h` removed** — Include `pmm/pmm_presets.h` instead.

---

## Note on `alloc_block_count()`

Issue #75: `alloc_block_count()` always includes `BlockHeader_0` (the sentinel block at offset 0). After fully deallocating all user blocks, `alloc_block_count() == 1` (not 0).

```cpp
pmm.allocate_typed<int>(n);  // for each block
pmm.deallocate_typed(p);     // for each block
assert(pmm.alloc_block_count() == 1);  // BlockHeader_0 still counted
```
