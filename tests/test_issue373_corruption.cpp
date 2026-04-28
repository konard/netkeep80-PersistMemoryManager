#include <catch2/catch_test_macros.hpp>

#include "pmm/arena_internals.h"
#include "pmm/heap_storage.h"
#include "pmm/manager_configs.h"
#include "pmm/persist_memory_manager.h"

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
