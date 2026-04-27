# 07. Требования к внешним интерфейсам (`IF`)

Внешний интерфейс описывает взаимодействие между ПО и пользователем, другой программной системой или устройством.

| ID | Требование к интерфейсу | Приоритет | Статус | Основание |
|---|---|---|---|---|
| IF-001 | Основной внешний интерфейс должен быть C++20 header-only API через `include/pmm/`. | Must | Recovered | README, repository structure |
| IF-002 | Библиотека должна предоставлять single-header варианты в `single_include/pmm/`. | Should | Recovered | README |
| IF-003 | Файловые helper-функции должны находиться в `pmm/io.h`. | Should | Recovered | README |
| IF-004 | Сборка и проверки должны поддерживаться через CMake/CTest. | Should | Recovered | README |
| IF-005 | Storage backend interface должен позволять HeapStorage, StaticStorage и MMapStorage. | Must | Recovered | README, `docs/architecture.md` |
| IF-006 | Конфигурационный интерфейс должен задавать `address_traits`, `storage_backend`, `free_block_tree`, `lock_policy` и optional `logging_policy`. | Must | Recovered | README, `docs/architecture.md` |
| IF-007 | Preset interface должен предоставлять алиасы для типовых сценариев: embedded static, embedded heap, single-threaded heap, multi-threaded heap, industrial DB heap, large DB heap. | Should | Recovered | README |
| IF-008 | Public API должен быть static API manager type; пользователь не должен создавать instance менеджера. | Must | Recovered | `docs/architecture.md` |
| IF-009 | `InstanceId` должен позволять сосуществование нескольких independent managers с одной конфигурацией. | Should | Recovered | `docs/architecture.md` |
| IF-010 | Public diagnostics interface должен экспонировать `VerifyResult`, `PmmError` и related diagnostic structures. | Should | Recovered | README, `docs/validation_model.md` |
| IF-011 | Optional demo interface должен собираться отдельно и не быть обязательной runtime-зависимостью ядра. | Could | Recovered | README |
