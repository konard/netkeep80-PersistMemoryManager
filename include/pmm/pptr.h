/**
 * @file pmm/pptr.h
 * @brief pptr<T, ManagerT> — персистентный типизированный указатель (Issue #102).
 *
 * Выделен из legacy_manager.h для использования с AbstractPersistMemoryManager.
 *
 * pptr<T, ManagerT> хранит гранульный индекс (4 байта, uint32_t) вместо адреса,
 * что делает его адресно-независимым и пригодным для персистентных хранилищ:
 *   - 4 байта вместо 8 (для 64-bit)
 *   - Нет смещения при повторной загрузке по другому адресу
 *   - Запрет адресной арифметики (pptr++ запрещён)
 *   - Index 0 означает null
 *
 * Единственный поддерживаемый режим (Issue #102):
 *   - `pptr<T, ManagerT>` — привязан к типу менеджера, разыменование через p.resolve(mgr)
 *
 * Разыменование:
 *   - Для AbstractPersistMemoryManager (рекомендуется): mgr.resolve<T>(p) или p.resolve(mgr)
 *
 * Пример использования с AbstractPersistMemoryManager:
 * @code
 *   using MyMgr = pmm::presets::SingleThreadedHeap;
 *   MyMgr pmm;
 *   pmm.create(64 * 1024);
 *
 *   // pptr привязан к типу менеджера через nested alias Manager::pptr<T>
 *   MyMgr::pptr<int> p = pmm.allocate_typed<int>();
 *   if (p) {
 *       *p.resolve(pmm) = 42;  // разыменование через экземпляр менеджера
 *   }
 *   pmm.deallocate_typed(p);
 * @endcode
 *
 * @see abstract_pmm.h — AbstractPersistMemoryManager::allocate_typed()
 * @see abstract_pmm.h — AbstractPersistMemoryManager::resolve()
 * @version 0.3 (Issue #102 — ManagerT mandatory, legacy void support removed)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace pmm
{

/**
 * @brief Персистентный типизированный указатель (4 байта, гранульный индекс).
 *
 * Хранит гранульный индекс пользовательских данных, а не адрес.
 * Адресно-независим: корректно загружается из файла по другому базовому адресу.
 *
 * @tparam T Тип данных, на который указывает pptr.
 * @tparam ManagerT Тип менеджера (обязателен, void не допускается).
 *
 * Для разыменования требуется базовый указатель менеджера:
 *   - `mgr.resolve<T>(p)` или `p.resolve(mgr)` — для AbstractPersistMemoryManager
 */
template <class T, class ManagerT> class pptr
{
    static_assert( !std::is_void<ManagerT>::value,
                   "pptr<T, void> is no longer supported. Use pptr<T, ManagerT> with a concrete manager type. "
                   "See pmm_presets.h for available presets." );

    std::uint32_t _idx; ///< Гранульный индекс пользовательских данных (0 = null)

  public:
    /// @brief Тип данных, на который ссылается pptr.
    using element_type = T;

    /// @brief Тип менеджера, к которому привязан pptr.
    using manager_type = ManagerT;

    constexpr pptr() noexcept : _idx( 0 ) {}
    constexpr explicit pptr( std::uint32_t idx ) noexcept : _idx( idx ) {}
    constexpr pptr( const pptr& ) noexcept            = default;
    constexpr pptr& operator=( const pptr& ) noexcept = default;
    ~pptr() noexcept                                  = default;

    // Адресная арифметика запрещена — pptr не является итератором
    pptr& operator++()      = delete;
    pptr  operator++( int ) = delete;
    pptr& operator--()      = delete;
    pptr  operator--( int ) = delete;

    /// @brief Проверить, является ли указатель нулевым.
    constexpr bool is_null() const noexcept { return _idx == 0; }

    /// @brief Явное преобразование в bool: true если не null.
    constexpr explicit operator bool() const noexcept { return _idx != 0; }

    /// @brief Получить гранульный индекс (для сохранения/восстановления).
    constexpr std::uint32_t offset() const noexcept { return _idx; }

    /// @brief Сравнение персистентных указателей одного типа.
    constexpr bool operator==( const pptr& other ) const noexcept { return _idx == other._idx; }
    constexpr bool operator!=( const pptr& other ) const noexcept { return _idx != other._idx; }

    // ─── Разыменование через экземпляр менеджера ───────────────────────────────

    /**
     * @brief Разыменовать через экземпляр менеджера.
     *
     * Эквивалентно mgr.resolve<T>(*this).
     *
     * @param mgr Экземпляр менеджера, с которым связан данный pptr.
     * @return T* — указатель на данные или nullptr если is_null().
     */
    T* resolve( ManagerT& mgr ) const noexcept { return mgr.template resolve<T>( *this ); }
};

// pptr<T, ManagerT> должен быть 4 байта (uint32_t гранульный индекс) — ManagerT не хранится
static_assert( sizeof( pptr<int, struct DummyMgr> ) == 4, "sizeof(pptr<T,ManagerT>) must be 4 bytes (Issue #59)" );
static_assert( sizeof( pptr<double, struct DummyMgr2> ) == 4, "sizeof(pptr<T,ManagerT>) must be 4 bytes (Issue #59)" );

} // namespace pmm
