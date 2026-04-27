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
