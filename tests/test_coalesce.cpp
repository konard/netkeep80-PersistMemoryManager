/**
 * @file test_coalesce.cpp
 * @brief Тесты слияния соседних свободных блоков (Issue #102 — новый API)
 *
 * Issue #102: использует AbstractPersistMemoryManager через pmm_presets.h.
 *   - Все операции через экземпляр менеджера.
 *   - Статистика через block_count(), free_block_count(), alloc_block_count().
 */

#include "pmm/pmm_presets.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

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

static bool test_coalesce_with_next()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );

    Mgr::pptr<std::uint8_t> p1 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p2 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p3 = pmm.allocate_typed<std::uint8_t>( 256 );
    PMM_TEST( !p1.is_null() && !p2.is_null() && !p3.is_null() );

    std::size_t blocks_before = pmm.block_count();

    pmm.deallocate_typed( p2 );
    PMM_TEST( pmm.block_count() == blocks_before );

    std::size_t blocks_mid = pmm.block_count();
    pmm.deallocate_typed( p1 );
    PMM_TEST( pmm.block_count() < blocks_mid );

    Mgr::pptr<std::uint8_t> big = pmm.allocate_typed<std::uint8_t>( 400 );
    PMM_TEST( !big.is_null() );

    pmm.deallocate_typed( big );
    pmm.deallocate_typed( p3 );

    pmm.destroy();
    return true;
}

static bool test_coalesce_with_prev()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );

    Mgr::pptr<std::uint8_t> p1 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p2 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p3 = pmm.allocate_typed<std::uint8_t>( 256 );
    PMM_TEST( !p1.is_null() && !p2.is_null() && !p3.is_null() );

    std::size_t blocks_before = pmm.block_count();

    pmm.deallocate_typed( p2 );
    PMM_TEST( pmm.block_count() == blocks_before );

    std::size_t blocks_mid = pmm.block_count();
    pmm.deallocate_typed( p3 );
    PMM_TEST( pmm.block_count() < blocks_mid );

    Mgr::pptr<std::uint8_t> big = pmm.allocate_typed<std::uint8_t>( 400 );
    PMM_TEST( !big.is_null() );

    pmm.deallocate_typed( big );
    pmm.deallocate_typed( p1 );

    pmm.destroy();
    return true;
}

static bool test_coalesce_both_neighbors()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 128 * 1024 ) );

    Mgr::pptr<std::uint8_t> p1 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p2 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p3 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p4 = pmm.allocate_typed<std::uint8_t>( 256 );
    PMM_TEST( !p1.is_null() && !p2.is_null() && !p3.is_null() && !p4.is_null() );

    pmm.deallocate_typed( p1 );
    pmm.deallocate_typed( p3 );

    std::size_t blocks_before = pmm.block_count();
    std::size_t free_before   = pmm.free_block_count();

    pmm.deallocate_typed( p2 );
    PMM_TEST( pmm.block_count() == blocks_before - 2 );
    PMM_TEST( pmm.free_block_count() == free_before - 1 );

    Mgr::pptr<std::uint8_t> big = pmm.allocate_typed<std::uint8_t>( 600 );
    PMM_TEST( !big.is_null() );

    pmm.deallocate_typed( big );
    pmm.deallocate_typed( p4 );

    pmm.destroy();
    return true;
}

static bool test_coalesce_no_merge_when_neighbors_used()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );

    Mgr::pptr<std::uint8_t> p1 = pmm.allocate_typed<std::uint8_t>( 128 );
    Mgr::pptr<std::uint8_t> p2 = pmm.allocate_typed<std::uint8_t>( 128 );
    Mgr::pptr<std::uint8_t> p3 = pmm.allocate_typed<std::uint8_t>( 128 );
    PMM_TEST( !p1.is_null() && !p2.is_null() && !p3.is_null() );

    std::size_t blocks_before = pmm.block_count();
    std::size_t free_before   = pmm.free_block_count();

    pmm.deallocate_typed( p2 );
    PMM_TEST( pmm.block_count() == blocks_before );
    PMM_TEST( pmm.free_block_count() == free_before + 1 );

    pmm.deallocate_typed( p1 );
    pmm.deallocate_typed( p3 );

    pmm.destroy();
    return true;
}

static bool test_coalesce_first_block_no_next_free()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );

    Mgr::pptr<std::uint8_t> p1 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p2 = pmm.allocate_typed<std::uint8_t>( 256 );
    PMM_TEST( !p1.is_null() && !p2.is_null() );

    std::size_t blocks_before = pmm.block_count();
    std::size_t free_before   = pmm.free_block_count();

    pmm.deallocate_typed( p1 );
    PMM_TEST( pmm.block_count() == blocks_before );
    PMM_TEST( pmm.free_block_count() == free_before + 1 );

    pmm.deallocate_typed( p2 );

    pmm.destroy();
    return true;
}

static bool test_coalesce_zero_fragmentation_after_all_free()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 256 * 1024 ) );

    const int               N = 8;
    Mgr::pptr<std::uint8_t> ptrs[N];
    for ( int i = 0; i < N; i++ )
    {
        ptrs[i] = pmm.allocate_typed<std::uint8_t>( 256 );
        PMM_TEST( !ptrs[i].is_null() );
    }

    for ( int i = 0; i < N; i += 2 )
        pmm.deallocate_typed( ptrs[i] );

    PMM_TEST( pmm.free_block_count() > 1 );

    for ( int i = 1; i < N; i += 2 )
        pmm.deallocate_typed( ptrs[i] );

    PMM_TEST( pmm.free_block_count() == 1 );
    PMM_TEST( pmm.block_count() == 2 );
    PMM_TEST( pmm.alloc_block_count() == 1 );

    pmm.destroy();
    return true;
}

static bool test_coalesce_lifo_results_in_one_block()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 128 * 1024 ) );

    const int               N = 5;
    Mgr::pptr<std::uint8_t> ptrs[N];
    for ( int i = 0; i < N; i++ )
    {
        ptrs[i] = pmm.allocate_typed<std::uint8_t>( 512 );
        PMM_TEST( !ptrs[i].is_null() );
    }

    for ( int i = N - 1; i >= 0; i-- )
        pmm.deallocate_typed( ptrs[i] );

    PMM_TEST( pmm.block_count() == 2 );
    PMM_TEST( pmm.free_block_count() == 1 );

    pmm.destroy();
    return true;
}

static bool test_coalesce_fifo_results_in_one_block()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 128 * 1024 ) );

    const int               N = 5;
    Mgr::pptr<std::uint8_t> ptrs[N];
    for ( int i = 0; i < N; i++ )
    {
        ptrs[i] = pmm.allocate_typed<std::uint8_t>( 512 );
        PMM_TEST( !ptrs[i].is_null() );
    }

    for ( int i = 0; i < N; i++ )
        pmm.deallocate_typed( ptrs[i] );

    PMM_TEST( pmm.block_count() == 2 );
    PMM_TEST( pmm.free_block_count() == 1 );

    pmm.destroy();
    return true;
}

static bool test_coalesce_large_allocation_after_merge()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );

    Mgr::pptr<std::uint8_t> p1 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p2 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p3 = pmm.allocate_typed<std::uint8_t>( 256 );
    PMM_TEST( !p1.is_null() && !p2.is_null() && !p3.is_null() );

    Mgr::pptr<std::uint8_t> probe = pmm.allocate_typed<std::uint8_t>( 700 );
    if ( !probe.is_null() )
        pmm.deallocate_typed( probe );

    pmm.deallocate_typed( p1 );
    pmm.deallocate_typed( p2 );
    pmm.deallocate_typed( p3 );

    Mgr::pptr<std::uint8_t> big = pmm.allocate_typed<std::uint8_t>( 512 );
    PMM_TEST( !big.is_null() );

    std::memset( big.resolve( pmm ), 0xAB, 512 );
    const std::uint8_t* p = big.resolve( pmm );
    for ( std::size_t i = 0; i < 512; i++ )
        PMM_TEST( p[i] == 0xAB );

    pmm.deallocate_typed( big );

    pmm.destroy();
    return true;
}

static bool test_coalesce_stress_interleaved()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 512 * 1024 ) );

    static const int        ROUNDS = 200;
    Mgr::pptr<std::uint8_t> ptrs[4];
    std::size_t             sizes[] = { 64, 128, 256, 512 };

    for ( int r = 0; r < ROUNDS; r++ )
    {
        int slot = r % 4;
        if ( !ptrs[slot].is_null() )
        {
            pmm.deallocate_typed( ptrs[slot] );
            ptrs[slot] = Mgr::pptr<std::uint8_t>();
        }
        ptrs[slot] = pmm.allocate_typed<std::uint8_t>( sizes[slot] );
        PMM_TEST( !ptrs[slot].is_null() );
    }

    for ( int k = 0; k < 4; k++ )
    {
        if ( !ptrs[k].is_null() )
            pmm.deallocate_typed( ptrs[k] );
    }

    PMM_TEST( pmm.free_block_count() == 1 );

    pmm.destroy();
    return true;
}

int main()
{
    std::cout << "=== test_coalesce ===\n";
    bool all_passed = true;

    PMM_RUN( "coalesce_with_next", test_coalesce_with_next );
    PMM_RUN( "coalesce_with_prev", test_coalesce_with_prev );
    PMM_RUN( "coalesce_both_neighbors", test_coalesce_both_neighbors );
    PMM_RUN( "coalesce_no_merge_when_neighbors_used", test_coalesce_no_merge_when_neighbors_used );
    PMM_RUN( "coalesce_first_block_no_next_free", test_coalesce_first_block_no_next_free );
    PMM_RUN( "coalesce_zero_fragmentation_after_all_free", test_coalesce_zero_fragmentation_after_all_free );
    PMM_RUN( "coalesce_lifo_results_in_one_block", test_coalesce_lifo_results_in_one_block );
    PMM_RUN( "coalesce_fifo_results_in_one_block", test_coalesce_fifo_results_in_one_block );
    PMM_RUN( "coalesce_large_allocation_after_merge", test_coalesce_large_allocation_after_merge );
    PMM_RUN( "coalesce_stress_interleaved", test_coalesce_stress_interleaved );

    std::cout << ( all_passed ? "\nAll tests PASSED\n" : "\nSome tests FAILED\n" );
    return all_passed ? 0 : 1;
}
