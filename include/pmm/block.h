/**
 * @file pmm/block.h
 * @brief Block<AddressTraits> — заголовок блока памяти (Issue #136).
 *
 * Issue #136: перенос AVL-ссылок из заголовка блока в область данных свободного блока.
 *
 * Архитектура (Issue #136):
 *   Block<A> = LinkedListNode<A> (prev_offset + next_offset, 8 байт) + TreeNode<A>
 *              (weight + root_offset + avl_height + node_type, 12 байт) +
 *              user_tree (user_left + user_right + user_parent, 12 байт) = 32 байта (2 гранулы).
 *
 * Поля `left_offset`, `right_offset`, `parent_offset` перемещены в FreeBlockData<A> —
 * структуру, хранящуюся в области данных свободного блока (по адресу блок + sizeof(Block<A>)).
 * Поле `prev_offset` остаётся в заголовке блока (LinkedListNode<A>) для O(1) доступа
 * при слиянии блоков (coalescing). Без этого каждое освобождение блока требовало бы
 * O(n) сканирования от начала списка блоков.
 *
 * Пользовательское AVL-дерево (Issue #136):
 *   Для занятых блоков поля user_left, user_right, user_parent хранятся в заголовке
 *   (байты [20..31]) и не пересекаются с пользовательскими данными (начинаются с [32..]).
 *   Для свободных блоков AVL-ссылки менеджера хранятся в FreeBlockData<A> ([32..43]).
 *
 * Раскладка полей при AddressTraitsT = DefaultAddressTraits (uint32_t, 16):
 *   [0..3]   LinkedListNode<A>: prev_offset (4)
 *   [4..7]   LinkedListNode<A>: next_offset (4)
 *   [8..11]  TreeNode<A>:       weight (4)
 *   [12..15] TreeNode<A>:       root_offset (4)
 *   [16..17] TreeNode<A>:       avl_height (2)
 *   [18..19] TreeNode<A>:       node_type (2)
 *   [20..23] user_left_offset (4)   — для пользовательских AVL-деревьев (Issue #136)
 *   [24..27] user_right_offset (4)  — для пользовательских AVL-деревьев (Issue #136)
 *   [28..31] user_parent_offset (4) — для пользовательских AVL-деревьев (Issue #136)
 *
 * Свободный блок дополнительно хранит в своей области данных FreeBlockData<A>:
 *   [32..43] FreeBlockData<A>: left_offset (4), right_offset (4), parent_offset (4)
 *
 * Размер и выравнивание:
 *   sizeof(Block<DefaultAddressTraits>) == 32 байта (2 гранулы по 16 байт).
 *   Подтверждено через static_assert в types.h.
 *
 * @see free_block_data.h — FreeBlockData<A> (хранится в области данных свободного блока)
 * @see plan_issue87.md §5 «Фаза 3: Block — блок как составной тип»
 * @version 0.7 (Issue #136 — AVL refs moved to FreeBlockData; prev_offset in header; user tree in header)
 */

#pragma once

#include "pmm/address_traits.h"
#include "pmm/free_block_data.h"
#include "pmm/linked_list_node.h"
#include "pmm/tree_node.h"

#include <cstdint>

namespace pmm
{

/**
 * @brief Заголовок блока памяти ПАП (Issue #136).
 *
 * @tparam AddressTraitsT  Traits адресного пространства (из address_traits.h).
 *                         Определяет тип индексных полей.
 *
 * Наследует LinkedListNode<AddressTraitsT> (prev_offset, next_offset) и
 * TreeNode<AddressTraitsT> (weight, root_offset, avl_height, node_type).
 * Дополнен пользовательскими AVL-полями до 32 байт для выравнивания данных по границе гранулы.
 *
 * Поля `left_offset`, `right_offset`, `parent_offset` (для менеджера свободных блоков)
 * хранятся в FreeBlockData<A> в области данных свободного блока (по адресу блок + sizeof(Block<A>)).
 *
 * Поля `user_left_offset`, `user_right_offset`, `user_parent_offset` (для пользовательских деревьев)
 * хранятся в заголовке блока (байты [20..31]) и доступны для занятых блоков.
 *
 * При AddressTraitsT = DefaultAddressTraits (uint32_t, 16):
 *   sizeof(Block<DefaultAddressTraits>) == 32 байта (2 гранулы)
 *
 * Доступ к полям через наследование:
 *   - prev_offset, next_offset                — через LinkedListNode<A>
 *   - weight, root_offset, avl_height,
 *     node_type                               — через TreeNode<A>
 *   - user_left_offset, user_right_offset,
 *     user_parent_offset                      — прямые поля Block<A> (пользовательское AVL-дерево)
 *   - left_offset, right_offset,
 *     parent_offset                           — через FreeBlockData<A>
 *                                               (только для свободных блоков, менеджер памяти)
 */
template <typename AddressTraitsT> struct Block : LinkedListNode<AddressTraitsT>, TreeNode<AddressTraitsT>
{
    using address_traits = AddressTraitsT;
    using index_type     = typename AddressTraitsT::index_type;

    // Issue #136: User AVL-tree pointers stored in block header (bytes [20..31]).
    // These are separate from FreeBlockData (manager's internal free-block AVL tree at +32).
    // For allocated blocks: these fields hold user tree pointers (left/right/parent of user's tree).
    // For free blocks: these fields are unused (manager uses FreeBlockData at +32 instead).
    // LinkedListNode<A> (8) + TreeNode<A> (12) = 20 bytes. Pad to 32 bytes with user tree fields.
    index_type user_left_offset;   ///< User AVL-tree: left child (or no_block if null)
    index_type user_right_offset;  ///< User AVL-tree: right child (or no_block if null)
    index_type user_parent_offset; ///< User AVL-tree: parent (or no_block if null)

  public:
    /**
     * @brief Получить доступ к данным свободного блока (FreeBlockData<A>).
     *
     * @warning Вызывать только для свободных блоков (weight == 0, root_offset == 0).
     *          Для занятых блоков эта область является пользовательскими данными.
     *
     * @return Ссылка на FreeBlockData<A>, хранящийся в области данных.
     */
    FreeBlockData<AddressTraitsT>& free_data() noexcept
    {
        return *reinterpret_cast<FreeBlockData<AddressTraitsT>*>( reinterpret_cast<std::uint8_t*>( this ) +
                                                                  sizeof( Block<AddressTraitsT> ) );
    }

    const FreeBlockData<AddressTraitsT>& free_data() const noexcept
    {
        return *reinterpret_cast<const FreeBlockData<AddressTraitsT>*>( reinterpret_cast<const std::uint8_t*>( this ) +
                                                                        sizeof( Block<AddressTraitsT> ) );
    }
};

} // namespace pmm
