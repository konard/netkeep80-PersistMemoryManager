# 06. Требования к данным (`DR`)

Требования к данным описывают логическую структуру, хранение, целостность, получение и утилизацию данных.

| ID | Требование к данным | Приоритет | Статус | Основание |
|---|---|---|---|---|
| DR-001 | Управляемая память должна быть линейной областью, разбитой на блоки. | Must | Recovered | `docs/architecture.md` |
| DR-002 | `ManagerHeader` должен храниться в пользовательской области `Block_0`. | Must | Recovered | `docs/architecture.md` |
| DR-003 | `ManagerHeader` должен содержать magic, total size, used size, block counters, free-tree root, image version, granule size, CRC32 и root offset. | Must | Recovered | `docs/architecture.md` |
| DR-004 | Каждый `BlockHeader<AT>` должен содержать в фиксированном порядке: `weight`, `left_offset`, `right_offset`, `parent_offset`, `root_offset`, `prev_offset`, `next_offset`, затем компактные `avl_height` (`std::uint8_t`) и `node_type` (`enum class NodeType : std::uint8_t`) в самом конце заголовка. | Must | Recovered | `include/pmm/block_header.h`, `docs/architecture.md` |
| DR-005 | Состояние блока определяется исключительно полем `node_type`. Свободный блок имеет `node_type == NodeType::Free`; никакие косвенные проверки вида `weight == 0` использоваться не должны. Для свободного блока `root_offset == 0`. | Must | Recovered | `include/pmm/block_header.h`, `include/pmm/block_state.h` |
| DR-006 | Выделенный блок должен иметь `node_type` из множества `is_allocated(NodeType)` и `root_offset == own_granule_index`. | Must | Recovered | `include/pmm/block_header.h`, `include/pmm/block_state.h` |
| DR-013 | `weight` интерпретируется как кэш размера блока в гранулах. Для свободного блока — полный размер блока (header + payload). Для выделенного блока — размер payload (без header). `weight` обновляется синхронно во всех split/coalesce/extend/shrink/free/allocate путях. | Must | Recovered | `include/pmm/allocator_policy.h`, `include/pmm/free_block_tree.h` |
| DR-014 | `avl_height` — компактное `std::uint8_t` поле; `avl_height == 0` означает, что блок не находится в AVL free-tree. | Must | Recovered | `include/pmm/block_header.h`, `include/pmm/block_state.h` |
| DR-015 | `NodeType` — `enum class : std::uint8_t`, перечисляющий все физические/логические типы узлов: `Free`, `ManagerHeader`, `Generic`, `ReadOnlyLocked`, `PStringView`, `PString`, `PArray`, `PMap`, `PPtr`. Перечисление расширяемо: добавление нового persistent object type требует только регистрации его свойств в `is_*`-хелперах. | Must | Recovered | `include/pmm/block_header.h` |
| DR-016 | Свойства узла (свободный/выделенный, изменяемый, удаляемый из ПАП, участвующий в AVL free-tree) выводятся централизованно из `NodeType` через `is_free`, `is_allocated`, `is_mutable`, `can_be_deleted_from_pap`, `participates_in_free_tree`. Allocator и free-tree обязаны использовать только эти хелперы. | Must | Recovered | `include/pmm/block_header.h` |
| DR-017 | AVL free-tree обязан использовать `weight` как ключ размера блока в нормальном пути (insert/remove/find_best_fit/ordering invariant). Вычисление размера через `next_offset - own_idx` в нормальном пути недопустимо. | Must | Recovered | `include/pmm/free_block_tree.h` |
| DR-018 | `is_allocated(NodeType)` должен быть closed-world `switch`-ем по всем известным значениям enum-а. Неизвестное значение `node_type` (повреждённый байт) не должно трактоваться как allocated. `NodeType::Free` не является user/PAP-deletable; путь `deallocate()` обязан проверять `is_allocated(nt) && can_be_deleted_from_pap(nt)`. | Must | Recovered | `include/pmm/block_header.h`, `include/pmm/persist_memory_manager.h` |
| DR-019 | Typed allocation paths (`allocate_typed<T>`, `create_typed<T>`, `reallocate_typed<T>`) обязаны проставлять блоку logical `NodeType` через `node_type_for<T>::value`. Регистрация нового persistent object type выполняется только специализацией `node_type_for<T>` без изменения базовой логики allocator-а. | Must | Recovered | `include/pmm/typed_manager_api.h`, `include/pmm/block_header.h` |
| DR-020 | Verify-режим обязан проверять, что для свободного блока кэшированный `weight` совпадает с физическим span-ом блока, вычисленным по соседям (`next_offset - own_idx` или `total_granules - own_idx`). Расхождение должно классифицироваться как `BlockStateInconsistent`. Для использования в validation/repair предусмотрены отдельные хелперы `physical_block_total_granules` и `cached_block_total_granules`. | Must | Recovered | `include/pmm/allocator_policy.h`, `include/pmm/types.h` |
| DR-007 | `pptr<T>` должен хранить granule index размером 1/2/4/8 байт в зависимости от address traits. | Must | Recovered | README, `docs/architecture.md` |
| DR-008 | Image version должен поддерживать legacy migration `0 → 1`; неподдерживаемая версия должна приводить к ошибке unsupported format. | Must | Recovered | `docs/architecture.md` |
| DR-009 | Persistent container nodes должны храниться в PAP-блоках и использовать встроенные `TreeNode` fields как AVL links. | Should | Recovered | `docs/architecture.md` |
| DR-010 | `pmap` type identity должен быть стабильным и не зависеть от compiler-specific spellings вроде `__PRETTY_FUNCTION__`. | Should | Recovered | `docs/architecture.md` |
| DR-011 | Для absolute control over persistent type identity должна быть предусмотрена специализация `pmap_type_identity<T>` с фиксированным ASCII-tag. | Could | Recovered | `docs/architecture.md` |
| DR-012 | File image должен включать достаточно metadata для проверки granule size, total size и layout compatibility при загрузке. | Must | Recovered | README, `docs/architecture.md` |

## Свойства `NodeType`

| NodeType        | Free | Allocated | Mutable | Deletable from PAP | In AVL free-tree |
|-----------------|:----:|:---------:|:-------:|:------------------:|:----------------:|
| Free            |  Y   |     N     |    Y    |         N          |        Y         |
| ManagerHeader   |  N   |     Y     |    Y    |         N          |        N         |
| Generic         |  N   |     Y     |    Y    |         Y          |        N         |
| ReadOnlyLocked  |  N   |     Y     |    N    |         N          |        N         |
| PStringView     |  N   |     Y     |    N    |         Y          |        N         |
| PString         |  N   |     Y     |    Y    |         Y          |        N         |
| PArray          |  N   |     Y     |    Y    |         Y          |        N         |
| PMap            |  N   |     Y     |    Y    |         Y          |        N         |
| PPtr            |  N   |     Y     |    Y    |         Y          |        N         |
