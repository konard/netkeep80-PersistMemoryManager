# Compact documentation to a canonical set

## Purpose

Сжать документацию до **короткого канонического набора**, где каждая тема описана ровно в одном активном документе.

Задача не в том, чтобы написать ещё документы, а в том, чтобы убрать конкурирующие, фазовые и исторические тексты из основной поверхности репозитория.

## Why now

В репозитории уже накопились:
- канонические архитектурные документы;
- фазовые документы;
- общий план с историей версий и issue;
- концептуальные дубли;
- материалы, полезные при разработке, но не годящиеся на роль активного канона.

Именно документация сейчас создаёт значительную часть ощущения “раздутости”.

## Deliverables

1. Новый `docs/README.md` или `docs/index.md` — единая точка входа в docs.
2. Canonical set документов, который остаётся в активной навигации.
3. Список документов, которые:
   - переносятся в `docs/archive/`,
   - или удаляются после извлечения полезного содержания.
4. Обновлённый `README.md`, который ссылается только на canonical set.

## Target canonical set

В активной навигации должны остаться только документы, которые действительно нужны для текущего и целевого состояния PMM.

Рекомендуемый набор:
- `README.md`
- `docs/pmm_avl_forest.md`
- `docs/block_and_treenode_semantics.md`
- `docs/bootstrap.md`
- `docs/free_tree_forest_policy.md`
- `docs/recovery.md`
- `docs/thread_safety.md`
- `docs/api_reference.md`
- `docs/storage_seams.md` или переименованный эквивалент
- `docs/repository_shape.md` / `docs/deletion_policy.md` после задачи 01

## Candidate archive/delete set

Проверить и принять решение минимум по следующим документам:
- `docs/plan.md`
- `docs/PMM_AVL_Forest_Concept.md`
- `docs/avl_forest_analysis_ru.md`
- `docs/phase1_safety.md`
- `docs/phase2_persistence.md`
- `docs/phase3_types.md`
- `docs/phase4_api.md`
- `docs/phase5_testing.md`
- `docs/phase6_documentation.md`
- `docs/phase7_4_encryption_compression.md`
- `docs/plan4BinDiffSynchronizer.md`

Правило: если документ не описывает активный канон, он не должен быть в основном маршруте чтения.

## Work breakdown

### A. Разделить docs на три класса
- `canonical`
- `supporting`
- `historical`

### B. Извлечь нормативное содержание
Перед удалением/архивацией исторических документов нужно вынести из них в канон только то, что реально нужно как:
- инвариант;
- контракт;
- ограничение дизайна;
- актуальное целевое состояние.

### C. Убрать phase/history narration
Из канонических документов удалить:
- фазы;
- ссылки на старые issue;
- версии;
- story-like sections “что было сделано”.

### D. Ввести единый docs index
Нужен один документ-навигатор, где:
- есть краткая карта документации;
- у каждого документа одна роль;
- нет дублей.

### E. Обновить README
README должен ссылаться только на active docs и не вести пользователя в исторические пласты.

## Acceptance criteria

- У docs есть один индекс и один официальный маршрут чтения.
- Для каждой крупной темы существует ровно один canonical doc.
- Исторические phase-документы не участвуют в основной навигации.
- `README.md` больше не ведёт в фазовую/историческую документацию.
- Канонические документы описывают только текущее и целевое состояние.

## Out of scope

- переписывание тестов;
- удаление compatibility-кода;
- API clean-up;
- storage-level implementation.

## Dependencies

Задача 01.
