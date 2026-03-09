/**
 * @file test_issue146_configs.cpp
 * @brief Тесты переосмысления конфигураций менеджеров ПАП (Issue #146).
 *
 * Проверяет:
 *   - Корректность новых конфигураций из manager_configs.h:
 *       - EmbeddedStaticConfig<N> — StaticStorage с DefaultAddressTraits
 *       - kMinGranuleSize — минимальный размер гранулы (4 байта)
 *   - Корректность нового пресета из pmm_presets.h:
 *       - EmbeddedStaticHeap<N> — менеджер с фиксированным статическим пулом
 *   - Статические проверки (static_assert) для всех конфигураций
 *   - Функциональность всех пресетов (create/allocate/deallocate/destroy)
 *
 * @see include/pmm/manager_configs.h
 * @see include/pmm/pmm_presets.h
 * @version 0.1 (Issue #146 — переосмысление конфигов)
 */

#include "pmm_single_threaded_heap.h"
#include "pmm_multi_threaded_heap.h"
#include "pmm_embedded_heap.h"
#include "pmm_industrial_db_heap.h"
#include "pmm_embedded_static_heap.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>

// ─── Макросы тестирования ─────────────────────────────────────────────────────

#define PMM_TEST( expr )                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if ( !( expr ) )                                                                                               \
        {                                                                                                              \
            std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "] " << #expr << "\n";                             \
            return false;                                                                                              \
        }                                                                                                              \
    } while ( false )

#define PMM_RUN( name, fn )                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        std::cout << "  " << name << " ... ";                                                                          \
        if ( fn() )                                                                                                    \
        {                                                                                                              \
            std::cout << "PASS\n";                                                                                     \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            std::cout << "FAIL\n";                                                                                     \
            all_passed = false;                                                                                        \
        }                                                                                                              \
    } while ( false )

// =============================================================================
// Issue #146 Tests Section A: Static checks (compile-time validation)
// =============================================================================

/// @brief kMinGranuleSize == 4 (размер слова архитектуры).
static bool test_i146_min_granule_size_constant()
{
    static_assert( pmm::kMinGranuleSize == 4, "kMinGranuleSize must be 4 (architecture word size)" );
    return true;
}

/// @brief Все конфигурации имеют granule_size >= kMinGranuleSize.
static bool test_i146_all_configs_granule_size_valid()
{
    static_assert( pmm::CacheManagerConfig::granule_size >= pmm::kMinGranuleSize,
                   "CacheManagerConfig: granule_size must be >= kMinGranuleSize" );
    static_assert( pmm::PersistentDataConfig::granule_size >= pmm::kMinGranuleSize,
                   "PersistentDataConfig: granule_size must be >= kMinGranuleSize" );
    static_assert( pmm::EmbeddedManagerConfig::granule_size >= pmm::kMinGranuleSize,
                   "EmbeddedManagerConfig: granule_size must be >= kMinGranuleSize" );
    static_assert( pmm::IndustrialDBConfig::granule_size >= pmm::kMinGranuleSize,
                   "IndustrialDBConfig: granule_size must be >= kMinGranuleSize" );
    static_assert( pmm::EmbeddedStaticConfig<4096>::granule_size >= pmm::kMinGranuleSize,
                   "EmbeddedStaticConfig: granule_size must be >= kMinGranuleSize" );
    return true;
}

/// @brief Все конфигурации имеют granule_size — степень двойки.
static bool test_i146_all_configs_granule_power_of_two()
{
    static_assert( ( pmm::CacheManagerConfig::granule_size & ( pmm::CacheManagerConfig::granule_size - 1 ) ) == 0,
                   "CacheManagerConfig: granule_size must be power of 2" );
    static_assert( ( pmm::PersistentDataConfig::granule_size & ( pmm::PersistentDataConfig::granule_size - 1 ) ) == 0,
                   "PersistentDataConfig: granule_size must be power of 2" );
    static_assert( ( pmm::EmbeddedManagerConfig::granule_size & ( pmm::EmbeddedManagerConfig::granule_size - 1 ) ) == 0,
                   "EmbeddedManagerConfig: granule_size must be power of 2" );
    static_assert( ( pmm::IndustrialDBConfig::granule_size & ( pmm::IndustrialDBConfig::granule_size - 1 ) ) == 0,
                   "IndustrialDBConfig: granule_size must be power of 2" );
    static_assert(
        ( pmm::EmbeddedStaticConfig<4096>::granule_size & ( pmm::EmbeddedStaticConfig<4096>::granule_size - 1 ) ) == 0,
        "EmbeddedStaticConfig: granule_size must be power of 2" );
    return true;
}

/// @brief EmbeddedStaticConfig использует StaticStorage (нет heap).
static bool test_i146_embedded_static_config_uses_static_storage()
{
    using Config = pmm::EmbeddedStaticConfig<4096>;
    static_assert( std::is_same<Config::storage_backend, pmm::StaticStorage<4096, pmm::DefaultAddressTraits>>::value,
                   "EmbeddedStaticConfig must use StaticStorage<4096, DefaultAddressTraits>" );
    static_assert( std::is_same<Config::lock_policy, pmm::config::NoLock>::value,
                   "EmbeddedStaticConfig must use NoLock" );
    return true;
}

/// @brief EmbeddedStaticConfig pptr размером 4 байта (32-bit адресация).
static bool test_i146_embedded_static_pptr_size()
{
    using ESH = pmm::presets::EmbeddedStaticHeap<4096>;
    static_assert( sizeof( ESH::pptr<int> ) == 4, "EmbeddedStaticHeap pptr<int> must be 4 bytes" );
    return true;
}

// =============================================================================
// Issue #146 Tests Section B: EmbeddedStaticHeap (StaticStorage, no heap)
// =============================================================================

/// @brief EmbeddedStaticHeap<4096>: базовый жизненный цикл.
static bool test_i146_embedded_static_heap_lifecycle()
{
    using ESH = pmm::PersistMemoryManager<pmm::EmbeddedStaticConfig<4096>, 1460>;

    PMM_TEST( !ESH::is_initialized() );

    // StaticStorage уже содержит буфер 4096 байт
    // create() работает когда initial_size <= BufferSize
    bool created = ESH::create( 4096 );
    PMM_TEST( created );
    PMM_TEST( ESH::is_initialized() );
    PMM_TEST( ESH::total_size() == 4096 );

    void* ptr = ESH::allocate( 64 );
    PMM_TEST( ptr != nullptr );
    std::memset( ptr, 0xAB, 64 );
    ESH::deallocate( ptr );

    ESH::destroy();
    PMM_TEST( !ESH::is_initialized() );
    return true;
}

/// @brief EmbeddedStaticHeap: typed allocation via pptr<T>.
static bool test_i146_embedded_static_heap_typed_alloc()
{
    using ESH = pmm::PersistMemoryManager<pmm::EmbeddedStaticConfig<4096>, 1461>;

    PMM_TEST( ESH::create( 4096 ) );

    ESH::pptr<int> p = ESH::allocate_typed<int>();
    PMM_TEST( !p.is_null() );
    PMM_TEST( p.offset() > 0 );

    *p.resolve() = 12345;
    PMM_TEST( *p.resolve() == 12345 );

    ESH::deallocate_typed( p );
    ESH::destroy();
    return true;
}

/// @brief EmbeddedStaticHeap: StaticStorage не расширяется (expand всегда false).
static bool test_i146_embedded_static_heap_no_expand()
{
    using ESH = pmm::PersistMemoryManager<pmm::EmbeddedStaticConfig<4096>, 1462>;

    PMM_TEST( ESH::create( 4096 ) );
    std::size_t sz_before = ESH::total_size();

    // Выделяем больше, чем осталось в пуле — expand вернёт false, allocate вернёт nullptr
    // (StaticStorage не может расшириться)
    void* large = ESH::allocate( 4080 ); // Почти весь буфер — заголовки занимают место
    // Результат зависит от overhead — просто проверяем стабильность
    if ( large != nullptr )
        ESH::deallocate( large );

    // Размер не изменился — StaticStorage
    PMM_TEST( ESH::total_size() == sz_before );

    ESH::destroy();
    return true;
}

/// @brief EmbeddedStaticHeap: множественные аллокации в статическом пуле.
static bool test_i146_embedded_static_heap_multiple_allocs()
{
    using ESH = pmm::PersistMemoryManager<pmm::EmbeddedStaticConfig<4096>, 1463>;

    PMM_TEST( ESH::create( 4096 ) );

    void* ptrs[8];
    for ( int i = 0; i < 8; ++i )
    {
        ptrs[i] = ESH::allocate( 64 );
        PMM_TEST( ptrs[i] != nullptr );
        std::memset( ptrs[i], static_cast<int>( i ), 64 );
    }

    // Проверяем, что данные корректны
    for ( int i = 0; i < 8; ++i )
    {
        std::uint8_t* bytes = static_cast<std::uint8_t*>( ptrs[i] );
        PMM_TEST( bytes[0] == static_cast<std::uint8_t>( i ) );
    }

    for ( int i = 0; i < 8; ++i )
        ESH::deallocate( ptrs[i] );

    ESH::destroy();
    return true;
}

// =============================================================================
// Issue #146 Tests Section C: Verification of all preset lock policies
// =============================================================================

/// @brief Все пресеты имеют корректные политики блокировок.
static bool test_i146_preset_lock_policies()
{
    using STH = pmm::presets::SingleThreadedHeap;
    using MTH = pmm::presets::MultiThreadedHeap;
    using EMB = pmm::presets::EmbeddedHeap;
    using IDB = pmm::presets::IndustrialDBHeap;
    using ESH = pmm::presets::EmbeddedStaticHeap<4096>;

    static_assert( std::is_same<STH::thread_policy, pmm::config::NoLock>::value, "SingleThreadedHeap must use NoLock" );
    static_assert( std::is_same<MTH::thread_policy, pmm::config::SharedMutexLock>::value,
                   "MultiThreadedHeap must use SharedMutexLock" );
    static_assert( std::is_same<EMB::thread_policy, pmm::config::NoLock>::value, "EmbeddedHeap must use NoLock" );
    static_assert( std::is_same<IDB::thread_policy, pmm::config::SharedMutexLock>::value,
                   "IndustrialDBHeap must use SharedMutexLock" );
    static_assert( std::is_same<ESH::thread_policy, pmm::config::NoLock>::value, "EmbeddedStaticHeap must use NoLock" );

    return true;
}

/// @brief Все пресеты используют DefaultAddressTraits (uint32_t, 16B granule).
static bool test_i146_preset_address_traits()
{
    using STH = pmm::presets::SingleThreadedHeap;
    using MTH = pmm::presets::MultiThreadedHeap;
    using EMB = pmm::presets::EmbeddedHeap;
    using IDB = pmm::presets::IndustrialDBHeap;
    using ESH = pmm::presets::EmbeddedStaticHeap<4096>;

    static_assert( std::is_same<STH::address_traits, pmm::DefaultAddressTraits>::value,
                   "SingleThreadedHeap must use DefaultAddressTraits" );
    static_assert( std::is_same<MTH::address_traits, pmm::DefaultAddressTraits>::value,
                   "MultiThreadedHeap must use DefaultAddressTraits" );
    static_assert( std::is_same<EMB::address_traits, pmm::DefaultAddressTraits>::value,
                   "EmbeddedHeap must use DefaultAddressTraits" );
    static_assert( std::is_same<IDB::address_traits, pmm::DefaultAddressTraits>::value,
                   "IndustrialDBHeap must use DefaultAddressTraits" );
    static_assert( std::is_same<ESH::address_traits, pmm::DefaultAddressTraits>::value,
                   "EmbeddedStaticHeap must use DefaultAddressTraits" );

    return true;
}

// =============================================================================
// Issue #146 Tests Section D: Growth rate verification
// =============================================================================

/// @brief Конфигурации имеют корректные коэффициенты роста.
static bool test_i146_grow_ratios()
{
    // CacheManagerConfig: 25% growth
    static_assert( pmm::CacheManagerConfig::grow_numerator == pmm::config::kDefaultGrowNumerator,
                   "CacheManagerConfig grow_numerator must be kDefaultGrowNumerator" );
    static_assert( pmm::CacheManagerConfig::grow_denominator == pmm::config::kDefaultGrowDenominator,
                   "CacheManagerConfig grow_denominator must be kDefaultGrowDenominator" );

    // PersistentDataConfig: 25% growth
    static_assert( pmm::PersistentDataConfig::grow_numerator == pmm::config::kDefaultGrowNumerator,
                   "PersistentDataConfig grow_numerator must be kDefaultGrowNumerator" );
    static_assert( pmm::PersistentDataConfig::grow_denominator == pmm::config::kDefaultGrowDenominator,
                   "PersistentDataConfig grow_denominator must be kDefaultGrowDenominator" );

    // EmbeddedManagerConfig: 50% growth (3/2)
    static_assert( pmm::EmbeddedManagerConfig::grow_numerator == 3, "EmbeddedManagerConfig grow_numerator must be 3" );
    static_assert( pmm::EmbeddedManagerConfig::grow_denominator == 2,
                   "EmbeddedManagerConfig grow_denominator must be 2" );

    // IndustrialDBConfig: 100% growth (2/1)
    static_assert( pmm::IndustrialDBConfig::grow_numerator == 2, "IndustrialDBConfig grow_numerator must be 2" );
    static_assert( pmm::IndustrialDBConfig::grow_denominator == 1, "IndustrialDBConfig grow_denominator must be 1" );

    // EmbeddedStaticConfig: 50% growth (3/2, but not used since StaticStorage doesn't expand)
    static_assert( pmm::EmbeddedStaticConfig<4096>::grow_numerator == 3,
                   "EmbeddedStaticConfig grow_numerator must be 3" );
    static_assert( pmm::EmbeddedStaticConfig<4096>::grow_denominator == 2,
                   "EmbeddedStaticConfig grow_denominator must be 2" );

    return true;
}

// =============================================================================
// Issue #146 Tests Section E: EmbeddedStaticHeap preset via pmm_presets.h
// =============================================================================

/// @brief EmbeddedStaticHeap<8192> — использование пресета с нестандартным размером.
static bool test_i146_embedded_static_heap_preset_8k()
{
    // Используем уникальный InstanceId чтобы не конфликтовать с другими тестами
    using ESH8K = pmm::PersistMemoryManager<pmm::EmbeddedStaticConfig<8192>, 1470>;

    PMM_TEST( !ESH8K::is_initialized() );
    PMM_TEST( ESH8K::create( 8192 ) );
    PMM_TEST( ESH8K::is_initialized() );
    PMM_TEST( ESH8K::total_size() == 8192 );

    ESH8K::pptr<double> p = ESH8K::allocate_typed<double>();
    PMM_TEST( !p.is_null() );
    *p.resolve() = 3.14159;
    PMM_TEST( *p.resolve() == 3.14159 );

    ESH8K::deallocate_typed( p );
    ESH8K::destroy();
    PMM_TEST( !ESH8K::is_initialized() );
    return true;
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "=== test_issue146_configs (Issue #146: Config rethinking) ===\n\n";
    bool all_passed = true;

    std::cout << "--- I146-A: Static compile-time checks ---\n";
    PMM_RUN( "I146-A1: kMinGranuleSize == 4 (architecture word size)", test_i146_min_granule_size_constant );
    PMM_RUN( "I146-A2: All configs have granule_size >= kMinGranuleSize", test_i146_all_configs_granule_size_valid );
    PMM_RUN( "I146-A3: All configs have granule_size as power of 2", test_i146_all_configs_granule_power_of_two );
    PMM_RUN( "I146-A4: EmbeddedStaticConfig uses StaticStorage (no heap)",
             test_i146_embedded_static_config_uses_static_storage );
    PMM_RUN( "I146-A5: EmbeddedStaticHeap pptr size == 4 bytes", test_i146_embedded_static_pptr_size );

    std::cout << "\n--- I146-B: EmbeddedStaticHeap (StaticStorage, no heap) ---\n";
    PMM_RUN( "I146-B1: EmbeddedStaticHeap<4096> lifecycle", test_i146_embedded_static_heap_lifecycle );
    PMM_RUN( "I146-B2: EmbeddedStaticHeap<4096> typed allocation", test_i146_embedded_static_heap_typed_alloc );
    PMM_RUN( "I146-B3: EmbeddedStaticHeap<4096> no expand (StaticStorage)", test_i146_embedded_static_heap_no_expand );
    PMM_RUN( "I146-B4: EmbeddedStaticHeap<4096> multiple allocations", test_i146_embedded_static_heap_multiple_allocs );

    std::cout << "\n--- I146-C: Preset lock policies and address traits ---\n";
    PMM_RUN( "I146-C1: All preset lock policies are correct", test_i146_preset_lock_policies );
    PMM_RUN( "I146-C2: All presets use DefaultAddressTraits", test_i146_preset_address_traits );

    std::cout << "\n--- I146-D: Growth rate verification ---\n";
    PMM_RUN( "I146-D1: All config grow ratios are correct", test_i146_grow_ratios );

    std::cout << "\n--- I146-E: EmbeddedStaticHeap preset variants ---\n";
    PMM_RUN( "I146-E1: EmbeddedStaticHeap<8192> preset lifecycle", test_i146_embedded_static_heap_preset_8k );

    std::cout << "\n" << ( all_passed ? "All tests PASSED\n" : "Some tests FAILED\n" );
    return all_passed ? 0 : 1;
}
