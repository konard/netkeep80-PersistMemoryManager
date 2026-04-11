# Stabilize verify / repair and diagnostics boundary

## Purpose

Довести verify/repair до окончательно жёсткого состояния после компактификации:
чтобы это был не просто “разделённый код”, а **чёткий операционный контракт** для пользователя и для дальнейших задач 08–10.

## Why now

Validation, corruption tests и storage seams бессмысленно развивать, пока не зафиксировано:
- что именно считается verify;
- что именно считается repair;
- где разрешена реконструкция;
- где обязана быть остановка;
- как выглядит диагностика нарушения.

## Deliverables

1. `docs/verify_repair_contract.md` — явный operational contract.
2. `docs/diagnostics_taxonomy.md` — типы нарушений и форматы диагностик.
3. Проверка startup/load/recovery путей на отсутствие hidden repair.
4. Обновлённые tests для verify/repair boundary.

## Required contract

### Verify
- только читает и диагностирует;
- ничего не правит;
- возвращает структурированный результат;
- не подменяет собой load recovery.

### Repair
- отдельная явная операция;
- имеет документированную область допустимой реконструкции;
- сообщает, что именно было исправлено;
- не называется “тихой загрузкой”.

### Load
- не выполняет скрытый repair;
- может выполнять только минимально допустимые проверки/инициализацию, не маскирующие corruption;
- на серьёзном нарушении останавливается предсказуемо.

## Work breakdown

### A. Разобрать все recovery-related entry points
Проверить:
- `load()`
- startup sequence
- rebuild linked list
- rebuild free-tree
- любые path-ы, делающие “self-healing” или state normalization

### B. Ввести классификацию нарушений
Минимум:
- pointer/header corruption
- linked-list corruption
- free-tree corruption
- registry corruption
- dictionary corruption
- bootstrap structure absence/inconsistency

### C. Определить границы допустимого repair
Для каждого типа нарушения нужно явно сказать:
- repair allowed
- repair forbidden
- verify only
- hard stop

### D. Свести diagnostics к одной форме
Диагностика должна содержать:
- тип нарушения;
- affected object/block/domain/root;
- severity;
- action taken / no action.

### E. Очистить docs и code paths
После задачи в коде и документации не должно оставаться двусмысленных формулировок вроде:
- “recover”
- “normalize”
- “fix if needed”
без явного указания, verify это или repair.

## Acceptance criteria

- Verify, repair и load имеют непересекающиеся роли.
- Нет hidden repair на обычных путях загрузки.
- Для каждого класса нарушений существует ясная policy.
- Диагностика структурирована и единообразна.
- Задачи 08 и 09 могут напрямую опираться на этот контракт.

## Out of scope

- полноценный transaction log;
- distributed recovery;
- `pjson_db` repair semantics.

## Dependencies

Задача 06.
