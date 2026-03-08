/**
 * @file pmm/free_block_data.h
 * @brief FreeBlockData<AddressTraits> — AVL-данные свободного блока, вложенные в область данных (Issue #136).
 *
 * Реализует идею переноса AVL-ссылок из заголовка блока в область данных свободного блока (Issue #136).
 *
 * Проблема:
 *   В предыдущей архитектуре Block<A> включал TreeNode<A> с полями left_offset, right_offset,
 *   parent_offset как часть заголовка. Эти поля нужны только для свободных блоков (находящихся
 *   в AVL-дереве). Для занятых блоков они были мёртвым весом.
 *
 * Решение (Issue #136):
 *   Перенести left_offset, right_offset, parent_offset из заголовка блока
 *   в область данных свободного блока. Это позволяет:
 *     - Устранить избыточные AVL-ссылки в каждом занятом блоке
 *     - Сохранить O(1) доступ к AVL-ссылкам для свободных блоков
 *
 * Примечание: `prev_offset` остаётся в заголовке LinkedListNode<A> для O(1) доступа
 * при слиянии блоков (coalescing). Без prev_offset в заголовке каждое освобождение блока
 * потребовало бы O(n) сканирования от начала списка.
 *
 * Раскладка памяти (DefaultAddressTraits):
 *
 *   Занятый блок:
 *     [0..3]   LinkedListNode<A>: prev_offset(4)
 *     [4..7]   LinkedListNode<A>: next_offset(4)
 *     [8..11]  TreeNode<A>:       weight(4)
 *     [12..15] TreeNode<A>:       root_offset(4)
 *     [16..17] TreeNode<A>:       avl_height(2)
 *     [18..19] TreeNode<A>:       node_type(2)
 *     [20..31] Padding (reserved, sizeof(Block<A>) = 32)
 *     [32..]   Пользовательские данные
 *
 *   Свободный блок (weight==0, root_offset==0):
 *     [0..31]  Block<A> (как у занятого, но weight=0, root_offset=0)
 *     [32..43] FreeBlockData<A>: left_offset(4), right_offset(4), parent_offset(4)
 *     [44..]   Не используется
 *
 * Гарантии безопасности:
 *   - FreeBlockData доступна только для блоков с weight==0 (свободных)
 *   - Для занятых блоков область данных [32..] не затрагивается
 *   - Минимальный размер блока: sizeof(Block<A>) + kGranuleSize =
 *     32 + 16 = 48 байт (3 гранулы)
 *
 * @see plan_issue136.md «Перенос AVL-ссылок в область данных свободного блока»
 * @version 0.2 (Issue #136 — only left/right/parent in FreeBlockData; prev_offset kept in header)
 */

#pragma once

#include "pmm/address_traits.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace pmm
{

/**
 * @brief AVL-данные свободного блока, вложенные в его область данных (Issue #136).
 *
 * Эта структура хранится по адресу `base + blk_idx * granule_size + sizeof(Block<A>)`
 * только для свободных блоков (weight == 0, root_offset == 0).
 *
 * Поля:
 *   - `left_offset`    — гранульный индекс левого потомка в AVL-дереве
 *   - `right_offset`   — гранульный индекс правого потомка в AVL-дереве
 *   - `parent_offset`  — гранульный индекс родителя в AVL-дереве
 *
 * Примечание: `prev_offset` хранится в LinkedListNode<A> (заголовке блока) для O(1) доступа.
 *
 * @tparam AddressTraitsT  Traits адресного пространства (из address_traits.h).
 */
template <typename AddressTraitsT> struct FreeBlockData
{
    using address_traits = AddressTraitsT;
    using index_type     = typename AddressTraitsT::index_type;

    /// Гранульный индекс левого дочернего узла AVL-дерева (или no_block).
    index_type left_offset;
    /// Гранульный индекс правого дочернего узла AVL-дерева (или no_block).
    index_type right_offset;
    /// Гранульный индекс родительского узла AVL-дерева (или no_block).
    index_type parent_offset;
};

// Layout: FreeBlockData is a standard-layout struct with exactly 3 index_type fields.
static_assert( std::is_standard_layout<pmm::FreeBlockData<pmm::DefaultAddressTraits>>::value,
               "FreeBlockData must be standard-layout (Issue #136)" );

} // namespace pmm
