# 09. Ограничения (`CON`)

Ограничение задает пределы выбора вариантов, доступных разработчику при проектировании и реализации продукта.

| ID | Ограничение | Приоритет | Статус | Основание |
|---|---|---|---|---|
| CON-001 | Язык реализации и использования: C++20. | Must | Recovered | README |
| CON-002 | Минимальные build-инструменты: CMake 3.16+ и компилятор GCC 10+, Clang 10+ или MSVC 2019 16.3+. | Must | Recovered | README |
| CON-003 | Библиотека должна оставаться header-only. | Must | Recovered | README, `docs/architecture.md` |
| CON-004 | API менеджера должен быть static; экземпляры менеджера не создаются. | Must | Recovered | `docs/architecture.md` |
| CON-005 | Multiple independent managers должны различаться через `InstanceId`. | Should | Recovered | `docs/architecture.md` |
| CON-006 | PMM не должен включать JSON/schema/database/query/sync semantics. | Must | Recovered | README, `docs/pmm_target_model.md` |
| CON-007 | Pointer arithmetic для `pptr<T>` намеренно отсутствует. | Must | Recovered | README |
| CON-008 | При использовании `StaticStorage` рост памяти невозможен. | Must | Recovered | README preset table |
| CON-009 | При использовании `NoLock` потокобезопасность не обеспечивается. | Must | Recovered | README, `docs/thread_safety.md` |
| CON-010 | `single_include/` генерируется из `include/` и не редактируется вручную. | Should | Recovered | README |
| CON-011 | PMM не должен выполнять валидацию `pjson`/верхнеуровневых объектов; это out of scope. | Must | Recovered | `docs/validation_model.md`, `docs/pmm_target_model.md` |
| CON-012 | `pstringview` blocks могут быть permanently locked и не должны освобождаться обычным `deallocate()`. | Should | Recovered | `docs/architecture.md` |
