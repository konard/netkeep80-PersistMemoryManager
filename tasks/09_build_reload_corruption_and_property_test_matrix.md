# Build reload, corruption and property test matrix

## Purpose

Закрыть PMM систематической матрицей тестов, которая проверяет не только allocator happy-path,
а устойчивость persistent image как forest-driven storage kernel.

## Why now

После компактификации и ужесточения validation/recovery именно тесты должны стать главным доказательством,
что репозиторий стал не просто “чище”, а действительно надёжнее.

Старая задача 09 сохраняется по смыслу, но теперь должна опираться на:
- frozen invariants;
- verify/repair contract;
- unified validation model.

## Deliverables

1. `docs/test_matrix.md` — полная карта тестовых направлений.
2. Набор test suites, сгруппированных по категориям.
3. Явное покрытие verify vs repair vs load behavior.
4. Property/generative scenarios для allocator/forest/image reload cycles.

## Test groups

### A. Bootstrap tests
Проверить, что `create()` создаёт корректный минимальный image:
- manager header;
- free-tree root;
- domain registry;
- symbol dictionary;
- system domain bindings.

### B. Reload / relocation tests
Проверить:
- reload того же image;
- reload при другом base address;
- сохранность root bindings;
- сохранность dictionary и registry после reload.

### C. Structural invariant tests
Проверить:
- linked-list topology;
- AVL balance;
- tree ownership;
- domain membership;
- bootstrap-required structures.

### D. Corruption tests
Нужно иметь отдельные сценарии на:
- битые tree links;
- битые parent/root bindings;
- битый registry;
- битый dictionary root;
- несогласованные block fields;
- header corruption;
- invalid pointer provenance.

### E. Verify / repair behavior
Проверить:
- verify only reports;
- repair делает только разрешённые реконструкции;
- load не маскирует corruption;
- diagnostics отражают реальное действие.

### F. Property / generative tests
Нужны сценарии со случайными последовательностями:
- allocate / deallocate;
- container/domain updates;
- reload cycles;
- corruption injection;
- repeated verify and repair checks.

## Work breakdown

### A. Сначала сделать test matrix как документ
Нельзя начинать писать тесты без карты покрытия.
Для каждой категории нужно указать:
- что именно проверяется;
- какой инвариант покрывается;
- какой режим (load / verify / repair) затронут.

### B. Отделить deterministic corruption tests от fuzz/property tests
Обе группы нужны, но они решают разные задачи:
- deterministic — воспроизводимое доказательство policy;
- generative — поиск неожиданных комбинаций.

### C. Привязать тесты к diagnostics
Тест не должен проверять только “успех/неуспех”.
Где возможно, он должен проверять:
- тип нарушения;
- policy outcome;
- отсутствие hidden repair.

### D. Проверить suite organization
Тесты должны быть разбиты так, чтобы было ясно:
- где bootstrap;
- где reload;
- где corruption;
- где validation;
- где property-based tests.

## Acceptance criteria

- Есть единый test matrix document.
- Есть отдельные suites на bootstrap, reload, corruption, verify/repair.
- Есть покрытие relocation / different base address.
- Есть property/generative tests поверх persistent image scenarios.
- Тесты проверяют не только allocator, но и forest/domain/bootstrap infrastructure.

## Out of scope

- performance benchmarking;
- distributed sync testing;
- VM/execution testing.

## Dependencies

Задачи 06–08.
