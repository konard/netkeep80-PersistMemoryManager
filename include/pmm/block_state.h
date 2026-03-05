/**
 * @file pmm/block_state.h
 * @brief BlockState machine — автомат состояний блока для атомарных операций (Issue #93).
 *
 * Реализует state machine через наследование состояний, где каждое состояние — это
 * наследник блока с приватными членами и доступными методами работы. Каждый метод
 * выполняет один атомарный шаг критической операции и возвращает следующего
 * наследника, соответствующего новому состоянию блока.
 *
 * Корректные состояния:
 *   - FreeBlock        — свободный блок (size=0, root_offset=0, в AVL-дереве)
 *   - AllocatedBlock   — занятый блок (size>0, root_offset=idx, не в AVL)
 *
 * Переходные состояния (только во время операций):
 *   - FreeBlockRemovedAVL    — свободный, удалённый из AVL (перед allocate)
 *   - FreeBlockNotInAVL      — свободный, не в AVL (после deallocate, перед coalesce)
 *   - SplittingBlock         — блок в процессе разбиения
 *   - CoalescingBlock        — блок в процессе слияния
 *
 * Гарантии:
 *   1. Типобезопасность: компилятор запрещает вызов недоступных методов
 *   2. Восстановимость: переходные состояния детектируются при load()
 *   3. Атомарность: каждый метод выполняет один атомарный шаг
 *   4. Завершаемость: цепочка вызовов приводит к корректному состоянию
 *
 * @see docs/atomic_writes.md «Граф состояний блока»
 * @see plan_issue87.md §5 «Фаза 9: BlockState machine»
 * @version 0.1 (Issue #93)
 */

#pragma once

#include "pmm/address_traits.h"
#include "pmm/block.h"
#include "pmm/linked_list_node.h"
#include "pmm/tree_node.h"

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace pmm
{

// Forward declarations
template <typename AddressTraitsT> class FreeBlock;
template <typename AddressTraitsT> class AllocatedBlock;
template <typename AddressTraitsT> class FreeBlockRemovedAVL;
template <typename AddressTraitsT> class FreeBlockNotInAVL;
template <typename AddressTraitsT> class SplittingBlock;
template <typename AddressTraitsT> class CoalescingBlock;

/**
 * @brief Базовый класс блока с защищёнными атрибутами.
 *
 * Все поля приватные для потомков. Доступ только через методы состояний.
 * Бинарно совместим с BlockHeader (НЕ с Block<A>, т.к. они имеют разный layout).
 *
 * Layout BlockHeader (32 bytes):
 *   [0]  size (4)         — занятый размер в гранулах (0 = свободный)
 *   [4]  prev_offset (4)  — предыдущий блок
 *   [8]  next_offset (4)  — следующий блок
 *   [12] left_offset (4)  — левый дочерний узел AVL
 *   [16] right_offset (4) — правый дочерний узел AVL
 *   [20] parent_offset (4)— родительский узел AVL
 *   [24] avl_height (2)   — высота AVL-поддерева
 *   [26] _pad (2)         — padding
 *   [28] root_offset (4)  — идентификатор дерева
 *
 * @tparam AddressTraitsT  Traits адресного пространства.
 */
template <typename AddressTraitsT> class BlockStateBase
{
  public:
    using address_traits = AddressTraitsT;
    using index_type     = typename AddressTraitsT::index_type;

    // Прямое создание запрещено — используйте cast_from_raw()
    BlockStateBase() = delete;

    // Read-only доступ к size (определяет состояние: 0 = свободный, >0 = занятый)
    index_type size() const noexcept { return _size; }

    // Read-only доступ к полям связного списка (не критичны для состояния)
    index_type prev_offset() const noexcept { return _prev_offset; }
    index_type next_offset() const noexcept { return _next_offset; }

    // Read-only доступ к AVL-полям (для диагностики)
    index_type   left_offset() const noexcept { return _left_offset; }
    index_type   right_offset() const noexcept { return _right_offset; }
    index_type   parent_offset() const noexcept { return _parent_offset; }
    std::int16_t avl_height() const noexcept { return _avl_height; }

    // Read-only доступ к root_offset (определяет состояние)
    index_type root_offset() const noexcept { return _root_offset; }

    // Синоним для совместимости (weight == size в контексте state machine)
    index_type weight() const noexcept { return _size; }

    /**
     * @brief Определить, является ли блок свободным (по структурным признакам).
     * @return true если size == 0 и root_offset == 0.
     */
    bool is_free() const noexcept { return _size == 0 && _root_offset == 0; }

    /**
     * @brief Определить, является ли блок занятым (по структурным признакам).
     * @param own_idx Гранульный индекс данного блока.
     * @return true если size > 0 и root_offset == own_idx.
     */
    bool is_allocated( index_type own_idx ) const noexcept { return _size > 0 && _root_offset == own_idx; }

  protected:
    // Поля — в ТОЧНОМ порядке как BlockHeader (бинарная совместимость)
    index_type    _size;          // [0]  = BlockHeader.size
    index_type    _prev_offset;   // [4]  = BlockHeader.prev_offset
    index_type    _next_offset;   // [8]  = BlockHeader.next_offset
    index_type    _left_offset;   // [12] = BlockHeader.left_offset
    index_type    _right_offset;  // [16] = BlockHeader.right_offset
    index_type    _parent_offset; // [20] = BlockHeader.parent_offset
    std::int16_t  _avl_height;    // [24] = BlockHeader.avl_height
    std::uint16_t _pad;           // [26] = BlockHeader._pad
    index_type    _root_offset;   // [28] = BlockHeader.root_offset

    // Внутренние сеттеры для наследников
    void set_size( index_type v ) noexcept { _size = v; }
    void set_prev_offset( index_type v ) noexcept { _prev_offset = v; }
    void set_next_offset( index_type v ) noexcept { _next_offset = v; }
    void set_left_offset( index_type v ) noexcept { _left_offset = v; }
    void set_right_offset( index_type v ) noexcept { _right_offset = v; }
    void set_parent_offset( index_type v ) noexcept { _parent_offset = v; }
    void set_avl_height( std::int16_t v ) noexcept { _avl_height = v; }
    void set_root_offset( index_type v ) noexcept { _root_offset = v; }

    // Reset AVL fields to "not in tree" state
    void reset_avl_fields() noexcept
    {
        _left_offset   = AddressTraitsT::no_block;
        _right_offset  = AddressTraitsT::no_block;
        _parent_offset = AddressTraitsT::no_block;
        _avl_height    = 0;
    }
};

// Проверка бинарной совместимости с BlockHeader
static_assert( sizeof( BlockStateBase<DefaultAddressTraits> ) == 32,
               "BlockStateBase<DefaultAddressTraits> must be 32 bytes (Issue #93)" );

/**
 * @brief FreeBlock — свободный блок в корректном состоянии.
 *
 * Инварианты:
 *   - size == 0
 *   - root_offset == 0
 *   - Блок находится в AVL-дереве свободных блоков
 *
 * Допустимые операции:
 *   - remove_from_avl() → FreeBlockRemovedAVL (начало allocate)
 */
template <typename AddressTraitsT> class FreeBlock : public BlockStateBase<AddressTraitsT>
{
  public:
    using Base       = BlockStateBase<AddressTraitsT>;
    using index_type = typename AddressTraitsT::index_type;

    /**
     * @brief Интерпретировать сырые байты как FreeBlock.
     *
     * @param raw Указатель на BlockHeader / Block<A>.
     * @return Указатель на FreeBlock.
     *
     * @warning Вызывающий код должен гарантировать, что блок действительно свободен.
     */
    static FreeBlock* cast_from_raw( void* raw ) noexcept { return reinterpret_cast<FreeBlock*>( raw ); }

    static const FreeBlock* cast_from_raw( const void* raw ) noexcept
    {
        return reinterpret_cast<const FreeBlock*>( raw );
    }

    /**
     * @brief Проверить инварианты свободного блока.
     * @return true если блок в корректном состоянии FreeBlock.
     */
    bool verify_invariants() const noexcept { return Base::is_free(); }

    /**
     * @brief Удалить блок из AVL-дерева (первый шаг allocate).
     *
     * @note AVL-операция выполняется вызывающим кодом отдельно.
     * @return Указатель на блок в состоянии FreeBlockRemovedAVL.
     */
    FreeBlockRemovedAVL<AddressTraitsT>* remove_from_avl() noexcept
    {
        // AVL-удаление выполняется внешне; здесь только reinterpret
        // (инварианты сохраняются: size=0, root_offset=0)
        return reinterpret_cast<FreeBlockRemovedAVL<AddressTraitsT>*>( this );
    }
};

/**
 * @brief FreeBlockRemovedAVL — свободный блок, удалённый из AVL-дерева.
 *
 * Переходное состояние во время операции allocate.
 *
 * Инварианты:
 *   - size == 0
 *   - root_offset == 0
 *   - Блок НЕ находится в AVL-дереве (удалён)
 *
 * Допустимые операции:
 *   - mark_as_allocated() → AllocatedBlock (завершение allocate)
 *   - begin_splitting()   → SplittingBlock (если нужно разбить блок)
 */
template <typename AddressTraitsT> class FreeBlockRemovedAVL : public BlockStateBase<AddressTraitsT>
{
  public:
    using Base       = BlockStateBase<AddressTraitsT>;
    using index_type = typename AddressTraitsT::index_type;

    /**
     * @brief Интерпретировать сырые байты как FreeBlockRemovedAVL.
     */
    static FreeBlockRemovedAVL* cast_from_raw( void* raw ) noexcept
    {
        return reinterpret_cast<FreeBlockRemovedAVL*>( raw );
    }

    /**
     * @brief Пометить блок как занятый (финализация allocate без split).
     *
     * @param data_granules Размер данных в гранулах.
     * @param own_idx       Гранульный индекс данного блока.
     * @return Указатель на блок в состоянии AllocatedBlock.
     */
    AllocatedBlock<AddressTraitsT>* mark_as_allocated( index_type data_granules, index_type own_idx ) noexcept
    {
        Base::set_size( data_granules );
        Base::set_root_offset( own_idx );
        Base::reset_avl_fields();
        return reinterpret_cast<AllocatedBlock<AddressTraitsT>*>( this );
    }

    /**
     * @brief Начать операцию разбиения блока.
     *
     * @return Указатель на блок в состоянии SplittingBlock.
     */
    SplittingBlock<AddressTraitsT>* begin_splitting() noexcept
    {
        return reinterpret_cast<SplittingBlock<AddressTraitsT>*>( this );
    }

    /**
     * @brief Восстановить блок обратно в AVL-дерево (откат allocate).
     *
     * @note AVL-операция выполняется вызывающим кодом отдельно.
     * @return Указатель на блок в состоянии FreeBlock.
     */
    FreeBlock<AddressTraitsT>* insert_to_avl() noexcept { return reinterpret_cast<FreeBlock<AddressTraitsT>*>( this ); }
};

/**
 * @brief SplittingBlock — блок в процессе разбиения на два.
 *
 * Переходное состояние во время операции allocate с split.
 *
 * Допустимые операции:
 *   - initialize_new_block() — инициализировать новый блок (memset)
 *   - link_new_block()       — обновить связный список
 *   - finalize_split()       — завершить split и вернуться к mark_as_allocated
 */
template <typename AddressTraitsT> class SplittingBlock : public BlockStateBase<AddressTraitsT>
{
  public:
    using Base       = BlockStateBase<AddressTraitsT>;
    using index_type = typename AddressTraitsT::index_type;

    /**
     * @brief Интерпретировать сырые байты как SplittingBlock.
     */
    static SplittingBlock* cast_from_raw( void* raw ) noexcept { return reinterpret_cast<SplittingBlock*>( raw ); }

    /**
     * @brief Инициализировать новый блок (результат split).
     *
     * @param new_blk_ptr Указатель на новый блок (memset + инициализация полей).
     * @param new_idx     Гранульный индекс нового блока (unused, for API clarity).
     * @param own_idx     Гранульный индекс текущего блока.
     */
    void initialize_new_block( void* new_blk_ptr, [[maybe_unused]] index_type new_idx, index_type own_idx ) noexcept
    {
        std::memset( new_blk_ptr, 0, sizeof( BlockStateBase<AddressTraitsT> ) );
        // Прямая инициализация через reinterpret_cast к "сырой" структуре с публичными полями
        struct RawBlock
        {
            index_type    size, prev, next, left, right, parent;
            std::int16_t  h;
            std::uint16_t pad;
            index_type    root;
        };
        auto* raw   = reinterpret_cast<RawBlock*>( new_blk_ptr );
        raw->prev   = own_idx;
        raw->next   = Base::next_offset();
        raw->left   = AddressTraitsT::no_block;
        raw->right  = AddressTraitsT::no_block;
        raw->parent = AddressTraitsT::no_block;
        raw->h      = 1; // Будет вставлен в AVL
    }

    /**
     * @brief Обновить связный список для включения нового блока.
     *
     * @param old_next_blk Указатель на старый следующий блок (может быть nullptr).
     * @param new_idx      Гранульный индекс нового блока.
     */
    void link_new_block( void* old_next_blk, index_type new_idx ) noexcept
    {
        if ( old_next_blk != nullptr )
        {
            // Прямая модификация через "сырую" структуру (бинарно совместима с BlockHeader)
            struct RawBlock
            {
                index_type    size, prev, next, left, right, parent;
                std::int16_t  h;
                std::uint16_t pad;
                index_type    root;
            };
            auto* raw = reinterpret_cast<RawBlock*>( old_next_blk );
            raw->prev = new_idx;
        }
        Base::set_next_offset( new_idx );
    }

    /**
     * @brief Завершить операцию split и пометить текущий блок как занятый.
     *
     * @param data_granules Размер данных в гранулах.
     * @param own_idx       Гранульный индекс текущего блока.
     * @return Указатель на блок в состоянии AllocatedBlock.
     */
    AllocatedBlock<AddressTraitsT>* finalize_split( index_type data_granules, index_type own_idx ) noexcept
    {
        Base::set_size( data_granules );
        Base::set_root_offset( own_idx );
        Base::reset_avl_fields();
        return reinterpret_cast<AllocatedBlock<AddressTraitsT>*>( this );
    }
};

/**
 * @brief AllocatedBlock — занятый блок в корректном состоянии.
 *
 * Инварианты:
 *   - size > 0
 *   - root_offset == собственный гранульный индекс
 *   - Блок НЕ находится в AVL-дереве свободных блоков
 *
 * Допустимые операции:
 *   - mark_as_free() → FreeBlockNotInAVL (начало deallocate)
 *   - user_ptr()     — получить указатель на пользовательские данные
 */
template <typename AddressTraitsT> class AllocatedBlock : public BlockStateBase<AddressTraitsT>
{
  public:
    using Base       = BlockStateBase<AddressTraitsT>;
    using index_type = typename AddressTraitsT::index_type;

    /**
     * @brief Интерпретировать сырые байты как AllocatedBlock.
     */
    static AllocatedBlock* cast_from_raw( void* raw ) noexcept { return reinterpret_cast<AllocatedBlock*>( raw ); }

    static const AllocatedBlock* cast_from_raw( const void* raw ) noexcept
    {
        return reinterpret_cast<const AllocatedBlock*>( raw );
    }

    /**
     * @brief Проверить инварианты занятого блока.
     * @param own_idx Гранульный индекс данного блока.
     * @return true если блок в корректном состоянии AllocatedBlock.
     */
    bool verify_invariants( index_type own_idx ) const noexcept { return Base::is_allocated( own_idx ); }

    /**
     * @brief Получить указатель на пользовательские данные.
     * @return Указатель на данные (после заголовка блока).
     */
    void* user_ptr() noexcept
    {
        return reinterpret_cast<std::uint8_t*>( this ) + sizeof( BlockStateBase<AddressTraitsT> );
    }

    const void* user_ptr() const noexcept
    {
        return reinterpret_cast<const std::uint8_t*>( this ) + sizeof( BlockStateBase<AddressTraitsT> );
    }

    /**
     * @brief Пометить блок как свободный (первый шаг deallocate).
     *
     * @return Указатель на блок в состоянии FreeBlockNotInAVL.
     */
    FreeBlockNotInAVL<AddressTraitsT>* mark_as_free() noexcept
    {
        Base::set_size( 0 );
        Base::set_root_offset( 0 );
        return reinterpret_cast<FreeBlockNotInAVL<AddressTraitsT>*>( this );
    }
};

/**
 * @brief FreeBlockNotInAVL — только что освобождённый блок, ещё не в AVL.
 *
 * Переходное состояние во время операции deallocate, перед coalesce.
 *
 * Инварианты:
 *   - size == 0
 *   - root_offset == 0
 *   - Блок НЕ находится в AVL-дереве (ещё не добавлен)
 *
 * Допустимые операции:
 *   - begin_coalescing() → CoalescingBlock (если есть соседи для слияния)
 *   - insert_to_avl()    → FreeBlock (завершение deallocate)
 */
template <typename AddressTraitsT> class FreeBlockNotInAVL : public BlockStateBase<AddressTraitsT>
{
  public:
    using Base       = BlockStateBase<AddressTraitsT>;
    using index_type = typename AddressTraitsT::index_type;

    /**
     * @brief Интерпретировать сырые байты как FreeBlockNotInAVL.
     */
    static FreeBlockNotInAVL* cast_from_raw( void* raw ) noexcept
    {
        return reinterpret_cast<FreeBlockNotInAVL*>( raw );
    }

    /**
     * @brief Начать операцию слияния с соседними блоками.
     *
     * @return Указатель на блок в состоянии CoalescingBlock.
     */
    CoalescingBlock<AddressTraitsT>* begin_coalescing() noexcept
    {
        return reinterpret_cast<CoalescingBlock<AddressTraitsT>*>( this );
    }

    /**
     * @brief Добавить блок в AVL-дерево (завершение deallocate).
     *
     * @note AVL-операция выполняется вызывающим кодом отдельно.
     * @return Указатель на блок в состоянии FreeBlock.
     */
    FreeBlock<AddressTraitsT>* insert_to_avl() noexcept
    {
        Base::set_avl_height( 1 ); // Готов к вставке в AVL
        return reinterpret_cast<FreeBlock<AddressTraitsT>*>( this );
    }
};

/**
 * @brief CoalescingBlock — блок в процессе слияния с соседями.
 *
 * Переходное состояние во время операции coalesce.
 *
 * Допустимые операции:
 *   - coalesce_with_next() — слить с правым соседом
 *   - coalesce_with_prev() — слить с левым соседом (this будет уничтожен)
 *   - finalize_coalesce()  — завершить слияние и добавить в AVL
 */
template <typename AddressTraitsT> class CoalescingBlock : public BlockStateBase<AddressTraitsT>
{
  public:
    using Base       = BlockStateBase<AddressTraitsT>;
    using index_type = typename AddressTraitsT::index_type;

    /**
     * @brief Интерпретировать сырые байты как CoalescingBlock.
     */
    static CoalescingBlock* cast_from_raw( void* raw ) noexcept { return reinterpret_cast<CoalescingBlock*>( raw ); }

    /**
     * @brief Слить текущий блок с правым соседом.
     *
     * @param next_blk       Указатель на правый соседний блок (будет поглощён).
     * @param next_next_blk  Указатель на блок после соседа (может быть nullptr).
     * @param own_idx        Гранульный индекс текущего блока.
     *
     * @note AVL-удаление соседа выполняется вызывающим кодом отдельно.
     */
    void coalesce_with_next( void* next_blk, void* next_next_blk, index_type own_idx ) noexcept
    {
        // "Сырая" структура для прямого доступа (бинарно совместима с BlockHeader)
        struct RawBlock
        {
            index_type    size, prev, next, left, right, parent;
            std::int16_t  h;
            std::uint16_t pad;
            index_type    root;
        };
        auto* nxt = reinterpret_cast<RawBlock*>( next_blk );

        // Обновляем связный список
        Base::set_next_offset( nxt->next );
        if ( next_next_blk != nullptr )
        {
            // Direct byte-level write: prev is at offset 4 (after size)
            auto* byte_ptr                                 = reinterpret_cast<std::uint8_t*>( next_next_blk );
            *reinterpret_cast<index_type*>( byte_ptr + 4 ) = own_idx;
        }

        // Обнуляем поглощённый блок
        std::memset( next_blk, 0, sizeof( BlockStateBase<AddressTraitsT> ) );
    }

    /**
     * @brief Слить текущий блок с левым соседом (текущий будет поглощён).
     *
     * @param prev_blk       Указатель на левый соседний блок (станет результатом).
     * @param next_blk       Указатель на следующий блок за текущим (может быть nullptr).
     * @param prev_idx       Гранульный индекс левого соседа.
     *
     * @note AVL-удаление соседа выполняется вызывающим кодом отдельно.
     * @return Указатель на результирующий блок (prev_blk) в состоянии CoalescingBlock.
     */
    CoalescingBlock<AddressTraitsT>* coalesce_with_prev( void* prev_blk, void* next_blk, index_type prev_idx ) noexcept
    {
        // Direct byte-level writes to ensure correct layout
        auto* prv_bytes = reinterpret_cast<std::uint8_t*>( prev_blk );
        // next is at offset 8
        *reinterpret_cast<index_type*>( prv_bytes + 8 ) = Base::next_offset();

        if ( next_blk != nullptr )
        {
            auto* nxt_bytes = reinterpret_cast<std::uint8_t*>( next_blk );
            // prev is at offset 4
            *reinterpret_cast<index_type*>( nxt_bytes + 4 ) = prev_idx;
        }

        // Обнуляем текущий блок (поглощён)
        std::memset( this, 0, sizeof( BlockStateBase<AddressTraitsT> ) );

        // Возвращаем левый сосед как результирующий блок
        return reinterpret_cast<CoalescingBlock<AddressTraitsT>*>( prev_blk );
    }

    /**
     * @brief Завершить операцию coalesce и добавить блок в AVL-дерево.
     *
     * @note AVL-операция выполняется вызывающим кодом отдельно.
     * @return Указатель на блок в состоянии FreeBlock.
     */
    FreeBlock<AddressTraitsT>* finalize_coalesce() noexcept
    {
        Base::set_avl_height( 1 ); // Готов к вставке в AVL
        return reinterpret_cast<FreeBlock<AddressTraitsT>*>( this );
    }
};

// ─── Утилиты для работы со state machine ───────────────────────────────────────

/**
 * @brief Определить состояние блока по сырым данным.
 *
 * @tparam AddressTraitsT Traits адресного пространства.
 * @param raw_blk   Указатель на блок (BlockHeader / Block<A>).
 * @param own_idx   Гранульный индекс данного блока.
 * @return 0 = FreeBlock, 1 = AllocatedBlock, -1 = неопределённое/переходное.
 */
template <typename AddressTraitsT>
int detect_block_state( const void* raw_blk, typename AddressTraitsT::index_type own_idx ) noexcept
{
    const auto* base = reinterpret_cast<const BlockStateBase<AddressTraitsT>*>( raw_blk );
    if ( base->is_free() )
        return 0; // FreeBlock (или переходное — требует проверки AVL)
    if ( base->is_allocated( own_idx ) )
        return 1; // AllocatedBlock
    return -1;    // Неопределённое состояние (ошибка или переходное)
}

/**
 * @brief Восстановить блок в корректное состояние (при load()).
 *
 * Используется для восстановления после crash — приводит блок к корректному
 * состоянию (FreeBlock или AllocatedBlock) в зависимости от size и root_offset.
 *
 * @tparam AddressTraitsT Traits адресного пространства.
 * @param raw_blk   Указатель на блок.
 * @param own_idx   Гранульный индекс данного блока.
 */
template <typename AddressTraitsT>
void recover_block_state( void* raw_blk, typename AddressTraitsT::index_type own_idx ) noexcept
{
    using index_type = typename AddressTraitsT::index_type;
    // "Сырая" структура для прямого доступа (бинарно совместима с BlockHeader)
    struct RawBlock
    {
        index_type    size, prev, next, left, right, parent;
        std::int16_t  h;
        std::uint16_t pad;
        index_type    root;
    };
    auto* blk = reinterpret_cast<RawBlock*>( raw_blk );

    // Если size > 0, но root_offset неверен — исправляем
    if ( blk->size > 0 && blk->root != own_idx )
    {
        blk->root = own_idx;
    }

    // Если size == 0, но root_offset != 0 — исправляем
    if ( blk->size == 0 && blk->root != 0 )
    {
        blk->root = 0;
    }
}

} // namespace pmm
