/**
 * @file pmm/pstringview.h
 * @brief pstringview<ManagerT> — персистентная строка только для чтения с интернированием (Issue #151).
 *
 * Реализует хранение строк в персистентном адресном пространстве (ПАП) с гарантией
 * уникальности: одна и та же строка хранится в ПАП ровно один раз.
 *
 * Ключевые особенности:
 *   - Read-only: символьные данные никогда не изменяются после создания.
 *   - Интернирование: одинаковые строки используют одно и то же хранилище.
 *     Два pstringview с одинаковым содержимым указывают на один chars_idx.
 *   - Блокировка блоков: блоки с символьными данными блокируются через
 *     lock_block_permanent() — они не могут быть освобождены через deallocate().
 *   - Словарь: pstringview_table<ManagerT> растёт в течение жизни менеджера,
 *     экономя память за счёт дедупликации строковых констант.
 *   - Персистентность: granule-индексы адресно-независимы и корректны
 *     при перезагрузке ПАП по другому базовому адресу.
 *
 * Использование:
 * @code
 *   using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig>;
 *   Mgr::create(64 * 1024);
 *
 *   // Интернировать строку (найти существующую или создать новую)
 *   Mgr::pptr<pmm::pstringview<Mgr>> p = pmm::pstringview_manager<Mgr>::intern("hello");
 *   if (p) {
 *       const char* s = p->c_str();   // "hello"
 *       std::size_t n = p->size();    // 5
 *   }
 *
 *   // Повторное интернирование возвращает тот же pptr
 *   Mgr::pptr<pmm::pstringview<Mgr>> p2 = pmm::pstringview_manager<Mgr>::intern("hello");
 *   assert(p == p2);  // одинаковый granule index
 *
 *   Mgr::destroy();
 * @endcode
 *
 * @see persist_memory_manager.h — PersistMemoryManager (статическая модель, Issue #110)
 * @see pptr.h — pptr<T, ManagerT> (персистентный указатель)
 * @version 0.1 (Issue #151 — реализация pstringview)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace pmm
{

// Forward declarations
template <typename ManagerT> struct pstringview;
template <typename ManagerT> struct pstringview_table;
template <typename ManagerT> struct pstringview_manager;

// ─── pstringview_entry ──────────────────────────────────────────────────────

/// @brief Одна запись в таблице интернирования строк.
///
/// Хранит FNV-1a хэш, granule-индекс символьных данных и длину строки.
/// Пустая ячейка: chars_idx == 0.
template <typename ManagerT> struct pstringview_entry
{
    using index_type = typename ManagerT::index_type;

    std::uint64_t hash;      ///< FNV-1a хэш строки (0 = пустая ячейка)
    index_type    chars_idx; ///< Granule-индекс массива char в ПАП; 0 = пустая ячейка
    index_type psview_idx; ///< Granule-индекс объекта pstringview в ПАП; 0 если не создан
    std::uint32_t length;  ///< Длина строки (без нулевого терминатора)
};

// Note: pstringview_entry<ManagerT> is trivially copyable for any valid ManagerT
// (all fields are integral types). This is verified in tests.

// ─── pstringview ─────────────────────────────────────────────────────────────

/**
 * @brief Персистентная интернированная read-only строка (Issue #151).
 *
 * Хранит granule-индекс символьного массива (chars_idx) и длину строки.
 * Объекты pstringview живут в ПАП и не могут быть созданы на стеке напрямую.
 * Создавайте через pstringview_manager<ManagerT>::intern(s).
 *
 * Инварианты:
 *   - chars_idx указывает на null-terminated char[], заблокированный навечно.
 *   - Два pstringview с одинаковым содержимым имеют одинаковый chars_idx.
 *
 * @tparam ManagerT Тип менеджера памяти (PersistMemoryManager<ConfigT, InstanceId>).
 */
template <typename ManagerT> struct pstringview
{
    using manager_type = ManagerT;
    using index_type   = typename ManagerT::index_type;

    index_type    chars_idx; ///< Granule-индекс массива char в ПАП; 0 = пустая строка
    std::uint32_t length;    ///< Длина строки (без нулевого терминатора)

    /// @brief Получить raw C-строку. Действителен, пока менеджер инициализирован.
    const char* c_str() const noexcept
    {
        if ( chars_idx == 0 )
            return "";
        typename ManagerT::template pptr<char> p( chars_idx );
        const char*                            raw = ManagerT::template resolve<char>( p );
        return ( raw != nullptr ) ? raw : "";
    }

    /// @brief Длина строки (без нулевого терминатора).
    std::size_t size() const noexcept { return static_cast<std::size_t>( length ); }

    /// @brief Проверить, пустая ли строка.
    bool empty() const noexcept { return length == 0; }

    /// @brief Сравнение с C-строкой.
    bool operator==( const char* s ) const noexcept
    {
        if ( s == nullptr )
            return length == 0;
        return std::strcmp( c_str(), s ) == 0;
    }

    /// @brief Равенство двух pstringview.
    ///
    /// Интернирование гарантирует: одинаковые строки → одинаковый chars_idx.
    bool operator==( const pstringview& other ) const noexcept { return chars_idx == other.chars_idx; }

    /// @brief Неравенство с C-строкой.
    bool operator!=( const char* s ) const noexcept { return !( *this == s ); }

    /// @brief Неравенство двух pstringview.
    bool operator!=( const pstringview& other ) const noexcept { return !( *this == other ); }

    /// @brief Упорядочивание pstringview (для использования в pmap).
    bool operator<( const pstringview& other ) const noexcept { return std::strcmp( c_str(), other.c_str() ) < 0; }

  private:
    // Создание pstringview на стеке запрещено — только через pstringview_manager::intern().
    pstringview() noexcept : chars_idx( 0 ), length( 0 ) {}
    ~pstringview() = default;

    template <typename M> friend struct pstringview_manager;
};

// ─── pstringview_table ───────────────────────────────────────────────────────

/**
 * @brief Персистентная таблица интернирования строк (Issue #151).
 *
 * Хэш-таблица с открытой адресацией и линейным пробированием.
 * Ключ: FNV-1a хэш + granule-индекс символьных данных.
 * Строки НИКОГДА не удаляются — бессмертные записи.
 *
 * Load factor: < 0.5 (при превышении выполняется rehash × 2).
 *
 * @tparam ManagerT Тип менеджера памяти.
 */
template <typename ManagerT> struct pstringview_table
{
    using manager_type = ManagerT;
    using index_type   = typename ManagerT::index_type;
    using entry_type   = pstringview_entry<ManagerT>;
    using entry_pptr   = typename ManagerT::template pptr<entry_type>;
    using psview_type  = pstringview<ManagerT>;
    using psview_pptr  = typename ManagerT::template pptr<psview_type>;
    using char_pptr    = typename ManagerT::template pptr<char>;

    std::uint32_t count_;    ///< Число занятых ячеек
    std::uint32_t capacity_; ///< Ёмкость таблицы (число ячеек)
    index_type buckets_idx;  ///< Granule-индекс массива ячеек в ПАП; 0 если не создан

    /// @brief Вычислить FNV-1a хэш строки.
    static std::uint64_t fnv1a( const char* s ) noexcept
    {
        std::uint64_t h = 14695981039346656037ULL;
        while ( *s != '\0' )
        {
            h ^= static_cast<std::uint8_t>( *s++ );
            h *= 1099511628211ULL;
        }
        return h;
    }

    /// @brief Результат интернирования: granule-индексы символьных данных и pstringview.
    struct InternResult
    {
        index_type    chars_idx;  ///< Granule-индекс символьного массива
        index_type    psview_idx; ///< Granule-индекс объекта pstringview
        std::uint32_t length;     ///< Длина строки
    };

    /**
     * @brief Интернировать строку s: вернуть granule-индексы для строки.
     *
     * Если строка уже есть — вернуть существующие индексы.
     * Если нет — создать новый массив char в ПАП и новый pstringview,
     *            заблокировать блоки навечно (lock_block_permanent).
     *
     * @param self_idx Granule-индекс самого объекта pstringview_table в ПАП.
     * @param s C-строка для интернирования (nullptr обрабатывается как "").
     * @return InternResult с заполненными chars_idx, psview_idx и length.
     */
    InternResult intern( index_type self_idx, const char* s ) noexcept
    {
        if ( s == nullptr )
            s = "";

        auto          len  = static_cast<std::uint32_t>( std::strlen( s ) );
        std::uint64_t hash = fnv1a( s );

        // Убеждаемся, что таблица инициализирована.
        _ensure_initialized( self_idx );

        // Перехэшируем, если load factor > 0.5.
        {
            pstringview_table* self = _resolve_self( self_idx );
            if ( self != nullptr && self->count_ * 2 >= self->capacity_ )
                self->_rehash( self_idx, self->capacity_ * 2 );
        }

        // Ищем ячейку.
        pstringview_table* self = _resolve_self( self_idx );
        if ( self == nullptr )
        {
            // Аварийный случай: не удалось инициализировать таблицу.
            return { static_cast<index_type>( 0 ), static_cast<index_type>( 0 ), 0 };
        }

        std::uint32_t cap = self->capacity_;
        if ( cap == 0 )
            return { static_cast<index_type>( 0 ), static_cast<index_type>( 0 ), 0 };

        auto idx = static_cast<std::uint32_t>( hash % cap );

        for ( std::uint32_t probe = 0; probe < cap; probe++ )
        {
            // Переразрешаем self на каждой итерации (возможен realloc при расширении).
            self = _resolve_self( self_idx );
            if ( self == nullptr )
                break;
            cap           = self->capacity_;
            entry_type* e = _get_entry( self, idx );
            if ( e == nullptr )
                break;

            if ( e->chars_idx == 0 )
            {
                // Пустая ячейка — создаём новые данные.
                index_type new_chars  = _create_chars( self_idx, s, len );
                index_type new_psview = _create_psview( self_idx, new_chars, len );

                // Переразрешаем после возможного realloc.
                self = _resolve_self( self_idx );
                if ( self == nullptr )
                    return { new_chars, new_psview, len };
                cap = self->capacity_;
                // Пересчитываем idx после возможного rehash.
                idx = static_cast<std::uint32_t>( hash % cap );

                // Находим первую пустую ячейку для вставки.
                for ( std::uint32_t p2 = 0; p2 < cap; p2++ )
                {
                    entry_type* c2 = _get_entry( self, idx );
                    if ( c2 == nullptr )
                        break;
                    if ( c2->chars_idx == 0 )
                    {
                        c2->hash       = hash;
                        c2->chars_idx  = new_chars;
                        c2->psview_idx = new_psview;
                        c2->length     = len;
                        self->count_++;
                        return { new_chars, new_psview, len };
                    }
                    idx = ( idx + 1 ) % cap;
                }
                // Не должно произойти при корректном load factor.
                return { new_chars, new_psview, len };
            }

            // Занятая ячейка: сравниваем хэш, длину и содержимое.
            if ( e->hash == hash && e->length == len )
            {
                char_pptr   cp( e->chars_idx );
                const char* cs = ManagerT::template resolve<char>( cp );
                if ( cs != nullptr && std::strcmp( cs, s ) == 0 )
                    return { e->chars_idx, e->psview_idx, e->length };
            }
            idx = ( idx + 1 ) % cap;
        }

        // Таблица переполнена (не должно происходить при load factor < 0.5).
        index_type new_chars  = _create_chars( self_idx, s, len );
        index_type new_psview = _create_psview( self_idx, new_chars, len );
        return { new_chars, new_psview, len };
    }

  private:
    /// @brief Получить указатель на себя по granule-индексу (переразрешение).
    static pstringview_table* _resolve_self( index_type self_idx ) noexcept
    {
        if ( self_idx == 0 )
            return nullptr;
        typename ManagerT::template pptr<pstringview_table> p( self_idx );
        return ManagerT::template resolve<pstringview_table>( p );
    }

    /// @brief Получить указатель на ячейку таблицы по индексу.
    static entry_type* _get_entry( pstringview_table* self, std::uint32_t idx ) noexcept
    {
        if ( self == nullptr || self->buckets_idx == 0 )
            return nullptr;
        entry_pptr  ep( self->buckets_idx );
        entry_type* base_entry = ManagerT::template resolve<entry_type>( ep );
        if ( base_entry == nullptr )
            return nullptr;
        return base_entry + idx;
    }

    /// @brief Инициализировать массив ячеек.
    void _ensure_initialized( index_type self_idx ) noexcept
    {
        pstringview_table* self = _resolve_self( self_idx );
        if ( self == nullptr || self->capacity_ > 0 )
            return;
        _init_buckets( self_idx, 16u );
    }

    /// @brief Инициализировать массив из initial_cap пустых ячеек.
    static void _init_buckets( index_type self_idx, std::uint32_t initial_cap ) noexcept
    {
        if ( initial_cap == 0 )
            return;

        entry_pptr arr = ManagerT::template allocate_typed<entry_type>( static_cast<std::size_t>( initial_cap ) );
        if ( arr.is_null() )
            return;

        // Инициализируем нулями (пустые ячейки: chars_idx == 0).
        entry_type* raw = ManagerT::template resolve<entry_type>( arr );
        if ( raw == nullptr )
            return;
        for ( std::uint32_t i = 0; i < initial_cap; i++ )
        {
            raw[i].hash       = 0;
            raw[i].chars_idx  = static_cast<index_type>( 0 );
            raw[i].psview_idx = static_cast<index_type>( 0 );
            raw[i].length     = 0;
        }

        // Блокируем массив ячеек навечно (он никогда не должен освобождаться).
        void* buckets_raw = ManagerT::template resolve<entry_type>( arr );
        if ( buckets_raw != nullptr )
            ManagerT::lock_block_permanent( buckets_raw );

        pstringview_table* self = _resolve_self( self_idx );
        if ( self == nullptr )
            return;
        self->buckets_idx = arr.offset();
        self->capacity_   = initial_cap;
        self->count_      = 0;
    }

    /// @brief Создать новый массив char в ПАП. Возвращает granule-индекс.
    static index_type _create_chars( index_type /*self_idx*/, const char* s, std::uint32_t len ) noexcept
    {
        char_pptr arr = ManagerT::template allocate_typed<char>( static_cast<std::size_t>( len + 1 ) );
        if ( arr.is_null() )
            return static_cast<index_type>( 0 );

        char* dst = ManagerT::template resolve<char>( arr );
        if ( dst != nullptr )
            std::memcpy( dst, s, static_cast<std::size_t>( len + 1 ) );

        // Блокируем символьные данные навечно (Issue #151, Issue #126).
        if ( dst != nullptr )
            ManagerT::lock_block_permanent( dst );

        return arr.offset();
    }

    /// @brief Создать объект pstringview в ПАП. Возвращает granule-индекс.
    static index_type _create_psview( index_type self_idx, index_type chars_idx_val, std::uint32_t len ) noexcept
    {
        // Сохраняем self_idx перед возможным realloc.
        (void)self_idx;

        typename ManagerT::template pptr<psview_type> p = ManagerT::template allocate_typed<psview_type>();
        if ( p.is_null() )
            return static_cast<index_type>( 0 );

        psview_type* obj = ManagerT::template resolve<psview_type>( p );
        if ( obj != nullptr )
        {
            obj->chars_idx = chars_idx_val;
            obj->length    = len;
        }

        // Блокируем объект pstringview навечно (Issue #151, Issue #126).
        if ( obj != nullptr )
            ManagerT::lock_block_permanent( obj );

        return p.offset();
    }

    /// @brief Перехэшировать таблицу в новую ёмкость new_cap.
    void _rehash( index_type self_idx, std::uint32_t new_cap ) noexcept
    {
        if ( new_cap == 0 )
            return;

        pstringview_table* self = _resolve_self( self_idx );
        if ( self == nullptr )
            return;
        std::uint32_t old_cap     = self->capacity_;
        index_type    old_bkt_idx = self->buckets_idx;

        // Выделяем новый массив ячеек.
        entry_pptr new_arr = ManagerT::template allocate_typed<entry_type>( static_cast<std::size_t>( new_cap ) );
        if ( new_arr.is_null() )
            return;

        entry_type* new_raw = ManagerT::template resolve<entry_type>( new_arr );
        if ( new_raw == nullptr )
            return;
        for ( std::uint32_t i = 0; i < new_cap; i++ )
        {
            new_raw[i].hash       = 0;
            new_raw[i].chars_idx  = static_cast<index_type>( 0 );
            new_raw[i].psview_idx = static_cast<index_type>( 0 );
            new_raw[i].length     = 0;
        }

        // Переносим все существующие записи.
        if ( old_bkt_idx != 0 )
        {
            entry_pptr  old_ep( old_bkt_idx );
            entry_type* old_raw = ManagerT::template resolve<entry_type>( old_ep );
            if ( old_raw != nullptr )
            {
                for ( std::uint32_t i = 0; i < old_cap; i++ )
                {
                    if ( old_raw[i].chars_idx == 0 )
                        continue;
                    std::uint64_t h   = old_raw[i].hash;
                    auto          ins = static_cast<std::uint32_t>( h % new_cap );
                    for ( std::uint32_t p = 0; p < new_cap; p++ )
                    {
                        // Переразрешаем после каждой итерации (перестраховка).
                        new_raw = ManagerT::template resolve<entry_type>( new_arr );
                        if ( new_raw == nullptr )
                            break;
                        if ( new_raw[ins].chars_idx == 0 )
                        {
                            new_raw[ins] = old_raw[i];
                            break;
                        }
                        ins = ( ins + 1 ) % new_cap;
                    }
                    // Переразрешаем old_raw после новых аллокаций.
                    old_raw = ManagerT::template resolve<entry_type>( old_ep );
                    if ( old_raw == nullptr )
                        break;
                }
            }
        }

        // Блокируем новый массив навечно.
        void* new_buckets_raw = ManagerT::template resolve<entry_type>( new_arr );
        if ( new_buckets_raw != nullptr )
            ManagerT::lock_block_permanent( new_buckets_raw );

        // Обновляем self.
        self = _resolve_self( self_idx );
        if ( self == nullptr )
            return;
        self->buckets_idx = new_arr.offset();
        self->capacity_   = new_cap;
        // count_ не изменяется (строки бессмертны, deleted-записей нет).
    }

    // Запрещаем создание на стеке.
    pstringview_table() noexcept : count_( 0 ), capacity_( 0 ), buckets_idx( 0 ) {}
    ~pstringview_table() = default;

    template <typename M> friend struct pstringview_manager;
};

// ─── pstringview_manager ─────────────────────────────────────────────────────

/**
 * @brief Синглтон менеджера таблицы интернирования для конкретного ManagerT (Issue #151).
 *
 * Хранит granule-индекс pstringview_table в статической переменной.
 * При первом вызове intern() создаёт таблицу в ПАП.
 * При вызове reset() сбрасывает синглтон (для тестов).
 *
 * @tparam ManagerT Тип менеджера памяти.
 */
template <typename ManagerT> struct pstringview_manager
{
    using manager_type = ManagerT;
    using index_type   = typename ManagerT::index_type;
    using table_type   = pstringview_table<ManagerT>;
    using psview_type  = pstringview<ManagerT>;
    using psview_pptr  = typename ManagerT::template pptr<psview_type>;
    using table_pptr   = typename ManagerT::template pptr<table_type>;

    /**
     * @brief Интернировать строку s: найти существующий pstringview или создать новый.
     *
     * @param s C-строка для интернирования (nullptr обрабатывается как "").
     * @return pptr<pstringview<ManagerT>> — персистентный указатель на pstringview.
     *         Нулевой pptr при ошибке аллокации.
     */
    static psview_pptr intern( const char* s ) noexcept
    {
        if ( s == nullptr )
            s = "";

        // Получаем или создаём таблицу.
        index_type tbl_idx = _get_or_create_table();
        if ( tbl_idx == 0 )
            return psview_pptr();

        // Вызываем intern на таблице.
        table_pptr  tp( tbl_idx );
        table_type* tbl = ManagerT::template resolve<table_type>( tp );
        if ( tbl == nullptr )
            return psview_pptr();

        typename table_type::InternResult result = tbl->intern( tbl_idx, s );
        return psview_pptr( result.psview_idx );
    }

    /**
     * @brief Сбросить синглтон (для тестов).
     *
     * Сбрасывает статическую переменную _table_idx, но не освобождает
     * данные в ПАП (блоки заблокированы навечно).
     */
    static void reset() noexcept { _table_idx = static_cast<index_type>( 0 ); }

    /// @brief Granule-индекс таблицы интернирования в ПАП; 0 = не инициализировано.
    static inline index_type _table_idx = static_cast<index_type>( 0 );

  private:
    /**
     * @brief Получить granule-индекс таблицы интернирования.
     *
     * При первом вызове создаёт таблицу в ПАП и блокирует её навечно.
     *
     * @return Granule-индекс pstringview_table, или 0 при ошибке.
     */
    static index_type _get_or_create_table() noexcept
    {
        if ( _table_idx != 0 )
            return _table_idx;

        // Создаём таблицу в ПАП.
        table_pptr tp = ManagerT::template allocate_typed<table_type>();
        if ( tp.is_null() )
            return static_cast<index_type>( 0 );

        table_type* tbl = ManagerT::template resolve<table_type>( tp );
        if ( tbl != nullptr )
        {
            tbl->count_      = 0;
            tbl->capacity_   = 0;
            tbl->buckets_idx = static_cast<index_type>( 0 );
        }

        // Блокируем объект таблицы навечно (Issue #151, Issue #126).
        if ( tbl != nullptr )
            ManagerT::lock_block_permanent( tbl );

        _table_idx = tp.offset();
        return _table_idx;
    }
};

// Определение статической переменной _table_idx (C++17 inline).
// Объявлено как static inline в теле структуры — определение не требуется вне класса.

} // namespace pmm
