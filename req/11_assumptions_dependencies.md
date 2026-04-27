# 11. Предположения и зависимости (`ASM`, `DEP`)

Предположения и зависимости фиксируют внешние условия, которые влияют на реализацию и эксплуатацию требований.

## Предположения

| ID | Предположение | Статус | Основание |
|---|---|---|---|
| ASM-001 | Клиентский код не хранит raw pointers как persistent references. | Recovered | README, `docs/architecture.md` |
| ASM-002 | Клиентский код выбирает address traits, достаточные для максимального размера PAP. | Recovered | README, `docs/architecture.md` |
| ASM-003 | Клиентский код выбирает lock policy согласно модели конкурентного доступа. | Recovered | README, `docs/thread_safety.md` |
| ASM-004 | Пользовательские типы, создаваемые через `create_typed`, совместимы с nothrow lifecycle constraints. | Recovered | README |
| ASM-005 | Для абсолютного контроля persistent type identity в `pmap` пользователь может специализировать `pmap_type_identity<T>` фиксированным ASCII-tag. | Recovered | `docs/architecture.md` |
| ASM-006 | Верхние слои отвечают за schema migration собственных объектов, PMM отвечает только за свой image/layout/kernel metadata. | Recovered | `docs/pmm_target_model.md`, `docs/validation_model.md` |

## Зависимости

| ID | Зависимость | Статус | Основание |
|---|---|---|---|
| DEP-001 | Для build/test workflow нужны CMake, поддерживаемый C++20 compiler и CTest. | Recovered | README |
| DEP-002 | Для optional demo нужны GLFW, Dear ImGui и OpenGL. | Recovered | README |
| DEP-003 | Для benchmark workflow нужен Google Benchmark target, включаемый отдельной CMake option. | Recovered | README |
| DEP-004 | Для file-backed persistence на платформах с mmap требуется поддержка соответствующих OS primitives. | Recovered | `docs/architecture.md` |
| DEP-005 | Для многопоточного режима требуется доступный `std::shared_mutex`. | Recovered | `docs/thread_safety.md` |
