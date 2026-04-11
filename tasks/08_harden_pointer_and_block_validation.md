# Harden pointer and block validation

## Purpose

Сделать pointer/header validation одной из самых жёстких и проверяемых частей PMM.

Эта задача должна закрыть все опасные low-level переходы вида:
- raw ptr → block;
- user-facing pointer → persistent header;
- granule index → internal structure.

## Why now

После фиксации forest/domain/bootstrap semantics ошибка в low-level validation бьёт уже не только по allocator state,
но и по:
- root registry;
- symbol dictionary;
- системным forest domains;
- verify/repair diagnostics.

## Deliverables

1. `docs/validation_model.md` — модель low-level validation.
2. Единый набор helper-функций для:
   - fast-path checks;
   - full verify-level checks.
3. Пересмотр всех путей `raw ptr -> block`, включая `header_from_ptr_t()` и эквиваленты.
4. Набор negative tests на invalid pointer/block scenarios.

## Mandatory checks

### A. Pointer provenance
Проверять:
- принадлежность текущему image/manager;
- ненулевое и допустимое base relation;
- корректность преобразования byte/granule offset.

### B. Address correctness
Проверять:
- granule alignment;
- допустимый диапазон индексов;
- отсутствие переполнения при переводе offset ↔ pointer.

### C. Header integrity
Проверять:
- корректность размера и диапазона блока;
- согласованность `prev_offset` / `next_offset`;
- допустимость block state;
- отсутствие выхода за границы image.

### D. Tree/domain integrity
Где требуется, проверять:
- допустимость `root_offset`;
- допустимость `node_type`;
- корректность parent/left/right ownership;
- принадлежность домену;
- отсутствие ссылок на несуществующий root.

### E. Validation modes
Нужно разделить:
- `cheap` — fast-path для обычного API;
- `full` — verify-level consistency checks.

## Work breakdown

### A. Собрать все conversion paths
Нельзя чинить только один helper и оставить остальные обходные пути старыми.

### B. Выделить единый validation layer
Все low-level проверки должны проходить через понятный набор helper-ов, а не быть рассыпаны по месту использования.

### C. Согласовать с frozen invariants
Validation не может сама придумывать semantics `root_offset`, `node_type` или tree-membership.
Она должна проверять уже зафиксированные правила.

### D. Сделать ошибки явными
Нужны:
- формализованные error categories;
- диагностические коды/статусы;
- воспроизводимые negative tests.

## Acceptance criteria

- Все пути raw/user pointer → block проходят через определённый validation layer.
- Повреждённый или чужой pointer надёжно отклоняется.
- Header corruption отличим от domain/tree corruption.
- Fast-path и full verify-path формально разделены.
- Есть негативные тесты на invalid pointer, bad alignment, bad index, broken header, wrong domain/root.

## Out of scope

- восстановление image;
- `pjson` validation;
- object-level semantics верхних слоёв.

## Dependencies

Задачи 06–07.
