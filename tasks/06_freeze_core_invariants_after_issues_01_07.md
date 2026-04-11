# Freeze core invariants after issues 01–07

## Purpose

Не переоткрывать старые задачи 01–07, а **зафиксировать их итог в компактной канонической форме**:
в документах, кодовых инвариантах, assert-проверках и тестовых ожиданиях.

Эта задача превращает уже реализованную архитектуру из “набора результатов” в устойчивое основание для следующих работ.

## Why now

Старые 01–07 по смыслу уже закрывают:
- forest model;
- field semantics;
- domain registry;
- persistent dictionary;
- bootstrap;
- free-tree policy;
- verify/repair split.

Но после компактификации нужно проверить, что их результат:
- не размазан по нескольким документам;
- не зависит от исторических текстов;
- не живёт только в голове или в старых issue.

## Deliverables

1. `docs/core_invariants.md` — единый список инвариантов PMM после 01–07.
2. Таблица соответствия:
   - инвариант → canonical doc;
   - инвариант → кодовая точка проверки;
   - инвариант → тест/набор тестов.
3. Минимальные code assertions / validation hooks там, где инвариант пока есть только в docs.
4. Очистка дублирующих описаний в других документах.

## Mandatory invariant groups

Нужно зафиксировать минимум следующие группы:

### A. PMM model boundary
- PMM = persistent address space manager / storage kernel
- PMM не знает `pjson`-семантику
- AVL-forest является first-class abstraction, но не смешивается с верхними слоями

### B. Block and TreeNode semantics
- линейные соседи блока;
- intrusive tree links;
- `weight`;
- `root_offset`;
- `node_type`;
- ограничение “one intrusive tree-slot per block”.

### C. Forest/domain model
- системные домены;
- именованные roots;
- persistent registry;
- роль symbol dictionary.

### D. Bootstrap model
- что обязано существовать после `create()`;
- как это восстанавливается после `load()`;
- какие system structures считаются обязательными.

### E. Free-tree policy
- ordering;
- tie-breaker;
- связь с линейной геометрией ПАП;
- роль `weight` в free-tree.

### F. Verify/repair contract
- verify не чинит;
- repair не маскируется под verify/load;
- corruption фиксируется диагностически, а не ретушируется.

## Work breakdown

### A. Собрать инварианты в один документ
Нельзя оставлять инвариант “размазанным” по нескольким документам без главного источника.

### B. Привязать каждый инвариант к коду
Для каждого важного инварианта указать:
- где он должен проверяться;
- где допустим debug assert;
- где нужна runtime validation;
- где достаточно теста.

### C. Убрать противоречия
Если один документ говорит одно, а код/другой документ — другое, задача не считается закрытой.

### D. Проверить, что 08–10 строятся поверх этих инвариантов
Validation, tests и storage seams не должны переопределять базовые semantics.

## Acceptance criteria

- Существует один compact canonical document со списком core invariants.
- Для каждого инварианта есть связка doc → code/test.
- Старые 01–07 не нужно перечитывать как обязательную часть понимания текущего PMM.
- Документы и код не спорят друг с другом по базовой модели.
- Задачи 07–10 опираются на этот frozen invariant set, а не переизобретают его.

## Out of scope

- новые домены;
- новые контейнеры;
- журналирование;
- `pjson_db`-семантика.

## Dependencies

Задачи 01–05.
