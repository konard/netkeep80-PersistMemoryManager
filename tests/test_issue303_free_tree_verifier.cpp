#include "pmm/persist_memory_manager.h"
#include "pmm/pmm_presets.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>

using AT  = pmm::DefaultAddressTraits;
using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 303>;

static void setup_fragmented_free_tree()
{
    Mgr::destroy();
    REQUIRE( Mgr::create( 128 * 1024 ) );

    std::array<Mgr::pptr<std::uint8_t>, 4> gaps{};
    std::array<Mgr::pptr<std::uint8_t>, 5> barriers{};
    for ( std::size_t i = 0; i < gaps.size(); ++i )
    {
        gaps[i]     = Mgr::allocate_typed<std::uint8_t>( 256 + i * 64 );
        barriers[i] = Mgr::allocate_typed<std::uint8_t>( 32 );
        REQUIRE( !gaps[i].is_null() );
        REQUIRE( !barriers[i].is_null() );
    }
    barriers.back() = Mgr::allocate_typed<std::uint8_t>( 32 );
    REQUIRE( !barriers.back().is_null() );

    for ( auto gap : gaps )
        Mgr::deallocate_typed( gap );
}

static pmm::detail::ManagerHeader<AT>* header()
{
    constexpr std::size_t hdr_off =
        ( ( sizeof( pmm::Block<AT> ) + AT::granule_size - 1 ) / AT::granule_size ) * AT::granule_size;
    return reinterpret_cast<pmm::detail::ManagerHeader<AT>*>( Mgr::backend().base_ptr() + hdr_off );
}

static void* block_at( AT::index_type idx )
{
    return pmm::detail::block_at<AT>( Mgr::backend().base_ptr(), idx );
}

static bool has_free_tree_violation( const pmm::VerifyResult& result )
{
    for ( std::size_t i = 0; i < result.entry_count; ++i )
        if ( result.entries[i].type == pmm::ViolationType::FreeTreeStale )
            return true;
    return false;
}

TEST_CASE( "free-tree verifier detects invalid root", "[issue303][verify]" )
{
    setup_fragmented_free_tree();
    auto* hdr           = header();
    hdr->free_tree_root = AT::no_block;

    pmm::VerifyResult result = Mgr::verify();
    REQUIRE_FALSE( result.ok );
    REQUIRE( has_free_tree_violation( result ) );

    Mgr::destroy();
}

TEST_CASE( "free-tree verifier detects child link corruption", "[issue303][verify]" )
{
    setup_fragmented_free_tree();
    auto* hdr  = header();
    void* root = block_at( hdr->free_tree_root );
    pmm::BlockStateBase<AT>::set_left_offset_of( root, static_cast<AT::index_type>( 1 ) );

    pmm::VerifyResult result = Mgr::verify();
    REQUIRE_FALSE( result.ok );
    REQUIRE( has_free_tree_violation( result ) );

    Mgr::destroy();
}

TEST_CASE( "free-tree verifier detects parent link corruption", "[issue303][verify]" )
{
    setup_fragmented_free_tree();
    auto* hdr  = header();
    void* root = block_at( hdr->free_tree_root );
    pmm::BlockStateBase<AT>::set_parent_offset_of( root, hdr->first_block_offset );

    pmm::VerifyResult result = Mgr::verify();
    REQUIRE_FALSE( result.ok );
    REQUIRE( has_free_tree_violation( result ) );

    Mgr::destroy();
}

TEST_CASE( "free-tree verifier detects ordering corruption", "[issue303][verify]" )
{
    setup_fragmented_free_tree();
    auto* hdr   = header();
    void* root  = block_at( hdr->free_tree_root );
    auto  right = pmm::BlockStateBase<AT>::get_right_offset( root );
    REQUIRE( right != AT::no_block );

    pmm::BlockStateBase<AT>::set_left_offset_of( root, right );
    pmm::BlockStateBase<AT>::set_right_offset_of( root, AT::no_block );
    pmm::BlockStateBase<AT>::set_parent_offset_of( block_at( right ), hdr->free_tree_root );

    pmm::VerifyResult result = Mgr::verify();
    REQUIRE_FALSE( result.ok );
    REQUIRE( has_free_tree_violation( result ) );

    Mgr::destroy();
}

TEST_CASE( "free-tree verifier detects duplicate revisit", "[issue303][verify]" )
{
    setup_fragmented_free_tree();
    auto* hdr  = header();
    void* root = block_at( hdr->free_tree_root );
    pmm::BlockStateBase<AT>::set_left_offset_of( root, hdr->free_tree_root );

    pmm::VerifyResult result = Mgr::verify();
    REQUIRE_FALSE( result.ok );
    REQUIRE( has_free_tree_violation( result ) );

    Mgr::destroy();
}
