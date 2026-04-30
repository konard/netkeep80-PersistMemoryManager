# 12. Критерии приемки (`AC`)

Критерии приемки добавлены как практический слой поверх восстановленных требований. Это не отдельный тип Вигерса из базовой таблицы, но полезный артефакт для проверки требований.

Каждый критерий оформлен как заголовок уровня `##` с идентификатором в формате `ac-xxx`.

## ac-001

- **Критерий:** После `create(size)` менеджер инициализирован, статистика согласована, есть `Block_0` с `ManagerHeader` и свободный блок для пользовательских аллокаций.
- **Проверяет:** [fr-001](05_functional_requirements.md#fr-001), [dr-002](06_data_requirements.md#dr-002), [dr-003](06_data_requirements.md#dr-003)
- **Тесты:** `tests/test_allocate.cpp`, `tests/test_block_modernization.cpp`

## ac-002

- **Критерий:** После `allocate_typed<T>(n)` возвращенный `pptr<T>` разрешается через `resolve`, а `is_valid_ptr` возвращает `true`.
- **Проверяет:** [fr-005](05_functional_requirements.md#fr-005), [fr-007](05_functional_requirements.md#fr-007), [fr-008](05_functional_requirements.md#fr-008)
- **Тесты:** `tests/test_allocate.cpp`

## ac-003

- **Критерий:** После `deallocate_typed` блок помечается свободным, counters пересчитаны, соседние свободные блоки coalesce при наличии.
- **Проверяет:** [fr-004](05_functional_requirements.md#fr-004), [dr-005](06_data_requirements.md#dr-005), [qa-perf-001](08_quality_attributes.md#qa-perf-001)
- **Тесты:** `tests/test_deallocate.cpp`, `tests/test_coalesce.cpp`

## ac-004

- **Критерий:** Сохраненный image после загрузки по другому base address дает корректное значение ранее сохраненного `pptr` offset.
- **Проверяет:** [br-002](01_business_requirements.md#br-002), [qa-port-001](08_quality_attributes.md#qa-port-001)

## ac-005

- **Критерий:** `verify()` на валидном image не меняет байты image и возвращает отсутствие критических нарушений.
- **Проверяет:** [rule-006](02_business_rules.md#rule-006), [qa-rel-002](08_quality_attributes.md#qa-rel-002)

## ac-006

- **Критерий:** `load(VerifyResult&)` на допустимом image выполняет documented validation/repair и восстанавливает linked list, counters и free tree.
- **Проверяет:** [fr-014](05_functional_requirements.md#fr-014), [qa-rec-001](08_quality_attributes.md#qa-rec-001)

## ac-007

- **Критерий:** Image с неподдерживаемой `image_version` отвергается с диагностикой unsupported format/header corruption.
- **Проверяет:** [dr-008](06_data_requirements.md#dr-008), [qa-compat-001](08_quality_attributes.md#qa-compat-001)

## ac-008

- **Критерий:** В `NoLock` preset код компилируется без runtime-lock overhead; документация явно ограничивает использование single-threaded режимом.
- **Проверяет:** [qa-thread-002](08_quality_attributes.md#qa-thread-002), [con-009](09_constraints.md#con-009)

## ac-009

- **Критерий:** В `SharedMutexLock` preset read operations допускают concurrent shared locking, write operations используют unique locking.
- **Проверяет:** [qa-thread-001](08_quality_attributes.md#qa-thread-001)

## ac-010

- **Критерий:** Попытка использовать raw/foreign/misaligned pointer в API не приводит к silent corruption, а возвращает ошибку/false/null согласно validation model.
- **Проверяет:** [qa-rel-001](08_quality_attributes.md#qa-rel-001), [qa-diag-001](08_quality_attributes.md#qa-diag-001)

## ac-011

- **Критерий:** `single_include/` генерируется скриптом из `include/`; ручное изменение generated surface не требуется для core change.
- **Проверяет:** [rule-005](02_business_rules.md#rule-005), [con-010](09_constraints.md#con-010)

## ac-012

- **Критерий:** Public README/API демонстрирует CMake/CTest workflow и он проходит на поддерживаемых компиляторах.
- **Проверяет:** [con-002](09_constraints.md#con-002), [qa-test-001](08_quality_attributes.md#qa-test-001)
