# 05. Функциональные требования (`FR`)

Функциональное требование описывает требуемое поведение системы при определенных условиях.

| ID | Функциональное требование | Приоритет | Статус | Основание |
|---|---|---|---|---|
| FR-001 | Менеджер должен создавать PAP заданного размера через `create(std::size_t initial_size)`. | Must | Recovered | README API |
| FR-002 | Менеджер должен загружать существующий PAP через `load(VerifyResult&)`. | Must | Recovered | README API |
| FR-003 | Менеджер должен освобождать runtime-состояние через `destroy()`. | Must | Recovered | README API |
| FR-004 | Менеджер должен выделять память через `allocate(user_size)` и освобождать через `deallocate(ptr)`. | Must | Recovered | README API |
| FR-005 | Менеджер должен поддерживать `allocate_typed<T>`, `deallocate_typed<T>`, `create_typed<T>`, `destroy_typed<T>` и `reallocate_typed<T>`. | Must | Recovered | README API |
| FR-006 | `create_typed` должен отказывать/не компилироваться для неподходящих типов согласно nothrow lifecycle constraints. | Must | Recovered | README API |
| FR-007 | Менеджер должен разрешать `pptr<T>` в runtime-указатель через `resolve` и `resolve_at`. | Must | Recovered | README API |
| FR-008 | Менеджер должен проверять валидность persistent pointer через `is_valid_ptr`. | Must | Recovered | README API |
| FR-009 | Менеджер должен предоставлять обход всех блоков и свободных блоков через `for_each_block` и `for_each_free_block`. | Should | Recovered | README API |
| FR-010 | Менеджер должен возвращать статистику: total, used, free size, block count, free block count, allocated block count. | Should | Recovered | README API |
| FR-011 | Менеджер должен поддерживать legacy root pointer: `set_root`, `get_root`. | Should | Recovered | README API |
| FR-012 | Менеджер должен поддерживать named domains: `register_domain`, `register_system_domain`, `has_domain`, `get_domain_root`, `set_domain_root`. | Should | Recovered | README API |
| FR-013 | При нехватке памяти backend с ростом должен расширять буфер, копировать старое содержимое, добавлять/расширять free block и обновлять singleton pointer. | Must | Recovered | `docs/architecture.md` |
| FR-014 | При `load()` библиотека должна валидировать magic, image version, total size и granule size, затем восстанавливать linked list, counters и free tree. | Must | Recovered | README, `docs/architecture.md` |
| FR-015 | Менеджер должен предоставлять `last_error()` и `clear_error()` для диагностики последней ошибки. | Should | Recovered | README API |
| FR-016 | Файловые helper-функции должны сохранять и загружать image с CRC/diagnostics через `pmm/io.h`. | Should | Recovered | README save/load section |
| FR-017 | Persistent string interning должен возвращать один и тот же `pstringview` для одинакового содержимого, где это поддерживается. | Could | Recovered | `docs/architecture.md` |
| FR-018 | `pmap` должен хранить AVL root в type-scoped forest domain, а не в transient runtime state. | Could | Recovered | `docs/architecture.md` |
| FR-019 | `make_guard` должен обеспечивать безопасную очистку typed objects с `free_data()` или `free_all()` перед `destroy_typed()`. | Could | Recovered | README API |
| FR-020 | Конвертация байтов в гранулы должна быть проверяемой и не должна кодировать переполнение значением `0`. | Must | Issue #373 | `pmm/arena_internals.h` |
| FR-021 | Публичный API `allocate(0)` должен отклоняться с `PmmError::InvalidSize`; `allocate(overflowing_size)` — с `PmmError::Overflow`. | Must | Issue #373 | `pmm/persist_memory_manager.h` |
| FR-022 | Путь аллокации должен вычислять `data_gran` ровно один раз через `bytes_to_granules_checked` до выбора свободного блока. | Must | Issue #373 | `pmm/persist_memory_manager.h` |
| FR-023 | Все проверки диапазонов арены должны быть overflow-safe (через `fits_range` / `checked_add` / `checked_mul`). | Must | Issue #373 | `pmm/arena_internals.h` |
| FR-024 | Внутренние операции над физической ареной должны принимать `ArenaView<AT>` или эквивалентную сильно-парную пару `base+header`. | Must | Issue #373 | `pmm/arena_internals.h` |
| FR-025 | Верификация и repair физической цепочки блоков должны завершаться даже на повреждённых образах (детектор циклов). | Must | Issue #373 | `pmm/arena_internals.h` |
| FR-026 | Инициализация менеджера должна быть транзакционной: при сбое `create()` `_initialized` должен оставаться `false`. | Must | Issue #373 | `pmm/persist_memory_manager.h` |
| FR-027 | Расширение хранилища должно использовать единую проверяемую growth-policy, учитывающую grow ratio и max-memory лимит. | Must | Issue #373 | `pmm/arena_internals.h`, `pmm/layout.h` |
| FR-028 | Heap backend должен обеспечивать выравнивание базы по `AT::granule_size` (для `LargeAddressTraits` granule = 64). | Must | Issue #373 | `pmm/heap_storage.h` |
| FR-029 | Typed allocation должна допускать только типы, выравнивание которых представимо granule_size. | Must | Issue #373 | `pmm/typed_manager_api.h` |
