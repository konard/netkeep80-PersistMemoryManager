/**
 * @file test_issue100.cpp
 * @brief Tests for Issue #100: Phase 1 — Infrastructure Preparation.
 *
 * Tests:
 *   P100-A: pptr<T, ManagerT> — two-parameter persistent pointer
 *     - sizeof(pptr<T, ManagerT>) == 4 (ManagerT not stored)
 *     - pptr<T, ManagerT>::resolve(mgr) == mgr.resolve<T>(p)
 *     - element_type and manager_type typedefs
 *
 *   P100-B: AbstractPersistMemoryManager — manager_type and nested pptr<T>
 *     - manager_type typedef refers to own manager type
 *     - Nested alias Manager::pptr<T> == pmm::pptr<T, manager_type>
 *     - allocate_typed returns Manager::pptr<T>
 *     - resolve() and deallocate_typed() work with Manager::pptr<T>
 *     - Full lifecycle with Manager::pptr<T>
 *
 *   P100-C: manager_concept.h — is_persist_memory_manager<T>
 *     - Validates AbstractPersistMemoryManager through concept
 *     - Validates presets through concept
 *     - Rejects int and non-manager types
 *
 *   P100-D: static_manager_factory.h — StaticPersistMemoryManager<ConfigT, Tag>
 *     - Different tags = different types (TypeA::pptr<int> != TypeB::pptr<int>)
 *     - Full lifecycle of StaticPersistMemoryManager
 *     - StaticPersistMemoryManager satisfies is_persist_memory_manager_v
 *
 *   P100-E: manager_configs.h — ready-made configurations
 *     - CacheManagerConfig (NoLock)
 *     - PersistentDataConfig (SharedMutexLock)
 *     - EmbeddedManagerConfig (NoLock)
 *     - IndustrialDBConfig (SharedMutexLock)
 *
 * Note: Issue #102 removed pptr<T, void> backward compatibility.
 * This test file tests only the new two-parameter pptr<T, ManagerT> API.
 *
 * @see include/pmm/pptr.h
 * @see include/pmm/abstract_pmm.h
 * @see include/pmm/manager_concept.h
 * @see include/pmm/static_manager_factory.h
 * @see include/pmm/manager_configs.h
 * @version 0.2 (Issue #102 — updated for new API, removed pptr<T,void> backward compat)
 */

#include "pmm/abstract_pmm.h"
#include "pmm/manager_concept.h"
#include "pmm/manager_configs.h"
#include "pmm/pmm_presets.h"
#include "pmm/pptr.h"
#include "pmm/static_manager_factory.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>

// ─── Test macros ─────────────────────────────────────────────────────────────

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
        bool _result = fn();                                                                                           \
        std::cout << ( _result ? "PASS" : "FAIL" ) << "\n";                                                            \
        if ( !_result )                                                                                                \
            all_passed = false;                                                                                        \
    } while ( false )

// =============================================================================
// P100-A: pptr<T, ManagerT> — two-parameter persistent pointer
// =============================================================================

/// @brief pptr<T, ManagerT> stores only 4 bytes — ManagerT is not stored.
static bool test_p100_pptr_sizeof()
{
    using MgrType = pmm::presets::SingleThreadedHeap;

    // With manager — size does NOT change (ManagerT is not stored)
    static_assert( sizeof( pmm::pptr<int, MgrType> ) == 4, "sizeof(pptr<int,MgrType>) must be 4" );
    static_assert( sizeof( pmm::pptr<double, MgrType> ) == 4, "sizeof(pptr<double,MgrType>) must be 4" );
    static_assert( sizeof( MgrType::pptr<int> ) == 4, "sizeof(Manager::pptr<int>) must be 4" );

    PMM_TEST( sizeof( pmm::pptr<int, MgrType> ) == 4 );
    PMM_TEST( sizeof( MgrType::pptr<int> ) == 4 );
    return true;
}

/// @brief pptr<T, ManagerT> has typedefs element_type and manager_type.
static bool test_p100_pptr_typedefs()
{
    using MgrType = pmm::presets::SingleThreadedHeap;

    // element_type
    static_assert( std::is_same_v<pmm::pptr<int, MgrType>::element_type, int> );
    static_assert( std::is_same_v<pmm::pptr<double, MgrType>::element_type, double> );

    // manager_type
    static_assert( std::is_same_v<pmm::pptr<int, MgrType>::manager_type, MgrType> );
    static_assert( std::is_same_v<MgrType::pptr<int>::manager_type, MgrType> );

    return true;
}

/// @brief pptr<T, ManagerT>::resolve(mgr) == mgr.resolve<T>(p).
static bool test_p100_pptr_resolve_method()
{
    pmm::presets::SingleThreadedHeap pmm;
    PMM_TEST( pmm.create( 16 * 1024 ) );

    using MyPptr = pmm::presets::SingleThreadedHeap::pptr<int>;

    MyPptr p = pmm.allocate_typed<int>();
    PMM_TEST( !p.is_null() );

    // Dereference via pptr::resolve(mgr)
    int* ptr1 = p.resolve( pmm );
    PMM_TEST( ptr1 != nullptr );

    // Equivalent to mgr.resolve<T>(p)
    int* ptr2 = pmm.resolve( p );
    PMM_TEST( ptr2 != nullptr );

    // Both point to the same location
    PMM_TEST( ptr1 == ptr2 );

    // Read/write via both methods
    *ptr1 = 42;
    PMM_TEST( *ptr2 == 42 );

    *ptr2 = 99;
    PMM_TEST( *p.resolve( pmm ) == 99 );

    pmm.deallocate_typed( p );
    pmm.destroy();
    return true;
}

/// @brief Null pptr<T, ManagerT> is correctly initialized and detected.
static bool test_p100_pptr_null()
{
    using MgrType = pmm::presets::SingleThreadedHeap;

    MgrType::pptr<int> null_ptr;
    PMM_TEST( null_ptr.is_null() );
    PMM_TEST( !static_cast<bool>( null_ptr ) );
    PMM_TEST( null_ptr.offset() == 0 );

    MgrType::pptr<double> null_double;
    PMM_TEST( null_double.is_null() );
    PMM_TEST( null_double.offset() == 0 );

    return true;
}

// =============================================================================
// P100-B: AbstractPersistMemoryManager — manager_type and nested pptr<T>
// =============================================================================

/// @brief Manager::manager_type refers to the manager's own type.
static bool test_p100_manager_type_typedef()
{
    using MgrType = pmm::presets::SingleThreadedHeap;

    static_assert( std::is_same_v<MgrType::manager_type, MgrType>,
                   "Manager::manager_type must be the manager's own type" );

    static_assert( std::is_same_v<pmm::DefaultAbstractPMM::manager_type, pmm::DefaultAbstractPMM>,
                   "DefaultAbstractPMM::manager_type must be DefaultAbstractPMM" );

    return true;
}

/// @brief Manager::pptr<T> == pmm::pptr<T, manager_type>.
static bool test_p100_nested_pptr_alias()
{
    using MgrType = pmm::presets::SingleThreadedHeap;

    // Manager::pptr<T> must be pmm::pptr<T, MgrType>
    static_assert( std::is_same_v<MgrType::pptr<int>, pmm::pptr<int, MgrType>>,
                   "Manager::pptr<int> must be pmm::pptr<int, manager_type>" );

    static_assert( std::is_same_v<MgrType::pptr<double>, pmm::pptr<double, MgrType>>,
                   "Manager::pptr<double> must be pmm::pptr<double, manager_type>" );

    // Different manager types have different pptr<T>
    using MgrType2 = pmm::presets::MultiThreadedHeap;
    static_assert( !std::is_same_v<MgrType::pptr<int>, MgrType2::pptr<int>>,
                   "pptr from different manager types must be different types" );

    return true;
}

/// @brief allocate_typed returns Manager::pptr<T>.
static bool test_p100_allocate_typed_returns_manager_pptr()
{
    pmm::presets::SingleThreadedHeap pmm;
    PMM_TEST( pmm.create( 32 * 1024 ) );

    using MgrType = pmm::presets::SingleThreadedHeap;

    // Store in Manager::pptr<T>
    MgrType::pptr<int> p1 = pmm.allocate_typed<int>();
    PMM_TEST( !p1.is_null() );

    MgrType::pptr<int> p2 = pmm.allocate_typed<int>();
    PMM_TEST( !p2.is_null() );

    // Resolve and write
    *pmm.resolve( p1 ) = 111;
    *pmm.resolve( p2 ) = 222;

    PMM_TEST( *pmm.resolve( p1 ) == 111 );
    PMM_TEST( *pmm.resolve( p2 ) == 222 );

    // Different addresses
    PMM_TEST( p1.offset() != p2.offset() );

    pmm.deallocate_typed( p1 );
    pmm.deallocate_typed( p2 );
    pmm.destroy();
    return true;
}

/// @brief Full lifecycle: allocate/write/read/deallocate with Manager::pptr<T>.
static bool test_p100_full_lifecycle_with_manager_pptr()
{
    pmm::presets::SingleThreadedHeap pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );

    using MgrPptr = pmm::presets::SingleThreadedHeap::pptr<int>;

    // Allocate array of 5 elements
    MgrPptr arr = pmm.allocate_typed<int>( 5 );
    PMM_TEST( !arr.is_null() );

    // Write via resolve_at
    for ( int i = 0; i < 5; ++i )
    {
        int* elem = pmm.resolve_at( arr, static_cast<std::size_t>( i ) );
        PMM_TEST( elem != nullptr );
        *elem = i * 10;
    }

    // Read via resolve
    int* base = pmm.resolve( arr );
    PMM_TEST( base != nullptr );
    for ( int i = 0; i < 5; ++i )
        PMM_TEST( base[i] == i * 10 );

    // Dereference via pptr::resolve(mgr)
    int* base2 = arr.resolve( pmm );
    PMM_TEST( base2 == base );

    // Deallocate
    pmm.deallocate_typed( arr );
    pmm.destroy();
    return true;
}

// =============================================================================
// P100-C: manager_concept.h — is_persist_memory_manager<T>
// =============================================================================

/// @brief AbstractPersistMemoryManager satisfies the concept.
static bool test_p100_concept_abstract_pmm()
{
    // AbstractPersistMemoryManager satisfies the concept
    static_assert( pmm::is_persist_memory_manager_v<pmm::DefaultAbstractPMM>,
                   "DefaultAbstractPMM must satisfy is_persist_memory_manager" );

    static_assert( pmm::is_persist_memory_manager_v<pmm::SingleThreadedAbstractPMM>,
                   "SingleThreadedAbstractPMM must satisfy is_persist_memory_manager" );

    PMM_TEST( pmm::is_persist_memory_manager_v<pmm::DefaultAbstractPMM> );
    PMM_TEST( pmm::is_persist_memory_manager_v<pmm::SingleThreadedAbstractPMM> );
    return true;
}

/// @brief pmm_presets satisfy the concept.
static bool test_p100_concept_presets()
{
    static_assert( pmm::is_persist_memory_manager_v<pmm::presets::SingleThreadedHeap> );
    static_assert( pmm::is_persist_memory_manager_v<pmm::presets::MultiThreadedHeap> );
    static_assert( pmm::is_persist_memory_manager_v<pmm::presets::EmbeddedStatic4K> );
    static_assert( pmm::is_persist_memory_manager_v<pmm::presets::PersistentFileMapped> );
    static_assert( pmm::is_persist_memory_manager_v<pmm::presets::IndustrialDB> );

    PMM_TEST( pmm::is_persist_memory_manager_v<pmm::presets::SingleThreadedHeap> );
    PMM_TEST( pmm::is_persist_memory_manager_v<pmm::presets::MultiThreadedHeap> );
    PMM_TEST( pmm::is_persist_memory_manager_v<pmm::presets::EmbeddedStatic4K> );
    return true;
}

/// @brief Non-manager types do not satisfy the concept.
static bool test_p100_concept_rejects_non_managers()
{
    static_assert( !pmm::is_persist_memory_manager_v<int>, "int must not satisfy is_persist_memory_manager" );

    static_assert( !pmm::is_persist_memory_manager_v<double>, "double must not satisfy is_persist_memory_manager" );

    struct NotAManager
    {
        void foo() {}
    };
    static_assert( !pmm::is_persist_memory_manager_v<NotAManager>,
                   "NotAManager must not satisfy is_persist_memory_manager" );

    PMM_TEST( !pmm::is_persist_memory_manager_v<int> );
    PMM_TEST( !pmm::is_persist_memory_manager_v<double> );
    return true;
}

// =============================================================================
// P100-D: static_manager_factory.h — StaticPersistMemoryManager<ConfigT, Tag>
// =============================================================================

/// @brief Different tags create different manager types and different pptr types.
static bool test_p100_static_factory_different_tags()
{
    struct TagA
    {
    };
    struct TagB
    {
    };

    using MgrA = pmm::StaticPersistMemoryManager<pmm::CacheManagerConfig, TagA>;
    using MgrB = pmm::StaticPersistMemoryManager<pmm::CacheManagerConfig, TagB>;

    // Different manager types
    static_assert( !std::is_same_v<MgrA, MgrB>, "Different tags must produce different manager types" );

    // Different pptr types
    static_assert( !std::is_same_v<MgrA::pptr<int>, MgrB::pptr<int>>,
                   "pptr from different manager tags must be different types" );

    // Same pptr sizes
    static_assert( sizeof( MgrA::pptr<int> ) == 4 );
    static_assert( sizeof( MgrB::pptr<int> ) == 4 );

    PMM_TEST( sizeof( MgrA::pptr<int> ) == 4 );
    PMM_TEST( sizeof( MgrB::pptr<int> ) == 4 );
    return true;
}

/// @brief Full lifecycle of StaticPersistMemoryManager.
static bool test_p100_static_factory_lifecycle()
{
    struct MyTag
    {
    };
    using MyMgr = pmm::StaticPersistMemoryManager<pmm::CacheManagerConfig, MyTag>;

    MyMgr mgr;
    PMM_TEST( !mgr.is_initialized() );

    PMM_TEST( mgr.create( 32 * 1024 ) );
    PMM_TEST( mgr.is_initialized() );
    PMM_TEST( mgr.total_size() >= 32 * 1024 );
    PMM_TEST( mgr.free_size() > 0 );

    // Allocate via typed API
    MyMgr::pptr<int> p = mgr.allocate_typed<int>();
    PMM_TEST( !p.is_null() );

    // Dereference via pptr method (requires manager instance)
    *p.resolve( mgr ) = 123;
    PMM_TEST( *p.resolve( mgr ) == 123 );

    // Dereference via manager method
    PMM_TEST( *mgr.resolve( p ) == 123 );

    // Deallocate
    mgr.deallocate_typed( p );
    mgr.destroy();
    PMM_TEST( !mgr.is_initialized() );

    return true;
}

/// @brief StaticPersistMemoryManager satisfies is_persist_memory_manager_v.
static bool test_p100_static_factory_concept()
{
    struct MyTag
    {
    };
    using MyMgr = pmm::StaticPersistMemoryManager<pmm::CacheManagerConfig, MyTag>;

    static_assert( pmm::is_persist_memory_manager_v<MyMgr>,
                   "StaticPersistMemoryManager must satisfy is_persist_memory_manager" );
    static_assert( pmm::is_persist_memory_manager_v<pmm::StaticPersistMemoryManager<>>,
                   "StaticPersistMemoryManager<> must satisfy is_persist_memory_manager" );

    PMM_TEST( pmm::is_persist_memory_manager_v<MyMgr> );
    PMM_TEST( pmm::is_persist_memory_manager_v<pmm::StaticPersistMemoryManager<>> );
    return true;
}

/// @brief Two StaticPersistMemoryManager instances with different tags work independently.
static bool test_p100_static_factory_multiple_instances()
{
    struct CacheTag
    {
    };
    struct DataTag
    {
    };

    using CacheMgr = pmm::StaticPersistMemoryManager<pmm::CacheManagerConfig, CacheTag>;
    using DataMgr  = pmm::StaticPersistMemoryManager<pmm::PersistentDataConfig, DataTag>;

    CacheMgr cache;
    DataMgr  data;

    PMM_TEST( cache.create( 16 * 1024 ) );
    PMM_TEST( data.create( 32 * 1024 ) );

    CacheMgr::pptr<int> cp = cache.allocate_typed<int>();
    DataMgr::pptr<int>  dp = data.allocate_typed<int>();

    PMM_TEST( !cp.is_null() );
    PMM_TEST( !dp.is_null() );

    *cp.resolve( cache ) = 111;
    *dp.resolve( data )  = 222;

    PMM_TEST( *cp.resolve( cache ) == 111 );
    PMM_TEST( *dp.resolve( data ) == 222 );

    // cp and dp are different types (CacheMgr::pptr<int> != DataMgr::pptr<int>)
    static_assert( !std::is_same_v<CacheMgr::pptr<int>, DataMgr::pptr<int>> );

    cache.deallocate_typed( cp );
    data.deallocate_typed( dp );

    cache.destroy();
    data.destroy();
    return true;
}

// =============================================================================
// P100-E: manager_configs.h — ready-made configurations
// =============================================================================

/// @brief CacheManagerConfig uses NoLock.
static bool test_p100_configs_cache()
{
    static_assert( std::is_same_v<pmm::CacheManagerConfig::lock_policy, pmm::config::NoLock>,
                   "CacheManagerConfig must use NoLock" );
    static_assert( pmm::CacheManagerConfig::granule_size == 16 );
    static_assert( pmm::CacheManagerConfig::max_memory_gb == 64 );

    struct CacheTag
    {
    };
    using CacheMgr = pmm::StaticPersistMemoryManager<pmm::CacheManagerConfig, CacheTag>;

    CacheMgr mgr;
    PMM_TEST( mgr.create( 8 * 1024 ) );

    CacheMgr::pptr<double> p = mgr.allocate_typed<double>();
    PMM_TEST( !p.is_null() );
    *p.resolve( mgr ) = 3.14;
    PMM_TEST( *p.resolve( mgr ) == 3.14 );

    mgr.deallocate_typed( p );
    mgr.destroy();
    return true;
}

/// @brief PersistentDataConfig uses SharedMutexLock.
static bool test_p100_configs_persistent()
{
    static_assert( std::is_same_v<pmm::PersistentDataConfig::lock_policy, pmm::config::SharedMutexLock>,
                   "PersistentDataConfig must use SharedMutexLock" );

    struct PDataTag
    {
    };
    using PDataMgr = pmm::StaticPersistMemoryManager<pmm::PersistentDataConfig, PDataTag>;

    PDataMgr mgr;
    PMM_TEST( mgr.create( 16 * 1024 ) );

    PDataMgr::pptr<std::uint64_t> p = mgr.allocate_typed<std::uint64_t>();
    PMM_TEST( !p.is_null() );
    *p.resolve( mgr ) = 0xDEADBEEFCAFEBABEull;
    PMM_TEST( *p.resolve( mgr ) == 0xDEADBEEFCAFEBABEull );

    mgr.deallocate_typed( p );
    mgr.destroy();
    return true;
}

/// @brief EmbeddedManagerConfig uses NoLock with conservative growth.
static bool test_p100_configs_embedded()
{
    static_assert( std::is_same_v<pmm::EmbeddedManagerConfig::lock_policy, pmm::config::NoLock>,
                   "EmbeddedManagerConfig must use NoLock" );
    static_assert( pmm::EmbeddedManagerConfig::grow_numerator == 3 );
    static_assert( pmm::EmbeddedManagerConfig::grow_denominator == 2 );

    struct EmbTag
    {
    };
    using EmbMgr = pmm::StaticPersistMemoryManager<pmm::EmbeddedManagerConfig, EmbTag>;

    EmbMgr mgr;
    PMM_TEST( mgr.create( 4 * 1024 ) );
    PMM_TEST( mgr.is_initialized() );

    EmbMgr::pptr<char> p = mgr.allocate_typed<char>( 16 );
    PMM_TEST( !p.is_null() );
    std::memcpy( p.resolve( mgr ), "hello world!", 12 );
    PMM_TEST( std::memcmp( p.resolve( mgr ), "hello world!", 12 ) == 0 );

    mgr.deallocate_typed( p );
    mgr.destroy();
    return true;
}

/// @brief IndustrialDBConfig uses SharedMutexLock with aggressive growth.
static bool test_p100_configs_industrial()
{
    static_assert( std::is_same_v<pmm::IndustrialDBConfig::lock_policy, pmm::config::SharedMutexLock>,
                   "IndustrialDBConfig must use SharedMutexLock" );
    static_assert( pmm::IndustrialDBConfig::grow_numerator == 2 );
    static_assert( pmm::IndustrialDBConfig::grow_denominator == 1 );

    struct DBTag
    {
    };
    using DBMgr = pmm::StaticPersistMemoryManager<pmm::IndustrialDBConfig, DBTag>;

    DBMgr mgr;
    PMM_TEST( mgr.create( 64 * 1024 ) );
    PMM_TEST( mgr.is_initialized() );

    // Allocate multiple elements
    DBMgr::pptr<int> p1 = mgr.allocate_typed<int>();
    DBMgr::pptr<int> p2 = mgr.allocate_typed<int>();
    PMM_TEST( !p1.is_null() );
    PMM_TEST( !p2.is_null() );
    PMM_TEST( p1 != p2 );

    *p1.resolve( mgr ) = 1;
    *p2.resolve( mgr ) = 2;
    PMM_TEST( *p1.resolve( mgr ) == 1 );
    PMM_TEST( *p2.resolve( mgr ) == 2 );

    mgr.deallocate_typed( p1 );
    mgr.deallocate_typed( p2 );
    mgr.destroy();
    return true;
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "=== test_issue100 (Issue #100: Phase 1 Infrastructure Preparation, updated #102) ===\n\n";
    bool all_passed = true;

    std::cout << "--- P100-A: pptr<T, ManagerT> (two-parameter persistent pointer) ---\n";
    PMM_RUN( "P100-A1: sizeof(pptr<T, ManagerT>) == 4 (ManagerT not stored)", test_p100_pptr_sizeof );
    PMM_RUN( "P100-A2: pptr<T,ManagerT> typedefs element_type and manager_type", test_p100_pptr_typedefs );
    PMM_RUN( "P100-A3: pptr<T,ManagerT>::resolve(mgr) == mgr.resolve<T>(p)", test_p100_pptr_resolve_method );
    PMM_RUN( "P100-A4: null pptr<T,ManagerT> correctly initialized", test_p100_pptr_null );

    std::cout << "\n--- P100-B: AbstractPersistMemoryManager — manager_type and nested pptr<T> ---\n";
    PMM_RUN( "P100-B1: Manager::manager_type == Manager", test_p100_manager_type_typedef );
    PMM_RUN( "P100-B2: Manager::pptr<T> == pmm::pptr<T, manager_type>", test_p100_nested_pptr_alias );
    PMM_RUN( "P100-B3: allocate_typed returns Manager::pptr<T>", test_p100_allocate_typed_returns_manager_pptr );
    PMM_RUN( "P100-B4: full lifecycle with Manager::pptr<T>", test_p100_full_lifecycle_with_manager_pptr );

    std::cout << "\n--- P100-C: manager_concept.h — is_persist_memory_manager<T> ---\n";
    PMM_RUN( "P100-C1: AbstractPersistMemoryManager satisfies concept", test_p100_concept_abstract_pmm );
    PMM_RUN( "P100-C2: pmm_presets satisfy concept", test_p100_concept_presets );
    PMM_RUN( "P100-C3: non-manager types rejected by concept", test_p100_concept_rejects_non_managers );

    std::cout << "\n--- P100-D: static_manager_factory.h — StaticPersistMemoryManager<ConfigT, Tag> ---\n";
    PMM_RUN( "P100-D1: different tags = different manager types and pptr", test_p100_static_factory_different_tags );
    PMM_RUN( "P100-D2: full lifecycle of StaticPersistMemoryManager", test_p100_static_factory_lifecycle );
    PMM_RUN( "P100-D3: StaticPersistMemoryManager satisfies concept", test_p100_static_factory_concept );
    PMM_RUN( "P100-D4: multiple instances with different tags work independently",
             test_p100_static_factory_multiple_instances );

    std::cout << "\n--- P100-E: manager_configs.h — ready-made configurations ---\n";
    PMM_RUN( "P100-E1: CacheManagerConfig (NoLock)", test_p100_configs_cache );
    PMM_RUN( "P100-E2: PersistentDataConfig (SharedMutexLock)", test_p100_configs_persistent );
    PMM_RUN( "P100-E3: EmbeddedManagerConfig (NoLock, conservative growth)", test_p100_configs_embedded );
    PMM_RUN( "P100-E4: IndustrialDBConfig (SharedMutexLock, aggressive growth)", test_p100_configs_industrial );

    std::cout << "\n" << ( all_passed ? "All tests PASSED\n" : "Some tests FAILED\n" );
    return all_passed ? 0 : 1;
}
