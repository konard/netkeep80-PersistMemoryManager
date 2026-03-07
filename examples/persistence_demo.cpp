/**
 * @file persistence_demo.cpp
 * @brief PersistMemoryManager persistence demo (updated #102)
 *
 * Demonstrates:
 * 1. Creating a manager and populating with data.
 * 2. Saving the memory image to a file.
 * 3. Destroying the first manager (simulates program termination).
 * 4. Loading the image into a new manager.
 * 5. Verifying that data and metadata are fully restored.
 * 6. Continuing operations on the restored manager.
 *
 * Issue #102: uses new AbstractPersistMemoryManager API via pmm_presets.h.
 * - No legacy_manager.h
 * - No singleton PersistMemoryManager<>
 * - No pmm::save() / pmm::load_from_file() — uses pmm::save_manager / pmm::load_manager_from_file
 * - Uses Manager::pptr<T> instead of pmm::pptr<T>
 * - Uses p.resolve(mgr) instead of p.get()
 * - Uses pmm.is_initialized() instead of pmm.validate()
 */

#include "pmm/io.h"
#include "pmm/pmm_presets.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

using Mgr = pmm::presets::SingleThreadedHeap;

static const char* IMAGE_FILE = "heap_image.dat";

int main()
{
    std::cout << "=== PersistMemoryManager — Persistence Demo (updated #102) ===\n\n";

    // ─── Phase A: Create and populate ────────────────────────────────────────

    const std::size_t memory_size = 256 * 1024; // 256 KB

    Mgr pmm1;
    if ( !pmm1.create( memory_size ) )
    {
        std::cerr << "Failed to create manager\n";
        return 1;
    }
    std::cout << "[A] Manager created. Size: " << memory_size / 1024 << " KB\n";

    const std::size_t size1 = 512;
    const std::size_t size2 = 1024;
    const std::size_t size3 = 256;

    Mgr::pptr<uint8_t> p1 = pmm1.allocate_typed<uint8_t>( size1 );
    Mgr::pptr<uint8_t> p2 = pmm1.allocate_typed<uint8_t>( size2 );
    Mgr::pptr<uint8_t> p3 = pmm1.allocate_typed<uint8_t>( size3 );

    if ( p1.is_null() || p2.is_null() || p3.is_null() )
    {
        std::cerr << "Error allocating blocks\n";
        pmm1.destroy();
        return 1;
    }

    // Write: p1 = string, p2 = array of squares, p3 = pattern
    std::strcpy( reinterpret_cast<char*>( p1.resolve( pmm1 ) ), "Hello, PersistMemoryManager!" );

    int* arr2 = reinterpret_cast<int*>( p2.resolve( pmm1 ) );
    for ( std::size_t i = 0; i < size2 / sizeof( int ); i++ )
        arr2[i] = static_cast<int>( i * i );

    std::memset( p3.resolve( pmm1 ), 0xFF, size3 );

    std::cout << "[A] 3 blocks allocated and written.\n";

    // Deallocate p3 to show partial free state is preserved
    pmm1.deallocate_typed( p3 );
    std::cout << "[A] Block p3 deallocated (shows partial free heap saved correctly).\n";

    if ( !pmm1.is_initialized() )
    {
        std::cerr << "Manager not initialized before save\n";
        pmm1.destroy();
        return 1;
    }

    std::cout << "\nStats before save:\n"
              << "  Total blocks : " << pmm1.block_count() << "\n"
              << "  Free blocks  : " << pmm1.free_block_count() << "\n"
              << "  Alloc blocks : " << pmm1.alloc_block_count() << "\n"
              << "  Free size    : " << pmm1.free_size() << " bytes\n";

    // Save granule offsets for reconstruction after load
    std::uint32_t off1 = p1.offset();
    std::uint32_t off2 = p2.offset();

    // ─── Phase B: Save image ──────────────────────────────────────────────────

    if ( !pmm::save_manager( pmm1, IMAGE_FILE ) )
    {
        std::cerr << "Error saving image to: " << IMAGE_FILE << "\n";
        pmm1.destroy();
        return 1;
    }
    std::cout << "\n[B] Image saved to: " << IMAGE_FILE << "\n";

    pmm1.destroy();
    std::cout << "[B] First manager destroyed (simulates program termination).\n";

    // ─── Phase C: Restore from file ───────────────────────────────────────────

    std::cout << "\n[C] Loading image from file...\n";

    Mgr pmm2;
    if ( !pmm2.create( memory_size ) )
    {
        std::cerr << "Failed to create second manager\n";
        return 1;
    }

    if ( !pmm::load_manager_from_file( pmm2, IMAGE_FILE ) )
    {
        std::cerr << "Failed to load image from file\n";
        pmm2.destroy();
        return 1;
    }

    if ( !pmm2.is_initialized() )
    {
        std::cerr << "Manager not initialized after load\n";
        pmm2.destroy();
        return 1;
    }

    std::cout << "[C] Image loaded successfully.\n";
    std::cout << "\nStats after load:\n"
              << "  Total blocks : " << pmm2.block_count() << "\n"
              << "  Free blocks  : " << pmm2.free_block_count() << "\n"
              << "  Alloc blocks : " << pmm2.alloc_block_count() << "\n";

    // ─── Phase D: Data verification ───────────────────────────────────────────

    // Reconstruct pptr from saved granule offsets
    Mgr::pptr<uint8_t> q1( off1 );
    Mgr::pptr<uint8_t> q2( off2 );

    std::cout << "\n[D] Data verification:\n";

    bool data_ok = true;
    if ( std::strcmp( reinterpret_cast<const char*>( q1.resolve( pmm2 ) ), "Hello, PersistMemoryManager!" ) == 0 )
    {
        std::cout << "  p1 (string)  : OK — \"" << reinterpret_cast<const char*>( q1.resolve( pmm2 ) ) << "\"\n";
    }
    else
    {
        std::cout << "  p1 (string)  : FAIL — data corrupted\n";
        data_ok = false;
    }

    bool arr_ok = true;
    int* q2_int = reinterpret_cast<int*>( q2.resolve( pmm2 ) );
    for ( std::size_t i = 0; i < size2 / sizeof( int ); i++ )
    {
        if ( q2_int[i] != static_cast<int>( i * i ) )
        {
            arr_ok = false;
            break;
        }
    }
    std::cout << "  p2 (array)   : " << ( arr_ok ? "OK" : "FAIL" ) << "\n";
    data_ok &= arr_ok;

    // ─── Phase E: Continued operations ───────────────────────────────────────

    std::cout << "\n[E] Continued operation on restored manager:\n";

    Mgr::pptr<uint8_t> p_new = pmm2.allocate_typed<uint8_t>( 128 );
    if ( !p_new.is_null() )
    {
        std::memset( p_new.resolve( pmm2 ), 0xAB, 128 );
        std::cout << "  New block allocated: offset=" << p_new.offset() << "\n";
        pmm2.deallocate_typed( p_new );
        std::cout << "  New block deallocated.\n";
    }
    else
    {
        std::cout << "  Failed to allocate new block.\n";
        data_ok = false;
    }

    if ( pmm2.is_initialized() )
        std::cout << "  Final state validation: OK\n";
    else
    {
        std::cout << "  Final state validation: FAIL\n";
        data_ok = false;
    }

    // ─── Cleanup ──────────────────────────────────────────────────────────────

    pmm2.destroy();
    std::remove( IMAGE_FILE );

    std::cout << "\n=== Demo completed: " << ( data_ok ? "SUCCESS" : "ERROR" ) << " ===\n";
    return data_ok ? 0 : 1;
}
