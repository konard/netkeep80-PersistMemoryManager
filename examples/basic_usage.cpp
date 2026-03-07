/**
 * @file basic_usage.cpp
 * @brief Basic usage example for PersistMemoryManager (updated #102)
 *
 * Demonstrates:
 * 1. Creating a manager with HeapStorage.
 * 2. Allocating blocks of different sizes.
 * 3. Deallocating blocks.
 * 4. Manual realloc pattern (alloc-copy-dealloc).
 * 5. Block statistics via new API.
 * 6. Save/load round-trip.
 *
 * Issue #102: uses new AbstractPersistMemoryManager API via pmm_presets.h.
 * - No legacy_manager.h
 * - No singleton PersistMemoryManager<>
 * - No reallocate_typed() — use manual alloc-copy-dealloc
 * - No pmm::pptr<T> single-arg — use Manager::pptr<T>
 * - No p.get() — use p.resolve(mgr)
 * - No get_stats() — use pmm.block_count(), free_block_count(), alloc_block_count()
 */

#include "pmm/io.h"
#include "pmm/pmm_presets.h"

#include <cstdlib>
#include <cstring>
#include <iostream>

using Mgr = pmm::presets::SingleThreadedHeap;

int main()
{
    // ── 1. Create manager ─────────────────────────────────────────────────────
    const std::size_t memory_size = 1024 * 1024; // 1 MB

    Mgr pmm;
    if ( !pmm.create( memory_size ) )
    {
        std::cerr << "Failed to create PersistMemoryManager\n";
        return 1;
    }
    std::cout << "Manager created. Managed area: " << memory_size / 1024 << " KB\n\n";

    // ── 2. Allocate blocks ────────────────────────────────────────────────────
    Mgr::pptr<std::uint8_t> block1 = pmm.allocate_typed<std::uint8_t>( 256 );
    Mgr::pptr<std::uint8_t> block2 = pmm.allocate_typed<std::uint8_t>( 1024 );
    Mgr::pptr<std::uint8_t> block3 = pmm.allocate_typed<std::uint8_t>( 4096 );

    if ( block1.is_null() || block2.is_null() || block3.is_null() )
    {
        std::cerr << "Error allocating blocks\n";
        pmm.destroy();
        return 1;
    }

    std::cout << "Allocated 3 blocks:\n";
    std::cout << "  block1 (256 bytes):  offset=" << block1.offset() << "\n";
    std::cout << "  block2 (1024 bytes): offset=" << block2.offset() << "\n";
    std::cout << "  block3 (4096 bytes): offset=" << block3.offset() << "\n\n";

    // Check alignment (granule = 16 bytes guarantees align=16)
    auto check_align = []( void* ptr, std::size_t align, const char* name )
    {
        bool ok = ( reinterpret_cast<std::uintptr_t>( ptr ) % align == 0 );
        std::cout << "  Alignment " << name << " at " << align << " bytes: " << ( ok ? "OK" : "FAIL" ) << "\n";
        return ok;
    };
    bool aligns_ok = true;
    aligns_ok &= check_align( block1.resolve( pmm ), 16, "block1" );
    aligns_ok &= check_align( block2.resolve( pmm ), 16, "block2" );
    aligns_ok &= check_align( block3.resolve( pmm ), 16, "block3" );
    std::cout << "\n";

    // ── 3. Write data ─────────────────────────────────────────────────────────
    std::memset( block1.resolve( pmm ), 0xAA, 256 );
    std::memset( block2.resolve( pmm ), 0xBB, 1024 );
    std::memset( block3.resolve( pmm ), 0xCC, 4096 );
    std::cout << "Data written to blocks.\n\n";

    // ── 4. Block statistics ───────────────────────────────────────────────────
    std::cout << "Block statistics after allocations:\n"
              << "  Total blocks : " << pmm.block_count() << "\n"
              << "  Free blocks  : " << pmm.free_block_count() << "\n"
              << "  Alloc blocks : " << pmm.alloc_block_count() << "\n"
              << "  Free size    : " << pmm.free_size() << " bytes\n"
              << "  Used size    : " << pmm.used_size() << " bytes\n\n";

    // ── 5. Deallocate block1 ──────────────────────────────────────────────────
    pmm.deallocate_typed( block1 );
    block1 = Mgr::pptr<std::uint8_t>(); // null
    std::cout << "block1 deallocated.\n";
    std::cout << "Alloc blocks after dealloc: " << pmm.alloc_block_count() << "\n\n";

    // ── 6. Manual realloc of block2 (alloc-copy-dealloc) ─────────────────────
    // Issue #102: reallocate_typed() removed — use manual alloc-copy-dealloc
    const std::size_t old_size = 1024;
    const std::size_t new_size = 2048;

    Mgr::pptr<std::uint8_t> block2_new = pmm.allocate_typed<std::uint8_t>( new_size );
    if ( block2_new.is_null() )
    {
        std::cerr << "Error reallocating block2\n";
    }
    else
    {
        std::memcpy( block2_new.resolve( pmm ), block2.resolve( pmm ), old_size );
        pmm.deallocate_typed( block2 );
        block2 = block2_new;
        std::cout << "block2 reallocated (1024 -> 2048 bytes): offset=" << block2.offset() << "\n\n";
    }

    // ── 7. Verify manager is still valid ─────────────────────────────────────
    bool valid = pmm.is_initialized();
    std::cout << "Manager initialized: " << ( valid ? "OK" : "FAIL" ) << "\n\n";

    // ── 8. Deallocate remaining blocks ────────────────────────────────────────
    pmm.deallocate_typed( block2 );
    pmm.deallocate_typed( block3 );
    std::cout << "All blocks deallocated.\n";
    std::cout << "Final alloc_block_count: " << pmm.alloc_block_count() << "\n";

    // ── 9. Save/load round-trip demo ──────────────────────────────────────────
    const char* DEMO_FILE = "basic_usage_demo.dat";
    if ( pmm::save_manager( pmm, DEMO_FILE ) )
    {
        std::cout << "\nSaved manager state to: " << DEMO_FILE << "\n";

        Mgr pmm2;
        if ( pmm2.create( memory_size ) && pmm::load_manager_from_file( pmm2, DEMO_FILE ) )
        {
            std::cout << "Loaded manager state: " << ( pmm2.is_initialized() ? "OK" : "FAIL" ) << "\n";
            pmm2.destroy();
        }
        std::remove( DEMO_FILE );
    }

    // ── 10. Destroy manager ───────────────────────────────────────────────────
    pmm.destroy();

    std::cout << "\nExample completed " << ( aligns_ok && valid ? "successfully" : "with failures" ) << ".\n";
    return ( aligns_ok && valid ) ? 0 : 1;
}
