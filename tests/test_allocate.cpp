/**
 * @file test_allocate.cpp
 * @brief Тесты выделения памяти (Issue #102 — новый API)
 *
 * Issue #102: использует AbstractPersistMemoryManager через pmm_presets.h.
 *   - pmm::presets::SingleThreadedHeap — однопоточный менеджер на базе HeapStorage.
 *   - Все операции через экземпляр менеджера.
 *   - Выделение через allocate_typed<T>(), освобождение через deallocate_typed().
 *   - Автоматическое расширение памяти при нехватке.
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

static bool test_create_basic()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );
    PMM_TEST( pmm.is_initialized() );

    pmm.destroy();
    PMM_TEST( !pmm.is_initialized() );
    return true;
}

static bool test_create_too_small()
{
    Mgr pmm;
    PMM_TEST( !pmm.create( 128 ) );
    return true;
}

static bool test_allocate_single_small()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );

    Mgr::pptr<std::uint8_t> p = pmm.allocate_typed<std::uint8_t>( 64 );
    PMM_TEST( !p.is_null() );
    PMM_TEST( p.resolve( pmm ) != nullptr );
    PMM_TEST( reinterpret_cast<std::uintptr_t>( p.resolve( pmm ) ) % 16 == 0 );

    pmm.deallocate_typed( p );
    pmm.destroy();
    return true;
}

static bool test_allocate_multiple()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 256 * 1024 ) );

    const int               num = 10;
    Mgr::pptr<std::uint8_t> ptrs[num];
    for ( int i = 0; i < num; i++ )
    {
        ptrs[i] = pmm.allocate_typed<std::uint8_t>( 1024 );
        PMM_TEST( !ptrs[i].is_null() );
    }

    for ( int i = 0; i < num; i++ )
    {
        for ( int j = i + 1; j < num; j++ )
        {
            PMM_TEST( ptrs[i] != ptrs[j] );
        }
    }

    for ( int i = 0; i < num; i++ )
        pmm.deallocate_typed( ptrs[i] );

    pmm.destroy();
    return true;
}

static bool test_allocate_zero()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );

    Mgr::pptr<std::uint8_t> p = pmm.allocate_typed<std::uint8_t>( 0 );
    PMM_TEST( p.is_null() );

    pmm.destroy();
    return true;
}

/**
 * @brief Автоматическое расширение памяти при нехватке.
 */
static bool test_allocate_auto_expand()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 8 * 1024 ) );

    std::size_t initial_total = pmm.total_size();

    // Заполняем большую часть памяти
    Mgr::pptr<std::uint8_t> block1 = pmm.allocate_typed<std::uint8_t>( 4 * 1024 );
    PMM_TEST( !block1.is_null() );

    // Запрашиваем блок, который требует расширения
    Mgr::pptr<std::uint8_t> block2 = pmm.allocate_typed<std::uint8_t>( 4 * 1024 );
    PMM_TEST( !block2.is_null() );

    PMM_TEST( pmm.is_initialized() );
    PMM_TEST( pmm.total_size() > initial_total );

    pmm.destroy();
    return true;
}

static bool test_allocate_write_read()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );

    Mgr::pptr<std::uint8_t> p1 = pmm.allocate_typed<std::uint8_t>( 128 );
    Mgr::pptr<std::uint8_t> p2 = pmm.allocate_typed<std::uint8_t>( 256 );
    PMM_TEST( !p1.is_null() );
    PMM_TEST( !p2.is_null() );

    std::memset( p1.resolve( pmm ), 0xAA, 128 );
    std::memset( p2.resolve( pmm ), 0xBB, 256 );

    const std::uint8_t* r1 = p1.resolve( pmm );
    const std::uint8_t* r2 = p2.resolve( pmm );
    for ( std::size_t i = 0; i < 128; i++ )
        PMM_TEST( r1[i] == 0xAA );
    for ( std::size_t i = 0; i < 256; i++ )
        PMM_TEST( r2[i] == 0xBB );

    pmm.deallocate_typed( p1 );
    pmm.deallocate_typed( p2 );
    pmm.destroy();
    return true;
}

static bool test_allocate_metrics()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 64 * 1024 ) );

    PMM_TEST( pmm.total_size() == 64 * 1024 );
    PMM_TEST( pmm.used_size() > 0 );
    PMM_TEST( pmm.free_size() < 64 * 1024 );
    PMM_TEST( pmm.used_size() + pmm.free_size() <= 64 * 1024 );

    std::size_t used_before = pmm.used_size();

    Mgr::pptr<std::uint8_t> ptr = pmm.allocate_typed<std::uint8_t>( 512 );
    PMM_TEST( !ptr.is_null() );
    PMM_TEST( pmm.used_size() > used_before );

    pmm.deallocate_typed( ptr );
    pmm.destroy();
    return true;
}

/**
 * @brief Fragmented free blocks must be reused before the tail expansion space
 *        (Issue #53 fix verification).
 *
 * Strategy:
 *   1. Create a PMM and allocate N blocks, staying within the initial buffer.
 *   2. Free every other block to create N/2 non-adjacent fragmented holes.
 *   3. Re-allocate N/2 blocks of the same size.
 *   4. Verify total_size did NOT grow — all allocations fit in the freed holes.
 */
static bool test_fragmented_gaps_reused_before_expand_space()
{
    const std::size_t block_size   = 256;
    const std::size_t initial_size = 8 * 1024;

    Mgr pmm;
    PMM_TEST( pmm.create( initial_size ) );

    // Allocate blocks until most of the space is used, but stop before auto-grow.
    Mgr::pptr<std::uint8_t> ptrs[20];
    int                     n = 0;
    for ( ; n < 20; ++n )
    {
        ptrs[n] = pmm.allocate_typed<std::uint8_t>( block_size );
        if ( ptrs[n].is_null() )
            break;
        // Stop before we accidentally trigger auto-grow
        if ( pmm.total_size() != initial_size )
            break;
    }
    PMM_TEST( n >= 4 );

    // Free every other block — creates n/2 non-adjacent holes
    int holes = 0;
    for ( int i = 0; i < n; i += 2 )
    {
        pmm.deallocate_typed( ptrs[i] );
        ++holes;
    }
    PMM_TEST( holes >= 2 );

    // Record state before re-allocation
    std::size_t size_before = pmm.total_size();

    // Re-allocate the freed blocks; they must fit in the fragmented holes.
    for ( int i = 0; i < holes; ++i )
    {
        Mgr::pptr<std::uint8_t> p = pmm.allocate_typed<std::uint8_t>( block_size );
        PMM_TEST( !p.is_null() );
        ptrs[i] = p; // track for cleanup
    }

    // No expansion must have occurred — all allocations fit inside the freed holes.
    PMM_TEST( pmm.total_size() == size_before );

    pmm.destroy();
    return true;
}

int main()
{
    std::cout << "=== test_allocate ===\n";
    bool all_passed = true;

    PMM_RUN( "create_basic", test_create_basic );
    PMM_RUN( "create_too_small", test_create_too_small );
    PMM_RUN( "allocate_single_small", test_allocate_single_small );
    PMM_RUN( "allocate_multiple", test_allocate_multiple );
    PMM_RUN( "allocate_zero", test_allocate_zero );
    PMM_RUN( "allocate_auto_expand", test_allocate_auto_expand );
    PMM_RUN( "allocate_write_read", test_allocate_write_read );
    PMM_RUN( "allocate_metrics", test_allocate_metrics );
    PMM_RUN( "fragmented_gaps_reused_before_expand_space", test_fragmented_gaps_reused_before_expand_space );

    std::cout << ( all_passed ? "\nAll tests PASSED\n" : "\nSome tests FAILED\n" );
    return all_passed ? 0 : 1;
}
