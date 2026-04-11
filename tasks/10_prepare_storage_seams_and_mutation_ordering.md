# Prepare storage seams and mutation ordering

## Purpose

Подготовить PMM к следующему уровню развития не через преждевременную реализацию journaling/crypto,
а через **правильные seams и mutation ordering rules**.

Это завершающая задача пакета: она должна опереться уже на компактный repo, замороженные инварианты,
жёсткую validation model и тестируемый recovery contract.

## Why now

Если storage seams и crash-consistency-friendly ordering не зафиксировать сейчас, потом придётся
ломать уже вычищенную архитектуру:
- registry/dictionary updates;
- bootstrap assumptions;
- free-tree updates;
- image persistence model.

## Deliverables

1. `docs/storage_seams.md` — главный design doc по storage seams.
2. `docs/mutation_ordering.md` — правила порядка обновлений.
3. Явный список seam points для:
   - image encryption;
   - block/object payload encryption hooks;
   - compression hooks;
   - partial-write/crash-consistency support.
4. Минимальный contract для верхнего journal/snapshot слоя.

## Mandatory scope

### A. Seam points
Нужно определить расширяемые точки для:
- whole-image processing;
- per-block/payload processing;
- save/load pipeline;
- online vs backup-oriented modes.

### B. Mutation ordering
Нужно отдельно описать порядок обновлений минимум для:
- root updates;
- registry updates;
- dictionary updates;
- free-tree updates;
- header/state transitions, от которых зависит recoverability.

### C. Crash-oriented reasoning
Для каждой критической мутации нужно указать:
- какой partial state допустим;
- какой partial state считается corruption;
- что увидит verify после interrupted update;
- что repair может и не может делать.

### D. Separation from upper layers
Нужно формально закрепить, что seams:
- не втаскивают в PMM `pjson` semantics;
- не превращают PMM в transaction engine;
- не сливают PMM с `pjson_db` или AVM.

## Work breakdown

### A. Пересобрать текущие storage-related docs
Свести в один активный design surface всё, что сейчас размазано по:
- atomic writes;
- encryption/compression notes;
- recovery docs;
- bootstrap assumptions.

### B. Ввести operating modes
Нужно различить как минимум:
- normal persistent image operation;
- backup/export mode;
- future journal-assisted mode.

### C. Определить seam granularity
Явно решить:
- что обрабатывается на уровне image;
- что на уровне block payload;
- какие metadata должны оставаться доступными для structural recovery;
- что нельзя прятать/сжимать без изменения базовой модели.

### D. Проверить влияние на 08–09
Storage seams не должны ломать:
- pointer validation assumptions;
- verify/repair diagnostics;
- reload/corruption test matrix.

## Acceptance criteria

- Существует один canonical doc по storage seams.
- Для критических обновлений описан mutation ordering.
- Crash consistency рассмотрена на уровне registry/dictionary/free-tree/root updates.
- PMM остаётся type-erased storage kernel и не втягивает в себя `pjson` semantics.
- Результат готовит почву для будущего journal/snapshot слоя, не внедряя его преждевременно.

## Out of scope

- полная реализация transaction engine;
- полноценное object-level encryption API приложения;
- sync protocol;
- `pjson_db` business semantics.

## Dependencies

Задачи 06–09.
