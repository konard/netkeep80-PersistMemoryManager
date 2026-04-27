# 10. Системные требования (`SYS`)

Системное требование — требование верхнего уровня к продукту, состоящему из подсистем или взаимодействующих компонентов.

| ID | Системное требование | Приоритет | Статус | Основание |
|---|---|---|---|---|
| SYS-001 | PMM должен выступать нижним storage-kernel слоем для систем, которым нужно persistent address space. | Must | Recovered | `docs/pmm_target_model.md` |
| SYS-002 | Верхние слои должны отвечать за payload schema, business logic, query semantics и application format. | Must | Recovered | README, `docs/pmm_target_model.md` |
| SYS-003 | PMM должен быть пригоден для heap-backed, static embedded и mmap/file-backed сценариев. | Should | Recovered | README, `docs/architecture.md` |
| SYS-004 | PMM должен обеспечивать reusable substrate для intrusive persistent structures, включая free-tree, symbol/string intern forest и typed persistent maps. | Should | Recovered | README, `docs/architecture.md` |
| SYS-005 | PMM должен позволять нескольким независимым persistent heaps сосуществовать в одном процессе без смешения типов указателей. | Should | Recovered | `docs/architecture.md` |
| SYS-006 | PMM должен интегрироваться в CMake-based проекты как include-only библиотека без отдельной runtime-сборки ядра. | Should | Recovered | README |
| SYS-007 | PMM должен быть пригоден как слой для дальнейшего `pjson_db`, но не должен сам становиться database engine. | Must | Recovered | README, `docs/pmm_target_model.md` |
