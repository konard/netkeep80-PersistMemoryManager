# Reduce internal code volume and simplify structure

## Purpose

Уменьшить объём внутреннего кода не косметически, а структурно:
- сократить дублирование;
- убрать лишние слои;
- сделать один authoritative path на одну capability;
- выровнять внутреннюю организацию под уже принятую модель PMM.

## Why now

После удаления historical/docs noise и legacy paths нужно отдельно пройтись по живому коду:
иногда он уже не “мёртвый”, но всё ещё избыточен по форме.

Задача должна ответить на вопрос:  
**“Какие строки реально нужны для текущих функций PMM, а какие — следствие старой структуры?”**

## Deliverables

1. `docs/internal_structure.md` — короткая карта внутренних модулей и их ответственности.
2. `docs/code_reduction_report.md` — before/after по файлам, LOC и устранённым слоям.
3. Серия изменений, в которых:
   - дублирующие helper-ветки сведены;
   - внутренние API укорочены;
   - обязанности файлов и namespace-слоёв стали однозначными.

## Work breakdown

### A. Сделать карту внутренних подсистем
Разложить код минимум на такие классы ответственности:
- allocator / linear PAP mechanics;
- block/tree semantics;
- forest policies;
- registry/dictionary/bootstrap;
- validation/recovery;
- public container surface;
- storage backends / policies.

Для каждой области определить:
- authoritative file(s);
- allowed entry points;
- helper files;
- что дублирует что.

### B. Найти structural duplication
Искать не только literal copy-paste, но и:
- одинаковые проверки, написанные в нескольких местах;
- почти одинаковые helper-функции;
- несколько “тонких” прослоек между одними и теми же сущностями;
- вложенные generic abstractions без реальной отдачи.

### C. Ввести правило one capability → one implementation path
Для каждой capability должен остаться один главный путь:
- одна точка low-level validation;
- одна точка tree-link integrity logic;
- одна точка root/domain lookup;
- одна точка mutation ordering per subsystem.

### D. Изолировать internals
Привести внутренности к явному виду:
- публичное API — минимально;
- internal helpers — в `detail`;
- private contracts не изображают отдельный продуктовый слой.

### E. Проверить single_include strategy
Убедиться, что:
- modular headers являются источником истины;
- single-header artefacts только генерируются/собираются из них;
- не существует независимого ручного сопровождения двух реализаций.

### F. Зафиксировать результат цифрами
Нужен отчёт:
- сколько файлов упрощено;
- какие слои удалены;
- сколько строк ушло;
- какие дубли сведены.

## Acceptance criteria

- Есть явная карта внутренней структуры.
- Для каждой крупной capability определён один authoritative implementation path.
- Удалены или сведены structural duplicates.
- Internal API короче и понятнее, чем до задачи.
- Есть before/after report по коду, а не только субъективное ощущение “стало лучше”.

## Out of scope

- новая бизнес-функциональность;
- семантика `pjson`;
- финальная тестовая матрица;
- полная transactional model.

## Dependencies

Задачи 01–04.
