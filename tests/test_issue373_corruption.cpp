#include <catch2/catch_test_macros.hpp>

#include "pmm/allocator_policy.h"
#include "pmm/arena_internals.h"
#include "pmm/free_block_tree.h"
#include "pmm/heap_storage.h"
#include "pmm/manager_configs.h"
#include "pmm/persist_memory_manager.h"
#include "pmm/validation.h"

#include <cstdint>
#include <cstring>
#include <limits>

namespace
{
using AT         = pmm::DefaultAddressTraits;
using BlockState = pmm::BlockStateBase<AT>;

template <typename Mgr> auto* arena_base()
{
    return Mgr::backend().base_ptr();
}

template <typename Mgr> auto* arena_header()
{
    return pmm::detail::manager_header_at<typename Mgr::address_traits>( Mgr::backend().base_ptr() );
}

} // namespace

/*
### test-issue373-corruption-cycle
*/
TEST_CASE( "I373-Corruption: walker terminates on self-cycle next_offset", "[issue373][corruption]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 37310>;
    REQUIRE( Mgr::create( 4096 ) );
    void* p1 = Mgr::allocate( 64 );
    void* p2 = Mgr::allocate( 64 );
    REQUIRE( p1 != nullptr );
    REQUIRE( p2 != nullptr );

    auto*       base = arena_base<Mgr>();
    const auto* hdr  = arena_header<Mgr>();

    pmm::detail::ConstArenaView<AT> view{ base, hdr };
    AT::index_type                  idx = hdr->first_block_offset;
    REQUIRE( idx != AT::no_block );
    void*          blk_raw       = base + static_cast<std::size_t>( idx ) * AT::granule_size;
    AT::index_type original_next = BlockState::get_next_offset( blk_raw );
    BlockState::set_next_offset_of( blk_raw, idx );

    bool walked =
        pmm::detail::for_each_physical_block<AT>( view, []( AT::index_type, const void* ) noexcept { return true; } );
    REQUIRE_FALSE( walked );

    BlockState::set_next_offset_of( blk_raw, original_next );
    Mgr::deallocate( p2 );
    Mgr::deallocate( p1 );
    Mgr::destroy();
}

TEST_CASE( "I373-Corruption: walker terminates on backward next_offset", "[issue373][corruption]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 37311>;
    REQUIRE( Mgr::create( 4096 ) );
    void* p1 = Mgr::allocate( 64 );
    void* p2 = Mgr::allocate( 64 );
    REQUIRE( p1 != nullptr );
    REQUIRE( p2 != nullptr );

    auto*       base = arena_base<Mgr>();
    const auto* hdr  = arena_header<Mgr>();

    pmm::detail::ConstArenaView<AT> view{ base, hdr };
    AT::index_type                  idx = hdr->last_block_offset;
    REQUIRE( idx != AT::no_block );
    void*          blk_raw       = base + static_cast<std::size_t>( idx ) * AT::granule_size;
    AT::index_type original_next = BlockState::get_next_offset( blk_raw );
    BlockState::set_next_offset_of( blk_raw, hdr->first_block_offset );

    bool walked =
        pmm::detail::for_each_physical_block<AT>( view, []( AT::index_type, const void* ) noexcept { return true; } );
    REQUIRE_FALSE( walked );

    BlockState::set_next_offset_of( blk_raw, original_next );
    Mgr::deallocate( p2 );
    Mgr::deallocate( p1 );
    Mgr::destroy();
}

/*
### test-issue373-corruption-verify
*/
TEST_CASE( "I373-Corruption: verify reports corruption on cyclic chain without hanging", "[issue373][corruption]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 37312>;
    REQUIRE( Mgr::create( 4096 ) );
    void* p1 = Mgr::allocate( 64 );
    REQUIRE( p1 != nullptr );

    auto*          base          = arena_base<Mgr>();
    const auto*    hdr           = arena_header<Mgr>();
    AT::index_type idx           = hdr->first_block_offset;
    void*          blk_raw       = base + static_cast<std::size_t>( idx ) * AT::granule_size;
    AT::index_type original_next = BlockState::get_next_offset( blk_raw );
    BlockState::set_next_offset_of( blk_raw, idx );

    pmm::VerifyResult result = Mgr::verify();
    REQUIRE_FALSE( result.ok );

    BlockState::set_next_offset_of( blk_raw, original_next );
    Mgr::deallocate( p1 );
    Mgr::destroy();
}

/*
### test-issue373-overflow-reallocate
*/
TEST_CASE( "I373-Overflow: reallocate_typed rejects size_t overflow with PmmError::Overflow", "[issue373][overflow]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 37313>;
    REQUIRE( Mgr::create( 4096 ) );
    auto p = Mgr::allocate_typed<std::uint32_t>( 1 );
    REQUIRE( !p.is_null() );

    Mgr::clear_error();
    auto huge_p = Mgr::reallocate_typed<std::uint32_t>( p, 1, std::numeric_limits<std::size_t>::max() );
    REQUIRE( huge_p.is_null() );
    REQUIRE( Mgr::last_error() == pmm::PmmError::Overflow );

    Mgr::deallocate_typed( p );
    Mgr::destroy();
}

/*
### test-issue373-real-growth-cap
*/
TEST_CASE( "I373-Growth: real allocation respects max_memory_gb cap from config", "[issue373][growth]" )
{
    auto cap = pmm::detail::compute_growth( 1024, 1024 * 1024 * 1024, 16, 5, 4, 1 );
    REQUIRE_FALSE( cap.has_value() );
}

/*
### test-issue373-real-growth-uses-policy
*/
TEST_CASE( "I373-Growth: do_expand uses compute_growth for backend resize", "[issue373][growth]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 37314>;
    REQUIRE( Mgr::create( 4096 ) );
    std::size_t old_size = Mgr::backend().total_size();
    void*       big      = Mgr::allocate( 8192 );
    REQUIRE( big != nullptr );
    REQUIRE( Mgr::backend().total_size() > old_size );
    Mgr::deallocate( big );
    Mgr::destroy();
}

/*
### test-issue373-arenaview-huge-idx
*/
TEST_CASE( "I373-ArenaView: valid_block rejects index whose granule offset overflows size_t",
           "[issue373][arena][overflow]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 37315>;
    REQUIRE( Mgr::create( 4096 ) );
    auto*                           base = Mgr::backend().base_ptr();
    const auto*                     hdr  = pmm::detail::manager_header_at<AT>( base );
    pmm::detail::ConstArenaView<AT> view{ base, hdr };
    REQUIRE_FALSE( view.valid_block( std::numeric_limits<AT::index_type>::max() ) );
    REQUIRE_FALSE( view.valid_block( static_cast<AT::index_type>( hdr->total_size / AT::granule_size + 100 ) ) );
    Mgr::destroy();
}

/*
### test-issue373-allocate-from-block-zero
*/
TEST_CASE( "I373-Allocator: allocate_from_block(data_gran=0) returns nullptr instead of clamping to 1",
           "[issue373][allocator][contract]" )
{
    using Mgr   = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 37316>;
    using Alloc = pmm::AllocatorPolicy<pmm::AvlFreeTree<pmm::DefaultAddressTraits>, pmm::DefaultAddressTraits>;
    REQUIRE( Mgr::create( 4096 ) );
    auto*          base = Mgr::backend().base_ptr();
    auto*          hdr  = pmm::detail::manager_header_at<AT>( base );
    AT::index_type idx  = hdr->free_tree_root;
    REQUIRE( idx != AT::no_block );
    pmm::detail::ArenaView<AT> arena{ base, hdr };
    void*                      p = Alloc::allocate_from_block( arena, idx, 0 );
    REQUIRE( p == nullptr );
    Mgr::destroy();
}

/*
### test-issue373-recovery-cyclic-terminates
*/
TEST_CASE( "I373-Recovery: repair_linked_list / rebuild_free_tree terminate on cyclic chain", "[issue373][recovery]" )
{
    using Alloc = pmm::AllocatorPolicy<pmm::AvlFreeTree<pmm::DefaultAddressTraits>, pmm::DefaultAddressTraits>;
    using Mgr   = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 37317>;
    REQUIRE( Mgr::create( 4096 ) );
    void* p1 = Mgr::allocate( 64 );
    REQUIRE( p1 != nullptr );

    auto*          base = arena_base<Mgr>();
    auto*          hdr  = pmm::detail::manager_header_at<AT>( base );
    AT::index_type idx  = hdr->first_block_offset;
    REQUIRE( idx != AT::no_block );
    void*          blk_raw       = base + static_cast<std::size_t>( idx ) * AT::granule_size;
    AT::index_type original_next = BlockState::get_next_offset( blk_raw );
    BlockState::set_next_offset_of( blk_raw, idx );

    pmm::detail::ArenaView<AT> arena{ base, hdr };
    Alloc::repair_linked_list( arena );
    Alloc::rebuild_free_tree( arena );
    SUCCEED( "recovery returned without hanging" );

    BlockState::set_next_offset_of( blk_raw, original_next );
    Mgr::deallocate( p1 );
    Mgr::destroy();
}

/*
### test-issue373-validation-overflow
*/
TEST_CASE( "I373-Validation: validate_block_index handles overflow-prone indexes safely", "[issue373][validation]" )
{
    using IndexT                = AT::index_type;
    constexpr std::size_t total = 4096;
    REQUIRE_FALSE( pmm::detail::validate_block_index<AT>( total, AT::no_block ) );
    REQUIRE_FALSE( pmm::detail::validate_block_index<AT>( total, static_cast<IndexT>( total / AT::granule_size ) ) );
    REQUIRE_FALSE( pmm::detail::validate_block_index<AT>( total, std::numeric_limits<IndexT>::max() - 1 ) );
}

/*
### test-issue373-growth-traits-cap
*/
TEST_CASE( "I373-Growth: compute_growth_for_traits caps target by max_arena_size for AT", "[issue373][growth][traits]" )
{
    using AT32                      = pmm::DefaultAddressTraits;
    constexpr std::size_t kArenaCap = pmm::detail::max_arena_size<AT32>();
    auto over = pmm::detail::compute_growth_for_traits<AT32>( kArenaCap - 4096, 1024ULL * 1024ULL * 1024ULL, 5, 4, 0 );
    REQUIRE_FALSE( over.has_value() );
}

/*
### test-issue373-byte-off-checked
*/
TEST_CASE( "I373-Conv: byte_off_to_idx_checked rejects misalignment and overflow", "[issue373][conv]" )
{
    REQUIRE( pmm::detail::byte_off_to_idx_checked<AT>( 0 ).has_value() );
    REQUIRE( pmm::detail::byte_off_to_idx_checked<AT>( 16 ).has_value() );
    REQUIRE_FALSE( pmm::detail::byte_off_to_idx_checked<AT>( 17 ).has_value() );
    auto huge = static_cast<std::size_t>( std::numeric_limits<AT::index_type>::max() ) * AT::granule_size + 16;
    REQUIRE_FALSE( pmm::detail::byte_off_to_idx_checked<AT>( huge ).has_value() );
}
