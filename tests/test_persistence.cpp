/**
 * @file test_persistence.cpp
 * @brief Тесты персистентности (save_manager/load_manager_from_file) (Issue #102 — новый API)
 *
 * Issue #102: использует AbstractPersistMemoryManager через pmm_presets.h.
 *   - save_manager() и load_manager_from_file() с AbstractPersistMemoryManager.
 *   - Все операции через экземпляр менеджера.
 */

#include "pmm/io.h"
#include "pmm/pmm_presets.h"

#include <cassert>
#include <cstdio>
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

static const char* TEST_FILE = "test_heap.dat";

static void cleanup_file()
{
    std::remove( TEST_FILE );
}

static bool test_persistence_basic_roundtrip()
{
    const std::size_t size = 64 * 1024;

    Mgr pmm1;
    PMM_TEST( pmm1.create( size ) );

    std::size_t total1        = pmm1.total_size();
    std::size_t used1         = pmm1.used_size();
    std::size_t free1         = pmm1.free_size();
    std::size_t blocks1       = pmm1.block_count();
    std::size_t free_blocks1  = pmm1.free_block_count();
    std::size_t alloc_blocks1 = pmm1.alloc_block_count();

    PMM_TEST( pmm::save_manager( pmm1, TEST_FILE ) );
    pmm1.destroy();

    Mgr pmm2;
    PMM_TEST( pmm2.create( size ) );
    PMM_TEST( pmm::load_manager_from_file( pmm2, TEST_FILE ) );
    PMM_TEST( pmm2.is_initialized() );

    PMM_TEST( pmm2.total_size() == total1 );
    PMM_TEST( pmm2.used_size() == used1 );
    PMM_TEST( pmm2.free_size() == free1 );
    PMM_TEST( pmm2.block_count() == blocks1 );
    PMM_TEST( pmm2.free_block_count() == free_blocks1 );
    PMM_TEST( pmm2.alloc_block_count() == alloc_blocks1 );

    pmm2.destroy();
    cleanup_file();
    return true;
}

static bool test_persistence_user_data_preserved()
{
    const std::size_t size = 64 * 1024;

    Mgr pmm1;
    PMM_TEST( pmm1.create( size ) );

    const std::size_t       data_size = 256;
    Mgr::pptr<std::uint8_t> ptr1      = pmm1.allocate_typed<std::uint8_t>( data_size );
    PMM_TEST( !ptr1.is_null() );

    std::memset( ptr1.resolve( pmm1 ), 0xCA, data_size );

    std::uint32_t saved_offset = ptr1.offset();

    PMM_TEST( pmm::save_manager( pmm1, TEST_FILE ) );
    pmm1.destroy();

    Mgr pmm2;
    PMM_TEST( pmm2.create( size ) );
    PMM_TEST( pmm::load_manager_from_file( pmm2, TEST_FILE ) );
    PMM_TEST( pmm2.is_initialized() );

    // Issue #75: 1 user block + BlockHeader_0
    PMM_TEST( pmm2.alloc_block_count() == 2 );

    // Recover pptr by saved offset
    Mgr::pptr<std::uint8_t> ptr2( saved_offset );
    const std::uint8_t*     p = ptr2.resolve( pmm2 );
    for ( std::size_t i = 0; i < data_size; i++ )
        PMM_TEST( p[i] == 0xCA );

    pmm2.deallocate_typed( ptr2 );
    pmm2.destroy();
    cleanup_file();
    return true;
}

static bool test_persistence_multiple_blocks()
{
    const std::size_t size = 128 * 1024;

    Mgr pmm1;
    PMM_TEST( pmm1.create( size ) );

    Mgr::pptr<std::uint8_t> p1 = pmm1.allocate_typed<std::uint8_t>( 128 );
    Mgr::pptr<std::uint8_t> p2 = pmm1.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p3 = pmm1.allocate_typed<std::uint8_t>( 512 );
    Mgr::pptr<std::uint8_t> p4 = pmm1.allocate_typed<std::uint8_t>( 64 );
    PMM_TEST( !p1.is_null() && !p2.is_null() && !p3.is_null() && !p4.is_null() );

    pmm1.deallocate_typed( p2 );
    pmm1.deallocate_typed( p4 );

    std::size_t blocks1       = pmm1.block_count();
    std::size_t free_blocks1  = pmm1.free_block_count();
    std::size_t alloc_blocks1 = pmm1.alloc_block_count();
    std::size_t total1        = pmm1.total_size();
    std::size_t used1         = pmm1.used_size();

    PMM_TEST( pmm::save_manager( pmm1, TEST_FILE ) );
    pmm1.destroy();

    Mgr pmm2;
    PMM_TEST( pmm2.create( size ) );
    PMM_TEST( pmm::load_manager_from_file( pmm2, TEST_FILE ) );
    PMM_TEST( pmm2.is_initialized() );

    PMM_TEST( pmm2.block_count() == blocks1 );
    PMM_TEST( pmm2.free_block_count() == free_blocks1 );
    PMM_TEST( pmm2.alloc_block_count() == alloc_blocks1 );
    PMM_TEST( pmm2.total_size() == total1 );
    PMM_TEST( pmm2.used_size() == used1 );

    pmm2.destroy();
    cleanup_file();
    return true;
}

static bool test_persistence_allocate_after_load()
{
    const std::size_t size = 64 * 1024;

    Mgr pmm1;
    PMM_TEST( pmm1.create( size ) );

    Mgr::pptr<std::uint8_t> p1 = pmm1.allocate_typed<std::uint8_t>( 512 );
    PMM_TEST( !p1.is_null() );

    PMM_TEST( pmm::save_manager( pmm1, TEST_FILE ) );
    pmm1.destroy();

    Mgr pmm2;
    PMM_TEST( pmm2.create( size ) );
    PMM_TEST( pmm::load_manager_from_file( pmm2, TEST_FILE ) );
    PMM_TEST( pmm2.is_initialized() );

    Mgr::pptr<std::uint8_t> p2 = pmm2.allocate_typed<std::uint8_t>( 256 );
    PMM_TEST( !p2.is_null() );

    // Issue #75: 1 pre-load user + 1 new user + BlockHeader_0
    PMM_TEST( pmm2.alloc_block_count() == 3 );

    pmm2.deallocate_typed( p2 );

    pmm2.destroy();
    cleanup_file();
    return true;
}

static bool test_persistence_save_null_filename()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 16 * 1024 ) );

    PMM_TEST( pmm::save_manager( pmm, nullptr ) == false );

    pmm.destroy();
    return true;
}

static bool test_persistence_load_nonexistent_file()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 16 * 1024 ) );

    PMM_TEST( !pmm::load_manager_from_file( pmm, "no_such_file_xyz123.dat" ) );

    pmm.destroy();
    return true;
}

static bool test_persistence_load_null_filename()
{
    Mgr pmm;
    PMM_TEST( pmm.create( 16 * 1024 ) );

    PMM_TEST( !pmm::load_manager_from_file( pmm, nullptr ) );

    pmm.destroy();
    return true;
}

static bool test_persistence_corrupted_image()
{
    const std::size_t size = 16 * 1024;
    {
        std::FILE* f = std::fopen( TEST_FILE, "wb" );
        PMM_TEST( f != nullptr );
        std::uint8_t zeros[16 * 1024] = {};
        std::fwrite( zeros, 1, size, f );
        std::fclose( f );
    }

    Mgr pmm;
    PMM_TEST( pmm.create( size ) );

    PMM_TEST( !pmm::load_manager_from_file( pmm, TEST_FILE ) );

    pmm.destroy();
    cleanup_file();
    return true;
}

static bool test_persistence_buffer_too_small()
{
    Mgr pmm1;
    PMM_TEST( pmm1.create( 32 * 1024 ) );
    PMM_TEST( pmm::save_manager( pmm1, TEST_FILE ) );
    pmm1.destroy();

    // Create smaller manager — load should fail since file is larger
    Mgr pmm2;
    PMM_TEST( pmm2.create( 4 * 1024 ) );
    PMM_TEST( !pmm::load_manager_from_file( pmm2, TEST_FILE ) );

    pmm2.destroy();
    cleanup_file();
    return true;
}

static bool test_persistence_double_save_load()
{
    const std::size_t size = 64 * 1024;

    Mgr pmm1;
    PMM_TEST( pmm1.create( size ) );

    Mgr::pptr<std::uint8_t> p1 = pmm1.allocate_typed<std::uint8_t>( 128 );
    Mgr::pptr<std::uint8_t> p2 = pmm1.allocate_typed<std::uint8_t>( 256 );
    PMM_TEST( !p1.is_null() && !p2.is_null() );
    std::memset( p1.resolve( pmm1 ), 0xAA, 128 );
    std::memset( p2.resolve( pmm1 ), 0xBB, 256 );

    std::size_t blocks1       = pmm1.block_count();
    std::size_t alloc_blocks1 = pmm1.alloc_block_count();
    std::size_t total1        = pmm1.total_size();

    PMM_TEST( pmm::save_manager( pmm1, TEST_FILE ) );
    pmm1.destroy();

    Mgr pmm2;
    PMM_TEST( pmm2.create( size ) );
    PMM_TEST( pmm::load_manager_from_file( pmm2, TEST_FILE ) );
    PMM_TEST( pmm2.is_initialized() );

    static const char* TEST_FILE2 = "test_heap2.dat";
    PMM_TEST( pmm::save_manager( pmm2, TEST_FILE2 ) );
    pmm2.destroy();

    Mgr pmm3;
    PMM_TEST( pmm3.create( size ) );
    PMM_TEST( pmm::load_manager_from_file( pmm3, TEST_FILE2 ) );
    PMM_TEST( pmm3.is_initialized() );

    PMM_TEST( pmm3.block_count() == blocks1 );
    PMM_TEST( pmm3.alloc_block_count() == alloc_blocks1 );
    PMM_TEST( pmm3.total_size() == total1 );

    pmm3.destroy();
    std::remove( TEST_FILE );
    std::remove( TEST_FILE2 );
    return true;
}

static bool test_persistence_empty_manager()
{
    const std::size_t size = 16 * 1024;

    Mgr pmm1;
    PMM_TEST( pmm1.create( size ) );

    std::size_t free_blocks1 = pmm1.free_block_count();
    PMM_TEST( pmm::save_manager( pmm1, TEST_FILE ) );
    pmm1.destroy();

    Mgr pmm2;
    PMM_TEST( pmm2.create( size ) );
    PMM_TEST( pmm::load_manager_from_file( pmm2, TEST_FILE ) );
    PMM_TEST( pmm2.is_initialized() );

    // Issue #75: only BlockHeader_0 remains as allocated
    PMM_TEST( pmm2.alloc_block_count() == 1 );
    PMM_TEST( pmm2.free_block_count() == free_blocks1 );

    Mgr::pptr<std::uint8_t> p = pmm2.allocate_typed<std::uint8_t>( 512 );
    PMM_TEST( !p.is_null() );

    pmm2.deallocate_typed( p );
    pmm2.destroy();
    cleanup_file();
    return true;
}

static bool test_persistence_deallocate_after_load()
{
    const std::size_t size = 64 * 1024;

    Mgr pmm1;
    PMM_TEST( pmm1.create( size ) );

    Mgr::pptr<std::uint8_t> p1 = pmm1.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> p2 = pmm1.allocate_typed<std::uint8_t>( 512 );
    PMM_TEST( !p1.is_null() && !p2.is_null() );

    std::uint32_t off1 = p1.offset();
    std::uint32_t off2 = p2.offset();

    PMM_TEST( pmm::save_manager( pmm1, TEST_FILE ) );
    pmm1.destroy();

    Mgr pmm2;
    PMM_TEST( pmm2.create( size ) );
    PMM_TEST( pmm::load_manager_from_file( pmm2, TEST_FILE ) );
    PMM_TEST( pmm2.is_initialized() );

    Mgr::pptr<std::uint8_t> q1( off1 );
    Mgr::pptr<std::uint8_t> q2( off2 );

    pmm2.deallocate_typed( q1 );
    pmm2.deallocate_typed( q2 );

    // Issue #75: only BlockHeader_0 remains
    PMM_TEST( pmm2.alloc_block_count() == 1 );

    pmm2.destroy();
    cleanup_file();
    return true;
}

int main()
{
    std::cout << "=== test_persistence ===\n";
    bool all_passed = true;

    PMM_RUN( "persistence_basic_roundtrip", test_persistence_basic_roundtrip );
    PMM_RUN( "persistence_user_data_preserved", test_persistence_user_data_preserved );
    PMM_RUN( "persistence_multiple_blocks", test_persistence_multiple_blocks );
    PMM_RUN( "persistence_allocate_after_load", test_persistence_allocate_after_load );
    PMM_RUN( "persistence_save_null_filename", test_persistence_save_null_filename );
    PMM_RUN( "persistence_load_nonexistent_file", test_persistence_load_nonexistent_file );
    PMM_RUN( "persistence_load_null_filename", test_persistence_load_null_filename );
    PMM_RUN( "persistence_corrupted_image", test_persistence_corrupted_image );
    PMM_RUN( "persistence_buffer_too_small", test_persistence_buffer_too_small );
    PMM_RUN( "persistence_double_save_load", test_persistence_double_save_load );
    PMM_RUN( "persistence_empty_manager", test_persistence_empty_manager );
    PMM_RUN( "persistence_deallocate_after_load", test_persistence_deallocate_after_load );

    std::cout << ( all_passed ? "\nAll tests PASSED\n" : "\nSome tests FAILED\n" );
    return all_passed ? 0 : 1;
}
