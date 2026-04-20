/**
 * @file pmm/pmap.h
 * @brief pmap<_K,_V,ManagerT> — персистентный словарь на основе AVL-дерева.
 *
 * Реализует шаблонный ассоциативный контейнер в персистентном адресном пространстве (ПАП).
 * Каждый узел словаря — это блок в ПАП, хранящий пару ключ-значение (_K, _V).
 * Узлы используют встроенные поля TreeNode (left_offset, right_offset, parent_offset,
 * avl_height) для организации AVL-дерева, как это делает pstringview.
 *
 * Ключевые особенности:
 *   - Персистентный: гранульные индексы адресно-независимы при перезагрузке ПАП.
 *   - AVL-балансировка: O(log n) для вставки, поиска и удаления.
 *   - Встроенный AVL: узлы используют встроенные TreeNode-поля Block<AT> без
 *     дополнительных аллокаций структур дерева.
 *   - Не дублирует ключи: повторная вставка по существующему ключу обновляет значение.
 *   - Узлы НЕ блокируются навечно (в отличие от pstringview).
 *   - Тип ключа _K должен поддерживать operator< и operator==.
 *   - erase(key) — удаление узла по ключу с деаллокацией памяти.
 *   - size() — количество элементов за O(n).
 *   - begin()/end() — итератор для обхода в порядке ключей.
 *   - clear() — удаление всех элементов с деаллокацией.
 *
 * Пример использования:
 * @code
 *   using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig>;
 *   Mgr::create(64 * 1024);
 *
 *   // Создать словарь
 *   using MyMap = Mgr::pmap<int, int>;
 *
 *   MyMap map;
 *   map.insert(42, 100);
 *   map.insert(10, 200);
 *
 *   auto p = map.find(42);
 *   if (!p.is_null()) {
 *       int val = p->value;  // 100
 *   }
 *
 *   map.insert(42, 300);  // обновит значение
 *
 *   // Удаление по ключу
 *   bool removed = map.erase(42);  // true
 *
 *   // Итерация в порядке ключей
 *   for (auto it = map.begin(); it != map.end(); ++it) {
 *       auto node = *it;
 *       // node->key, node->value
 *   }
 *
 *   // Количество элементов
 *   std::size_t n = map.size();  // 1
 *
 *   map.clear();  // удалить все элементы
 *
 *   Mgr::destroy();
 * @endcode
 *
 * Пример с pstringview:
 * @code
 *   using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig>;
 *   Mgr::create(64 * 1024);
 *
 *   using StrIntMap = Mgr::pmap<Mgr::pstringview, int>;
 *   StrIntMap dict;
 *   auto key = static_cast<Mgr::pptr<Mgr::pstringview>>(Mgr::pstringview("hello"));
 *   dict.insert(*key.resolve(), 42);
 *
 *   Mgr::destroy();
 * @endcode
 *
 * @see pstringview.h — аналогичный тип с AVL-деревом
 * @see avl_tree_mixin.h — общие AVL-операции
 * @see pptr.h — pptr<T, ManagerT> (персистентный указатель)
 * @see tree_node.h — TreeNode<AT> (встроенные AVL-поля каждого блока)
 * @version 0.4
 */

#pragma once

#include "pmm/avl_tree_mixin.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace pmm
{

// Forward declaration
template <typename _K, typename _V, typename ManagerT> struct pmap;

// ─── pmap_node ────────────────────────────────────────────────────────────────

/**
 * @brief Узел pmap — хранит пару ключ-значение в ПАП.
 *
 * Каждый узел является отдельным блоком в ПАП. Встроенные поля TreeNode
 * (left_offset, right_offset, parent_offset, avl_height) используются для
 * организации AVL-дерева словаря.
 *
 * @tparam _K  Тип ключа. Должен поддерживать operator< и operator==.
 * @tparam _V  Тип значения.
 */
template <typename _K, typename _V> struct pmap_node
{
    _K key;   ///< Ключ узла
    _V value; ///< Значение узла
};

// ─── pmap ─────────────────────────────────────────────────────────────────────

/**
 * @brief Персистентный ассоциативный контейнер (словарь) на основе AVL-дерева.
 *
 * Объект pmap сам по себе не хранится в ПАП — он является хелпером на стеке,
 * содержащим гранульный индекс корня AVL-дерева. Узлы словаря (pmap_node) хранятся
 * в ПАП и используют встроенные TreeNode-поля для организации AVL-дерева.
 *
 * Особенности:
 *   - Вставка/поиск/удаление за O(log n).
 *   - Повторная вставка по существующему ключу обновляет значение.
 *   - erase(key) удаляет узел и освобождает память в ПАП.
 *   - size() возвращает количество элементов за O(n).
 *   - begin()/end() — итератор для обхода в порядке ключей.
 *   - clear() — удаление всех элементов с деаллокацией.
 *   - Узлы НЕ блокируются навечно: в отличие от pstringview, узлы
 *     pmap могут быть освобождены после удаления из дерева.
 *
 * @tparam _K       Тип ключа. Должен поддерживать operator< и operator==.
 * @tparam _V       Тип значения.
 * @tparam ManagerT Тип менеджера памяти (PersistMemoryManager<ConfigT, InstanceId>).
 */
template <typename _K, typename _V, typename ManagerT> struct pmap
{
    using manager_type = ManagerT;
    using index_type   = typename ManagerT::index_type;
    using node_type    = pmap_node<_K, _V>;
    using node_pptr    = typename ManagerT::template pptr<node_type>;

    struct forest_domain_descriptor
    {
        using index_type = typename ManagerT::index_type;
        using node_type  = pmap_node<_K, _V>;
        using node_pptr  = typename ManagerT::template pptr<node_type>;

        const index_type* root_index_slot;
        index_type*       mutable_root_index_slot;

        constexpr explicit forest_domain_descriptor( index_type* root = nullptr ) noexcept
            : root_index_slot( root ), mutable_root_index_slot( root )
        {
        }

        constexpr explicit forest_domain_descriptor( const index_type* root ) noexcept
            : root_index_slot( root ), mutable_root_index_slot( nullptr )
        {
        }

        static constexpr const char* name() noexcept { return "container/pmap"; }

        index_type root_index() const noexcept { return root_index_slot != nullptr ? *root_index_slot : 0; }

        index_type* root_index_ptr() noexcept { return mutable_root_index_slot; }

        static node_type* resolve_node( node_pptr p ) noexcept { return ManagerT::template resolve<node_type>( p ); }

        static int compare_key( const _K& key, node_pptr cur ) noexcept
        {
            node_type* obj = resolve_node( cur );
            if ( obj == nullptr )
                return 0;
            return ( key == obj->key ) ? 0 : ( ( key < obj->key ) ? -1 : 1 );
        }

        static bool less_node( node_pptr lhs, node_pptr rhs ) noexcept
        {
            node_type* lhs_obj = resolve_node( lhs );
            node_type* rhs_obj = resolve_node( rhs );
            return lhs_obj != nullptr && rhs_obj != nullptr && lhs_obj->key < rhs_obj->key;
        }

        static bool validate_node( node_pptr p ) noexcept { return resolve_node( p ) != nullptr; }
    };

    using forest_domain_view_policy = detail::ForestDomainViewOps<forest_domain_descriptor>;
    using forest_domain_policy      = detail::ForestDomainOps<forest_domain_descriptor>;

    /// @brief Sentinel value for "no node" in TreeNode fields.
    static constexpr index_type no_block = ManagerT::address_traits::no_block;

    /// @brief Гранульный индекс корня AVL-дерева; 0 = пустое дерево.
    index_type _root_idx;

    // ─── Конструктор ──────────────────────────────────────────────────────────

    /// @brief Создать пустой словарь.
    pmap() noexcept : _root_idx( static_cast<index_type>( 0 ) ) {}

    // ─── Методы доступа ───────────────────────────────────────────────────────

    forest_domain_policy forest_domain_ops() noexcept
    {
        return forest_domain_policy( forest_domain_descriptor( &_root_idx ) );
    }

    forest_domain_view_policy forest_domain_view_ops() const noexcept
    {
        return forest_domain_view_policy( forest_domain_descriptor( &_root_idx ) );
    }

    /// @brief Проверить, пуст ли словарь.
    bool empty() const noexcept { return _root_idx == static_cast<index_type>( 0 ); }

    /// @brief Получить количество элементов в словаре за O(n).
    /// @return Количество элементов (подсчитывается обходом дерева).
    std::size_t size() const noexcept
    {
        if ( _root_idx == static_cast<index_type>( 0 ) )
            return 0;
        return detail::avl_subtree_count( node_pptr( _root_idx ) );
    }

    // ─── Операции со словарём ─────────────────────────────────────────────────

    /**
     * @brief Вставить или обновить пару ключ-значение.
     *
     * Если ключ уже существует в словаре — обновляет значение.
     * Если ключ новый — создаёт новый узел в ПАП и вставляет в AVL-дерево.
     *
     * @param key   Ключ для вставки.
     * @param val   Значение для вставки.
     * @return pptr на узел с данным ключом. Нулевой pptr при ошибке аллокации.
     */
    node_pptr insert( const _K& key, const _V& val ) noexcept
    {
        auto ops = forest_domain_ops();

        // Ищем существующий узел.
        node_pptr existing = ops.find( key );
        if ( !existing.is_null() )
        {
            // Ключ найден — обновляем значение.
            node_type* obj = ManagerT::template resolve<node_type>( existing );
            if ( obj != nullptr )
                obj->value = val;
            return existing;
        }

        // Создаём новый узел в ПАП.
        node_pptr new_node = ManagerT::template allocate_typed<node_type>();
        if ( new_node.is_null() )
            return node_pptr();

        node_type* obj = ManagerT::template resolve<node_type>( new_node );
        if ( obj == nullptr )
            return node_pptr();

        obj->key   = key;
        obj->value = val;

        // Инициализируем AVL-поля нового узла.
        detail::avl_init_node( new_node );

        // Вставляем в AVL-дерево.
        ops.insert( new_node );

        return new_node;
    }

    /**
     * @brief Найти узел по ключу.
     *
     * @param key Ключ для поиска.
     * @return pptr на найденный узел, или нулевой pptr если не найден.
     */
    node_pptr find( const _K& key ) const noexcept { return forest_domain_view_ops().find( key ); }

    /**
     * @brief Проверить, содержит ли словарь заданный ключ.
     *
     * @param key Ключ для проверки.
     * @return true если ключ найден.
     */
    bool contains( const _K& key ) const noexcept { return !forest_domain_view_ops().find( key ).is_null(); }

    /**
     * @brief Удалить узел по ключу.
     *
     * Находит узел с заданным ключом, удаляет его из AVL-дерева и
     * освобождает память блока в ПАП.
     *
     * @param key Ключ для удаления.
     * @return true если ключ был найден и удалён, false если ключ не найден.
     */
    bool erase( const _K& key ) noexcept
    {
        node_pptr target = forest_domain_ops().find( key );
        if ( target.is_null() )
            return false;

        detail::avl_remove( target, _root_idx );
        ManagerT::template deallocate_typed<node_type>( target );
        return true;
    }

    /**
     * @brief Очистить словарь (удалить все элементы).
     *
     * Освобождает память всех узлов в ПАП.
     */
    void clear() noexcept
    {
        if ( _root_idx != static_cast<index_type>( 0 ) )
            detail::avl_clear_subtree( node_pptr( _root_idx ),
                                       []( node_pptr p ) { ManagerT::template deallocate_typed<node_type>( p ); } );
        _root_idx = static_cast<index_type>( 0 );
    }

    /**
     * @brief Сбросить словарь (для тестов).
     *
     * Сбрасывает _root_idx, но не освобождает данные в ПАП.
     */
    void reset() noexcept { forest_domain_ops().reset_root(); }

    // ─── Итератор ───────────────────────────────────────────────

    /// @brief Итератор для обхода словаря в порядке ключей (in-order).
    /// Uses shared AvlInorderIterator template to eliminate duplication.
    using iterator = detail::AvlInorderIterator<node_pptr>;

    /// @brief Начало итерации (самый левый узел = наименьший ключ).
    iterator begin() const noexcept
    {
        if ( _root_idx == static_cast<index_type>( 0 ) )
            return iterator();
        node_pptr min = detail::avl_min_node( node_pptr( _root_idx ) );
        return iterator( min.offset() );
    }

    /// @brief Конец итерации (sentinel = 0).
    iterator end() const noexcept { return iterator( static_cast<index_type>( 0 ) ); }
};

} // namespace pmm
