# 06. Требования к данным (`DR`)

Требования к данным описывают логическую структуру, хранение, целостность, получение и утилизацию данных.

| ID | Требование к данным | Приоритет | Статус | Основание |
|---|---|---|---|---|
| DR-001 | Управляемая память должна быть линейной областью, разбитой на блоки. | Must | Recovered | `docs/architecture.md` |
| DR-002 | `ManagerHeader` должен храниться в пользовательской области `Block_0`. | Must | Recovered | `docs/architecture.md` |
| DR-003 | `ManagerHeader` должен содержать magic, total size, used size, block counters, free-tree root, image version, granule size, CRC32 и root offset. | Must | Recovered | `docs/architecture.md` |
| DR-004 | Каждый `Block` должен содержать linked-list поля, AVL tree поля, `weight`, `root_offset`, `avl_height`, `node_type`. | Must | Recovered | `docs/architecture.md` |
| DR-005 | Свободный блок должен кодироваться парой `weight == 0` и `root_offset == 0`. | Must | Recovered | `docs/architecture.md`, `docs/validation_model.md` |
| DR-006 | Выделенный блок должен кодироваться `weight > 0` и `root_offset == own_granule_index`. | Must | Recovered | `docs/architecture.md`, `docs/validation_model.md` |
| DR-007 | `pptr<T>` должен хранить granule index размером 1/2/4/8 байт в зависимости от address traits. | Must | Recovered | README, `docs/architecture.md` |
| DR-008 | Image version должен поддерживать legacy migration `0 → 1`; неподдерживаемая версия должна приводить к ошибке unsupported format. | Must | Recovered | `docs/architecture.md` |
| DR-009 | Persistent container nodes должны храниться в PAP-блоках и использовать встроенные `TreeNode` fields как AVL links. | Should | Recovered | `docs/architecture.md` |
| DR-010 | `pmap` type identity должен быть стабильным и не зависеть от compiler-specific spellings вроде `__PRETTY_FUNCTION__`. | Should | Recovered | `docs/architecture.md` |
| DR-011 | Для absolute control over persistent type identity должна быть предусмотрена специализация `pmap_type_identity<T>` с фиксированным ASCII-tag. | Could | Recovered | `docs/architecture.md` |
| DR-012 | File image должен включать достаточно metadata для проверки granule size, total size и layout compatibility при загрузке. | Must | Recovered | README, `docs/architecture.md` |
