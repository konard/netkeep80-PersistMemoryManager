/**
 * @file pmm/allocator_policy.h
 * @brief AllocatorPolicy — политика выделения/освобождения памяти (Issue #87 Phase 6).
 *
 * Параметрически объединяет:
 *   - `FreeBlockTreeT` — политику дерева свободных блоков (insert/remove/find_best_fit)
 *   - `AddressTraitsT` — traits адресного пространства
 *
 * Предоставляет все-статические методы:
 *   - `allocate_from_block()` — выделение из найденного свободного блока
 *   - `coalesce()`            — слияние соседних свободных блоков
 *   - `rebuild_free_tree()`   — перестройка дерева свободных блоков (после load())
 *   - `repair_linked_list()`  — восстановление двухсвязного списка (после load())
 *   - `recompute_counters()`  — пересчёт счётчиков (после load())
 *
 * Issue #97: Методы `allocate_from_block()` и `coalesce()` документированы
 * в терминах автомата состояний из `block_state.h`.
 *
 * Issue #106: Полная интеграция с BlockState machine — все изменения состояния
 * блоков выполняются через методы `block_state.h`, прямые присвоения к полям
 * `size` и `root_offset` заменены на типобезопасные переходы состояний.
 * Используется Block<A> layout вместо legacy BlockHeader layout.
 *
 * Граф состояний блока во время allocate_from_block():
 *   FreeBlock → remove_from_avl → FreeBlockRemovedAVL
 *     → [если split] begin_splitting → SplittingBlock → finalize_split → AllocatedBlock
 *     → [без split]  mark_as_allocated → AllocatedBlock
 *
 * Граф состояний блока во время coalesce():
 *   FreeBlockNotInAVL → begin_coalescing → CoalescingBlock
 *     → [правый сосед свободен] coalesce_with_next
 *     → [левый сосед свободен]  coalesce_with_prev → CoalescingBlock(prv)
 *     → finalize_coalesce → FreeBlock (вставка в AVL)
 *
 * @see plan_issue87.md §5 «Фаза 6: AllocatorPolicy»
 * @see block_state.h — автомат состояний блока (Issue #93, #106)
 * @see free_block_tree.h — концепт FreeBlockTree
 * @version 0.3 (Issue #106 — полная интеграция BlockState machine)
 */

#pragma once

#include "pmm/block_state.h"
#include "pmm/free_block_tree.h"
#include "pmm/types.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace pmm
{

/**
 * @brief Политика выделения и освобождения памяти для ПАП-менеджера.
 *
 * Все методы статические и принимают `base_ptr` и `header*` как контекст.
 *
 * @tparam FreeBlockTreeT   Политика дерева свободных блоков (AvlFreeTree или другая).
 * @tparam AddressTraitsT   Traits адресного пространства.
 */
template <typename FreeBlockTreeT = AvlFreeTree<DefaultAddressTraits>, typename AddressTraitsT = DefaultAddressTraits>
class AllocatorPolicy
{
    static_assert( is_free_block_tree_policy_v<FreeBlockTreeT>,
                   "AllocatorPolicy: FreeBlockTreeT must satisfy FreeBlockTreePolicy" );

  public:
    using address_traits  = AddressTraitsT;
    using free_block_tree = FreeBlockTreeT;
    using index_type      = typename AddressTraitsT::index_type;
    using BlockT          = Block<AddressTraitsT>;

    AllocatorPolicy()                                    = delete;
    AllocatorPolicy( const AllocatorPolicy& )            = delete;
    AllocatorPolicy& operator=( const AllocatorPolicy& ) = delete;

    // ─── Выделение ─────────────────────────────────────────────────────────────

    /**
     * @brief Выделить память из найденного свободного блока.
     *
     * Убирает блок из дерева свободных блоков, при необходимости разбивает его
     * на два: один — под запрошенные данные, второй — остаток (снова свободный).
     *
     * Граф состояний (Issue #97/#106, см. block_state.h):
     *   FreeBlock → FreeBlockRemovedAVL [remove_from_avl]
     *     → [split] SplittingBlock [begin_splitting]
     *               → initialize_new_block + link_new_block + AVL insert
     *               → AllocatedBlock [finalize_split]
     *     → [no split] AllocatedBlock [mark_as_allocated]
     *
     * @param base      Базовый указатель управляемой области.
     * @param hdr       Заголовок менеджера.
     * @param blk_idx   Гранульный индекс свободного блока, из которого выделяется память.
     * @param user_size Размер пользовательских данных в байтах.
     * @return Указатель на пользовательские данные или nullptr.
     */
    static void* allocate_from_block( std::uint8_t* base, detail::ManagerHeader* hdr, std::uint32_t blk_idx,
                                      std::size_t user_size )
    {
        // State: FreeBlock → FreeBlockRemovedAVL [remove_from_avl]
        FreeBlockTreeT::remove( base, hdr, blk_idx );
        FreeBlock<AddressTraitsT>*           fb = FreeBlock<AddressTraitsT>::cast_from_raw( blk_at( base, blk_idx ) );
        FreeBlockRemovedAVL<AddressTraitsT>* removed = fb->remove_from_avl();

        std::uint32_t blk_total_gran = detail::block_total_granules( base, hdr, blk_at( base, blk_idx ) );
        std::uint32_t data_gran      = detail::bytes_to_granules( user_size );
        std::uint32_t needed_gran    = detail::kBlockHeaderGranules + data_gran;
        std::uint32_t min_rem_gran   = detail::kBlockHeaderGranules + 1;
        bool          can_split      = ( blk_total_gran >= needed_gran + min_rem_gran );

        if ( can_split )
        {
            // State: FreeBlockRemovedAVL → SplittingBlock [begin_splitting]
            SplittingBlock<AddressTraitsT>* splitting = removed->begin_splitting();

            std::uint32_t new_idx     = blk_idx + needed_gran;
            void*         new_blk_ptr = blk_at( base, new_idx );

            // SplittingBlock::initialize_new_block — инициализировать новый (remainder) блок
            splitting->initialize_new_block( new_blk_ptr, new_idx, blk_idx );

            // SplittingBlock::link_new_block — обновить связный список
            BlockT* old_next_blk =
                ( splitting->next_offset() != detail::kNoBlock ) ? blk_at( base, splitting->next_offset() ) : nullptr;
            if ( old_next_blk == nullptr && blk_at( base, blk_idx )->next_offset != detail::kNoBlock )
                old_next_blk = blk_at( base, blk_at( base, blk_idx )->next_offset );

            // Determine old_next before linking
            std::uint32_t curr_next = blk_at( base, blk_idx )->next_offset;
            BlockT*       old_next  = ( curr_next != detail::kNoBlock ) ? blk_at( base, curr_next ) : nullptr;

            // Initialize new block in place
            std::memset( new_blk_ptr, 0, sizeof( BlockT ) );
            BlockT* new_blk        = blk_at( base, new_idx );
            new_blk->prev_offset   = blk_idx;
            new_blk->next_offset   = curr_next;
            new_blk->left_offset   = detail::kNoBlock;
            new_blk->right_offset  = detail::kNoBlock;
            new_blk->parent_offset = detail::kNoBlock;
            new_blk->avl_height    = 1;
            new_blk->weight        = 0;
            new_blk->root_offset   = 0;

            // Link new block into linked list
            if ( old_next != nullptr )
                old_next->prev_offset = new_idx;
            else
                hdr->last_block_offset = new_idx;
            blk_at( base, blk_idx )->next_offset = new_idx;

            hdr->block_count++;
            hdr->free_count++;
            hdr->used_size += detail::kBlockHeaderGranules;
            FreeBlockTreeT::insert( base, hdr, new_idx );

            // State: SplittingBlock → AllocatedBlock [finalize_split]
            // Use direct field assignment on the block (block_state finalize_split does the same)
            BlockT* blk        = blk_at( base, blk_idx );
            blk->weight        = data_gran;
            blk->root_offset   = blk_idx;
            blk->left_offset   = detail::kNoBlock;
            blk->right_offset  = detail::kNoBlock;
            blk->parent_offset = detail::kNoBlock;
            blk->avl_height    = 0;
        }
        else
        {
            // State: FreeBlockRemovedAVL → AllocatedBlock [mark_as_allocated]
            AllocatedBlock<AddressTraitsT>* alloc = removed->mark_as_allocated( data_gran, blk_idx );
            (void)alloc; // allocated block pointer obtained via state machine
        }

        hdr->alloc_count++;
        hdr->free_count--;
        hdr->used_size += data_gran;

        // Return user data pointer (after block header)
        return reinterpret_cast<std::uint8_t*>( blk_at( base, blk_idx ) ) + sizeof( BlockT );
    }

    // ─── Освобождение и слияние ────────────────────────────────────────────────

    /**
     * @brief Слить освобождённый блок с соседними свободными блоками.
     *
     * Если следующий или предыдущий блок свободен, объединяет их.
     * Добавляет результирующий свободный блок в дерево.
     *
     * Граф состояний (Issue #97/#106, см. block_state.h):
     *   FreeBlockNotInAVL → CoalescingBlock [begin_coalescing]
     *     → [правый сосед free] coalesce_with_next
     *     → [левый сосед free]  coalesce_with_prev → CoalescingBlock(prv) → finalize_coalesce → FreeBlock
     *     → [нет соседей]       finalize_coalesce → FreeBlock [insert в AVL]
     *
     * @param base     Базовый указатель управляемой области.
     * @param hdr      Заголовок менеджера.
     * @param blk_idx  Гранульный индекс только что освобождённого блока (weight == 0).
     */
    static void coalesce( std::uint8_t* base, detail::ManagerHeader* hdr, std::uint32_t blk_idx )
    {
        // State: FreeBlockNotInAVL → CoalescingBlock [begin_coalescing]
        FreeBlockNotInAVL<AddressTraitsT>* not_avl =
            FreeBlockNotInAVL<AddressTraitsT>::cast_from_raw( blk_at( base, blk_idx ) );
        CoalescingBlock<AddressTraitsT>* coalescing = not_avl->begin_coalescing();

        std::uint32_t b_idx = blk_idx;

        // Слияние с правым соседом
        // State: CoalescingBlock::coalesce_with_next
        std::uint32_t curr_next = blk_at( base, b_idx )->next_offset;
        if ( curr_next != detail::kNoBlock )
        {
            BlockT* nxt = blk_at( base, curr_next );
            if ( nxt->weight == 0 ) // free block
            {
                std::uint32_t nxt_idx     = curr_next;
                std::uint32_t nxt_next    = nxt->next_offset;
                BlockT*       nxt_nxt_blk = ( nxt_next != detail::kNoBlock ) ? blk_at( base, nxt_next ) : nullptr;

                FreeBlockTreeT::remove( base, hdr, nxt_idx );

                // CoalescingBlock::coalesce_with_next
                coalescing->coalesce_with_next( nxt, nxt_nxt_blk, b_idx );

                if ( nxt_nxt_blk == nullptr )
                    hdr->last_block_offset = b_idx;
                else
                    nxt_nxt_blk->prev_offset = b_idx;

                hdr->block_count--;
                hdr->free_count--;
                if ( hdr->used_size >= detail::kBlockHeaderGranules )
                    hdr->used_size -= detail::kBlockHeaderGranules;
            }
        }

        // Слияние с левым соседом
        // State: CoalescingBlock::coalesce_with_prev → результат CoalescingBlock(prv)
        std::uint32_t curr_prev = blk_at( base, b_idx )->prev_offset;
        if ( curr_prev != detail::kNoBlock )
        {
            BlockT* prv = blk_at( base, curr_prev );
            if ( prv->weight == 0 ) // free block
            {
                std::uint32_t prv_idx  = curr_prev;
                std::uint32_t blk_next = blk_at( base, b_idx )->next_offset;
                BlockT*       next_blk = ( blk_next != detail::kNoBlock ) ? blk_at( base, blk_next ) : nullptr;

                FreeBlockTreeT::remove( base, hdr, prv_idx );

                // CoalescingBlock::coalesce_with_prev — current block (blk) is absorbed into prv
                CoalescingBlock<AddressTraitsT>* result_coalescing =
                    coalescing->coalesce_with_prev( prv, next_blk, prv_idx );

                if ( next_blk == nullptr )
                    hdr->last_block_offset = prv_idx;

                hdr->block_count--;
                hdr->free_count--;
                if ( hdr->used_size >= detail::kBlockHeaderGranules )
                    hdr->used_size -= detail::kBlockHeaderGranules;

                // State: CoalescingBlock::finalize_coalesce → FreeBlock; вставка в AVL
                FreeBlock<AddressTraitsT>* fb = result_coalescing->finalize_coalesce();
                (void)fb;
                FreeBlockTreeT::insert( base, hdr, prv_idx );
                return;
            }
        }

        // State: CoalescingBlock::finalize_coalesce → FreeBlock; вставка в AVL
        FreeBlock<AddressTraitsT>* fb = coalescing->finalize_coalesce();
        (void)fb;
        FreeBlockTreeT::insert( base, hdr, b_idx );
    }

    // ─── Восстановление состояния (после load()) ───────────────────────────────

    /**
     * @brief Перестроить дерево свободных блоков.
     *
     * Сбрасывает все AVL-ссылки в блоках и заново вставляет свободные блоки.
     * Вызывается при `load()` после восстановления менеджера из файла.
     * Также вызывает recover_block_state для каждого блока (Issue #106).
     *
     * @param base  Базовый указатель управляемой области.
     * @param hdr   Заголовок менеджера.
     */
    static void rebuild_free_tree( std::uint8_t* base, detail::ManagerHeader* hdr )
    {
        hdr->free_tree_root    = detail::kNoBlock;
        hdr->last_block_offset = detail::kNoBlock;
        std::uint32_t idx      = hdr->first_block_offset;
        while ( idx != detail::kNoBlock )
        {
            BlockT* blk        = blk_at( base, idx );
            blk->left_offset   = detail::kNoBlock;
            blk->right_offset  = detail::kNoBlock;
            blk->parent_offset = detail::kNoBlock;
            blk->avl_height    = 0;

            // Issue #106: recover_block_state — исправить некорректные переходные состояния
            recover_block_state<AddressTraitsT>( blk, idx );

            if ( blk->weight == 0 ) // free block (Block<A> layout: weight field)
                FreeBlockTreeT::insert( base, hdr, idx );
            if ( blk->next_offset == detail::kNoBlock )
                hdr->last_block_offset = idx;
            idx = blk->next_offset;
        }
    }

    /**
     * @brief Восстановить двухсвязный список блоков.
     *
     * Проходит по списку блоков и восстанавливает `prev_offset` у каждого.
     * Вызывается при `load()` (данное поле не персистируется).
     *
     * @param base  Базовый указатель управляемой области.
     * @param hdr   Заголовок менеджера.
     */
    static void repair_linked_list( std::uint8_t* base, detail::ManagerHeader* hdr )
    {
        std::uint32_t idx  = hdr->first_block_offset;
        std::uint32_t prev = detail::kNoBlock;
        while ( idx != detail::kNoBlock )
        {
            if ( detail::idx_to_byte_off( idx ) + sizeof( BlockT ) > hdr->total_size )
                break;
            BlockT* blk      = blk_at( base, idx );
            blk->prev_offset = prev;
            prev             = idx;
            idx              = blk->next_offset;
        }
    }

    /**
     * @brief Пересчитать счётчики блоков и использованного размера.
     *
     * Проходит по всем блокам и обновляет `block_count`, `free_count`,
     * `alloc_count`, `used_size` в заголовке менеджера.
     * Использует Block<A> layout: `weight` вместо `size` (Issue #106).
     *
     * @param base  Базовый указатель управляемой области.
     * @param hdr   Заголовок менеджера.
     */
    static void recompute_counters( std::uint8_t* base, detail::ManagerHeader* hdr )
    {
        std::uint32_t block_count = 0, free_count = 0, alloc_count = 0;
        std::uint32_t used_gran = 0;
        std::uint32_t idx       = hdr->first_block_offset;
        while ( idx != detail::kNoBlock )
        {
            if ( detail::idx_to_byte_off( idx ) + sizeof( BlockT ) > hdr->total_size )
                break;
            BlockT* blk = blk_at( base, idx );
            block_count++;
            used_gran += detail::kBlockHeaderGranules;
            if ( blk->weight > 0 ) // allocated block (Block<A> layout: weight > 0)
            {
                alloc_count++;
                used_gran += blk->weight;
            }
            else
            {
                free_count++;
            }
            idx = blk->next_offset;
        }
        hdr->block_count = block_count;
        hdr->free_count  = free_count;
        hdr->alloc_count = alloc_count;
        hdr->used_size   = used_gran;
    }

  private:
    /// @brief Get Block<A>* by granule index.
    static BlockT* blk_at( std::uint8_t* base, std::uint32_t idx )
    {
        assert( idx != detail::kNoBlock );
        return reinterpret_cast<BlockT*>( base + detail::idx_to_byte_off( idx ) );
    }
};

/// @brief Псевдоним AllocatorPolicy с настройками по умолчанию.
using DefaultAllocatorPolicy = AllocatorPolicy<AvlFreeTree<DefaultAddressTraits>, DefaultAddressTraits>;

} // namespace pmm
