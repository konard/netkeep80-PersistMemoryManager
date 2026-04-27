# 12. Критерии приемки (`AC`)

Критерии приемки добавлены как практический слой поверх восстановленных требований. Это не отдельный тип Вигерса из базовой таблицы, но полезный артефакт для проверки требований.

| ID | Критерий приемки | Проверяет |
|---|---|---|
| AC-001 | После `create(size)` менеджер инициализирован, статистика согласована, есть `Block_0` с `ManagerHeader` и свободный блок для пользовательских аллокаций. | FR-001, DR-002, DR-003 |
| AC-002 | После `allocate_typed<T>(n)` возвращенный `pptr<T>` разрешается через `resolve`, а `is_valid_ptr` возвращает `true`. | FR-005, FR-007, FR-008 |
| AC-003 | После `deallocate_typed` блок помечается свободным, counters пересчитаны, соседние свободные блоки coalesce при наличии. | FR-004, DR-005, QA-PERF-001 |
| AC-004 | Сохраненный image после загрузки по другому base address дает корректное значение ранее сохраненного `pptr` offset. | BR-002, QA-PORT-001 |
| AC-005 | `verify()` на валидном image не меняет байты image и возвращает отсутствие критических нарушений. | RULE-006, QA-REL-002 |
| AC-006 | `load(VerifyResult&)` на допустимом image выполняет documented validation/repair и восстанавливает linked list, counters и free tree. | FR-014, QA-REC-001 |
| AC-007 | Image с неподдерживаемой `image_version` отвергается с диагностикой unsupported format/header corruption. | DR-008, QA-COMPAT-001 |
| AC-008 | В `NoLock` preset код компилируется без runtime-lock overhead; документация явно ограничивает использование single-threaded режимом. | QA-THREAD-002, CON-009 |
| AC-009 | В `SharedMutexLock` preset read operations допускают concurrent shared locking, write operations используют unique locking. | QA-THREAD-001 |
| AC-010 | Попытка использовать raw/foreign/misaligned pointer в API не приводит к silent corruption, а возвращает ошибку/false/null согласно validation model. | QA-REL-001, QA-DIAG-001 |
| AC-011 | `single_include/` генерируется скриптом из `include/`; ручное изменение generated surface не требуется для core change. | RULE-005, CON-010 |
| AC-012 | Public README/API демонстрирует CMake/CTest workflow и он проходит на поддерживаемых компиляторах. | CON-002, QA-TEST-001 |
