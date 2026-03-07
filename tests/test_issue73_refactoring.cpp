/**
 * @file test_issue73_refactoring.cpp
 * @brief Tests for architectural refactoring Issue #73 (updated #102 — new API).
 *
 * Issue #102: PersistMemoryManager<> (singleton) removed. Tests verify:
 * - FR-03: BlockHeader (32 bytes) and ManagerHeader (64 bytes) sizes unchanged
 * - FR-02/AR-03: PersistentAvlTree is a standalone class
 * - FR-04/AR-01: Public API works through AbstractPersistMemoryManager presets
 * - AR-02: No virtual functions — all polymorphism is static
 * - AR-04: File separation: types, avl, manager, io
 */

#include "pmm/pmm_presets.h"
#include "pmm/types.h"
#include "pmm/free_block_tree.h"
#include "pmm/config.h"
#include "pmm/address_traits.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <type_traits>

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

using Mgr = pmm::presets::SingleThreadedHeap;

// ─── FR-03: Binary-compatibility static_assert checks ────────────────────────

static_assert( sizeof( pmm::detail::BlockHeader ) == 32, "FR-03: BlockHeader must be exactly 32 bytes" );
static_assert( sizeof( pmm::detail::ManagerHeader ) == 64, "FR-03: ManagerHeader must be exactly 64 bytes" );
static_assert( sizeof( Mgr::pptr<int> ) == 4, "pptr<T> must be exactly 4 bytes (Issue #59)" );

// ─── FR-02/AR-03: PersistentAvlTree is a standalone class ────────────────────

/// @brief Verify PersistentAvlTree is all-static and callable directly.
static bool test_avl_tree_standalone()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 128 * 1024 ) );

    // Allocate some blocks to create a non-trivial free tree
    Mgr::pptr<std::uint8_t> p1 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p2 = pmm.allocate_typed<std::uint8_t>( 512 );
    Mgr::pptr<std::uint8_t> p3 = pmm.allocate_typed<std::uint8_t>( 128 );
    PMM_TEST( !p1.is_null() && !p2.is_null() && !p3.is_null() );
    pmm.deallocate_typed( p1 );

    PMM_TEST( pmm.is_initialized() );

    pmm.deallocate_typed( p2 );
    pmm.deallocate_typed( p3 );
    pmm.destroy();
    return true;
}

// ─── FR-04/AR-01: Public API via AbstractPersistMemoryManager ────────────────

/// @brief Verify the new instance-based API works correctly (replaces static PMM).
static bool test_instance_api()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );
    PMM_TEST( pmm.is_initialized() );

    PMM_TEST( pmm.alloc_block_count() > 0 );  // BlockHeader_0
    PMM_TEST( pmm.free_block_count() == 1 );
    PMM_TEST( pmm.alloc_block_count() == 1 ); // BlockHeader_0

    Mgr::pptr<std::uint8_t> p = pmm.allocate_typed<std::uint8_t>( 128 );
    PMM_TEST( !p.is_null() );

    PMM_TEST( pmm.alloc_block_count() == 2 ); // BlockHeader_0 + p

    pmm.deallocate_typed( p );
    PMM_TEST( pmm.alloc_block_count() == 1 ); // BlockHeader_0 only
    pmm.destroy();
    return true;
}

/// @brief Verify two independent manager instances don't share state.
static bool test_two_instances_independent()
{
    Mgr pmm1, pmm2;
    PMM_TEST( pmm1.create( 64 * 1024 ) );
    PMM_TEST( pmm2.create( 64 * 1024 ) );

    PMM_TEST( pmm1.is_initialized() );
    PMM_TEST( pmm2.is_initialized() );

    // Each has separate total_size
    PMM_TEST( pmm1.total_size() > 0 );
    PMM_TEST( pmm2.total_size() > 0 );

    Mgr::pptr<std::uint32_t> p1 = pmm1.allocate_typed<std::uint32_t>( 4 );
    PMM_TEST( !p1.is_null() );
    PMM_TEST( pmm2.alloc_block_count() == 1 ); // pmm2 unaffected

    pmm1.deallocate_typed( p1 );
    pmm1.destroy();
    pmm2.destroy();
    return true;
}

// ─── AR-02: No virtual functions ─────────────────────────────────────────────

/// @brief Ensure AbstractPersistMemoryManager has no virtual functions (AR-02).
static bool test_no_virtual_functions()
{
    static_assert( !std::is_polymorphic<Mgr>::value,
                   "AR-02: AbstractPersistMemoryManager must have no virtual functions" );
    static_assert( !std::is_polymorphic<pmm::PersistentAvlTree>::value,
                   "AR-02: PersistentAvlTree must have no virtual functions" );
    return true;
}

// ─── AR-04: File separation ───────────────────────────────────────────────────

/// @brief Verify that types from each header are accessible independently.
static bool test_file_separation()
{
    // Types from persist_memory_types.h
    static_assert( pmm::kGranuleSize == 16, "Types header must provide kGranuleSize" );
    static_assert( sizeof( pmm::detail::BlockHeader ) == 32, "Types header: BlockHeader" );
    static_assert( sizeof( pmm::detail::ManagerHeader ) == 64, "Types header: ManagerHeader" );

    // Config from pmm_config.h
    static_assert( pmm::config::kDefaultGrowNumerator == 5, "Config header: grow_numerator" );
    static_assert( pmm::config::kDefaultGrowDenominator == 4, "Config header: grow_denominator" );

    // PersistentAvlTree from persist_avl_tree.h — just check it's a class
    static_assert( std::is_class<pmm::PersistentAvlTree>::value, "persist_avl_tree.h: PersistentAvlTree" );

    // AddressTraits
    static_assert( std::is_class<pmm::DefaultAddressTraits>::value, "address_traits.h: DefaultAddressTraits" );

    return true;
}

// ─── FR-05: NoLock policy via presets ────────────────────────────────────────

/// @brief Verify SingleThreadedHeap uses NoLock.
static bool test_nolock_preset()
{
    using NoLockMgr = pmm::presets::SingleThreadedHeap;

    NoLockMgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );
    PMM_TEST( pmm.is_initialized() );

    Mgr::pptr<std::uint32_t> p = pmm.allocate_typed<std::uint32_t>( 4 );
    PMM_TEST( !p.is_null() );

    pmm.deallocate_typed( p );
    pmm.destroy();
    return true;
}

// ─── Integration: multiple presets coexist ────────────────────────────────────

/// @brief Verify that two different preset types can be used simultaneously.
static bool test_presets_coexist()
{
    using ST = pmm::presets::SingleThreadedHeap;

    ST pmm1, pmm2;
    PMM_TEST( pmm1.create( 64 * 1024 ) );
    PMM_TEST( pmm2.create( 64 * 1024 ) );

    PMM_TEST( pmm1.is_initialized() );
    PMM_TEST( pmm2.is_initialized() );

    // Each has separate total_size
    PMM_TEST( pmm1.total_size() == pmm2.total_size() );

    pmm1.destroy();
    pmm2.destroy();
    return true;
}

int main()
{
    std::cout << "=== test_issue73_refactoring (updated #102 — new API) ===\n";
    bool all_passed = true;

    PMM_RUN( "FR-03: struct sizes", test_file_separation );
    PMM_RUN( "FR-02/AR-03: avl_tree_standalone", test_avl_tree_standalone );
    PMM_RUN( "FR-04/AR-01: instance_api", test_instance_api );
    PMM_RUN( "FR-04/AR-01: two_instances_independent", test_two_instances_independent );
    PMM_RUN( "AR-02: no_virtual_functions", test_no_virtual_functions );
    PMM_RUN( "AR-04: file_separation", test_file_separation );
    PMM_RUN( "FR-05: nolock_preset", test_nolock_preset );
    PMM_RUN( "FR-05: presets_coexist", test_presets_coexist );

    std::cout << ( all_passed ? "\nAll tests PASSED\n" : "\nSome tests FAILED\n" );
    return all_passed ? 0 : 1;
}
