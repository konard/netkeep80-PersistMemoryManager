# 08. Атрибуты качества и нефункциональные требования (`QA`)

Атрибут качества — вид нефункционального требования, описывающий характеристику сервиса или производительности продукта.

| ID | Атрибут качества / НФТ | Категория | Приоритет | Статус | Основание |
|---|---|---|---|---|---|
| QA-REL-001 | PMM должен предотвращать silent corruption при raw-pointer/block conversions через централизованную валидацию. | Reliability | Must | Recovered | `docs/validation_model.md` |
| QA-REL-002 | `verify()` должен выполнять read-only диагностику без ремонта образа. | Reliability | Must | Recovered | README, `docs/validation_model.md` |
| QA-REC-001 | `load()` должен восстанавливать служебные структуры там, где это документировано. | Recoverability | Must | Recovered | README, `docs/architecture.md` |
| QA-PERF-001 | Поиск best-fit free block должен выполняться через AVL tree с логарифмической сложностью. | Performance | Must | Recovered | README, `docs/architecture.md` |
| QA-PERF-002 | Cheap validation fast path должен использовать O(1)-проверки без обхода linked list. | Performance | Should | Recovered | `docs/validation_model.md` |
| QA-MEM-001 | Размер `pptr<T>` должен минимизироваться через выбор address traits. | Memory efficiency | Should | Recovered | README, `docs/architecture.md` |
| QA-PORT-001 | Persistent image должен оставаться usable после загрузки по другому базовому адресу. | Portability | Must | Recovered | README, `docs/architecture.md` |
| QA-THREAD-001 | В многопоточном режиме read operations должны использовать shared lock, write operations — unique lock. | Thread safety | Must | Recovered | README, `docs/thread_safety.md` |
| QA-THREAD-002 | В `NoLock` режиме блокировки должны быть no-op, а использование должно ограничиваться single-threaded сценарием. | Thread safety | Must | Recovered | README, `docs/thread_safety.md` |
| QA-MAINT-001 | Архитектура должна сохранять PMM как малый kernel с ограниченной поверхностью, а не расширяемый application framework. | Maintainability | Should | Recovered | `docs/pmm_target_model.md` |
| QA-MAINT-002 | Дублирование AVL-логики между контейнерами и free-tree должно быть устранено/минимизировано через shared AVL implementation. | Maintainability | Should | Recovered | `docs/architecture.md` |
| QA-DIAG-001 | Диагностика должна различать pointer provenance, address correctness и header integrity failure categories. | Diagnosability | Should | Recovered | `docs/validation_model.md` |
| QA-COMPAT-001 | Загрузка должна отвергать unsupported image versions и несовместимые granule sizes. | Compatibility | Must | Recovered | `docs/architecture.md` |
| QA-TEST-001 | Изменения должны проверяться через CMake/CTest и regression test suite. | Testability | Should | Recovered | README |
