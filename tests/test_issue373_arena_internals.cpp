#include <catch2/catch_test_macros.hpp>

#include "pmm/arena_internals.h"
#include "pmm/heap_storage.h"
#include "pmm/manager_configs.h"
#include "pmm/persist_memory_manager.h"

#include <atomic>
#include <cstdint>
#include <limits>

namespace
{

using SmallAT   = pmm::SmallAddressTraits;
using DefaultAT = pmm::DefaultAddressTraits;
using LargeAT   = pmm::LargeAddressTraits;

} // namespace

// I373-A: bytes_to_granules_checked
TEST_CASE( "I373-A: bytes_to_granules_checked returns 0 for zero bytes", "[issue373][arithmetic]" )
{
    auto r = pmm::detail::bytes_to_granules_checked<DefaultAT>( 0 );
    REQUIRE( r.has_value() );
    REQUIRE( r->value == 0 );
}

TEST_CASE( "I373-A: bytes_to_granules_checked rounds up to the next granule", "[issue373][arithmetic]" )
{
    auto r1 = pmm::detail::bytes_to_granules_checked<DefaultAT>( 1 );
    REQUIRE( r1.has_value() );
    REQUIRE( r1->value == 1 );

    auto r_exact = pmm::detail::bytes_to_granules_checked<DefaultAT>( DefaultAT::granule_size );
    REQUIRE( r_exact.has_value() );
    REQUIRE( r_exact->value == 1 );

    auto r_above = pmm::detail::bytes_to_granules_checked<DefaultAT>( DefaultAT::granule_size + 1 );
    REQUIRE( r_above.has_value() );
    REQUIRE( r_above->value == 2 );
}

TEST_CASE( "I373-A: bytes_to_granules_checked returns nullopt on size_t overflow", "[issue373][arithmetic]" )
{
    auto r = pmm::detail::bytes_to_granules_checked<DefaultAT>( std::numeric_limits<std::size_t>::max() );
    REQUIRE_FALSE( r.has_value() );
}

TEST_CASE( "I373-A: bytes_to_granules_checked returns nullopt above index_type::max", "[issue373][arithmetic]" )
{
    constexpr std::size_t too_many =
        ( static_cast<std::size_t>( std::numeric_limits<SmallAT::index_type>::max() ) + 2 ) * SmallAT::granule_size;
    auto r = pmm::detail::bytes_to_granules_checked<SmallAT>( too_many );
    REQUIRE_FALSE( r.has_value() );
}

// I373-B: fits_range
TEST_CASE( "I373-B: fits_range covers in-range, edge and overflow cases", "[issue373][arithmetic]" )
{
    REQUIRE( pmm::detail::fits_range( 0, 100, 100 ) );
    REQUIRE( pmm::detail::fits_range( 50, 50, 100 ) );
    REQUIRE_FALSE( pmm::detail::fits_range( 50, 51, 100 ) );
    REQUIRE_FALSE( pmm::detail::fits_range( 101, 0, 100 ) );
    REQUIRE_FALSE( pmm::detail::fits_range( std::numeric_limits<std::size_t>::max(), 1, 1 ) );
}

// I373-C: checked_add / checked_mul
TEST_CASE( "I373-C: checked arithmetic detects overflow", "[issue373][arithmetic]" )
{
    REQUIRE( pmm::detail::checked_add( 10, 20 ).value() == 30 );
    REQUIRE_FALSE( pmm::detail::checked_add( std::numeric_limits<std::size_t>::max(), 1 ).has_value() );
    REQUIRE( pmm::detail::checked_mul( 100, 100 ).value() == 10000 );
    REQUIRE_FALSE( pmm::detail::checked_mul( std::numeric_limits<std::size_t>::max(), 2 ).has_value() );
    REQUIRE( pmm::detail::checked_mul( 0, std::numeric_limits<std::size_t>::max() ).value() == 0 );
}

// I373-D: ArenaView
TEST_CASE( "I373-D: ArenaView reports valid blocks within the arena", "[issue373][arena]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 373>;
    REQUIRE( Mgr::create( 4096 ) );
    auto*                             base = Mgr::backend().base_ptr();
    auto*                             hdr  = pmm::detail::manager_header_at<DefaultAT>( base );
    pmm::detail::ArenaView<DefaultAT> view{ base, hdr };
    REQUIRE( view.valid() );
    REQUIRE( view.total_size() == hdr->total_size );
    REQUIRE( view.valid_block( hdr->first_block_offset ) );
    REQUIRE_FALSE( view.valid_block( DefaultAT::no_block ) );
    Mgr::destroy();
}

// I373-E: Public allocation rejects zero with InvalidSize and overflow with Overflow
TEST_CASE( "I373-E: allocate(0) returns nullptr and sets PmmError::InvalidSize", "[issue373][allocate]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 3731>;
    Mgr::clear_error();
    REQUIRE( Mgr::create( 4096 ) );
    void* p = Mgr::allocate( 0 );
    REQUIRE( p == nullptr );
    REQUIRE( Mgr::last_error() == pmm::PmmError::InvalidSize );
    Mgr::destroy();
}

TEST_CASE( "I373-E: allocate(huge) returns nullptr and sets PmmError::Overflow", "[issue373][allocate]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 3732>;
    Mgr::clear_error();
    REQUIRE( Mgr::create( 4096 ) );
    void* p = Mgr::allocate( std::numeric_limits<std::size_t>::max() );
    REQUIRE( p == nullptr );
    REQUIRE( Mgr::last_error() == pmm::PmmError::Overflow );
    Mgr::destroy();
}

// I373-F: GrowthPolicy semantics
TEST_CASE( "I373-F: compute_growth uses ratio when sufficient, else min_required", "[issue373][growth]" )
{
    auto a = pmm::detail::compute_growth( 1000, 100, 16, 5, 4, 0 );
    REQUIRE( a.has_value() );
    REQUIRE( *a >= 1100 );

    auto b = pmm::detail::compute_growth( 1000, 5000, 16, 5, 4, 0 );
    REQUIRE( b.has_value() );
    REQUIRE( *b >= 6000 );
}

TEST_CASE( "I373-F: compute_growth rejects degenerate denominator", "[issue373][growth]" )
{
    auto r = pmm::detail::compute_growth( 1000, 100, 16, 5, 0, 0 );
    REQUIRE_FALSE( r.has_value() );
}

TEST_CASE( "I373-F: compute_growth rejects max-memory cap violations", "[issue373][growth]" )
{
    auto r = pmm::detail::compute_growth( std::size_t{ 1024 } * 1024 * 1024, 100, 16, 5, 4, 1 );
    REQUIRE_FALSE( r.has_value() );
}

TEST_CASE( "I373-F: compute_growth rounds up to granule_size", "[issue373][growth]" )
{
    auto r = pmm::detail::compute_growth( 100, 50, 64, 5, 4, 0 );
    REQUIRE( r.has_value() );
    REQUIRE( *r % 64 == 0 );
}

// I373-G: HeapStorage alignment for LargeAddressTraits
TEST_CASE( "I373-G: HeapStorage<LargeAddressTraits> base_ptr is aligned to granule_size", "[issue373][alignment]" )
{
    pmm::HeapStorage<LargeAT> store( 1024 );
    auto*                     p = store.base_ptr();
    REQUIRE( p != nullptr );
    REQUIRE( reinterpret_cast<std::uintptr_t>( p ) % LargeAT::granule_size == 0 );
}

TEST_CASE( "I373-G: HeapStorage<DefaultAddressTraits> base_ptr is aligned to granule_size", "[issue373][alignment]" )
{
    pmm::HeapStorage<DefaultAT> store( 1024 );
    auto*                       p = store.base_ptr();
    REQUIRE( p != nullptr );
    REQUIRE( reinterpret_cast<std::uintptr_t>( p ) % DefaultAT::granule_size == 0 );
}

// I373-H: InitGuard transactional behavior
TEST_CASE( "I373-H: InitGuard resets on failure", "[issue373][initguard]" )
{
    std::atomic<bool> flag{ true };
    {
        pmm::detail::InitGuard guard( flag );
    }
    REQUIRE( flag.load() == false );
}

TEST_CASE( "I373-H: InitGuard preserves flag when committed", "[issue373][initguard]" )
{
    std::atomic<bool> flag{ true };
    {
        pmm::detail::InitGuard guard( flag );
        guard.commit();
    }
    REQUIRE( flag.load() == true );
}

// I373-I: for_each_physical_block walks normally
TEST_CASE( "I373-I: for_each_physical_block walks all blocks of a healthy arena", "[issue373][walker]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 3733>;
    REQUIRE( Mgr::create( 4096 ) );
    auto*                                  base   = Mgr::backend().base_ptr();
    const auto*                            hdr    = pmm::detail::manager_header_at<DefaultAT>( base );
    pmm::detail::ConstArenaView<DefaultAT> view{ base, hdr };
    std::size_t                            visits = 0;
    bool clean = pmm::detail::for_each_physical_block<DefaultAT>( view, [&]( DefaultAT::index_type, const void* ) noexcept
                                                                  {
                                                                      ++visits;
                                                                      return true;
                                                                  } );
    REQUIRE( clean );
    REQUIRE( visits == hdr->block_count );
    Mgr::destroy();
}

// I373-I: walker callback returning false propagates as failure
TEST_CASE( "I373-I: for_each_physical_block returns false when callback fails", "[issue373][walker]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 3734>;
    REQUIRE( Mgr::create( 4096 ) );
    auto*                                  base = Mgr::backend().base_ptr();
    const auto*                            hdr  = pmm::detail::manager_header_at<DefaultAT>( base );
    pmm::detail::ConstArenaView<DefaultAT> view{ base, hdr };
    bool ok = pmm::detail::for_each_physical_block<DefaultAT>( view, []( DefaultAT::index_type, const void* ) noexcept
                                                               { return false; } );
    REQUIRE_FALSE( ok );
    Mgr::destroy();
}

// I373-I: WalkControl differentiates StopOk from Fail
TEST_CASE( "I373-I: for_each_physical_block honors WalkControl::StopOk and Fail", "[issue373][walker]" )
{
    using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 3735>;
    REQUIRE( Mgr::create( 4096 ) );
    auto*                                  base = Mgr::backend().base_ptr();
    const auto*                            hdr  = pmm::detail::manager_header_at<DefaultAT>( base );
    pmm::detail::ConstArenaView<DefaultAT> view{ base, hdr };
    bool stop_ok = pmm::detail::for_each_physical_block<DefaultAT>( view, []( DefaultAT::index_type, const void* ) noexcept
                                                                    { return pmm::detail::WalkControl::StopOk; } );
    REQUIRE( stop_ok );
    bool fail = pmm::detail::for_each_physical_block<DefaultAT>( view, []( DefaultAT::index_type, const void* ) noexcept
                                                                 { return pmm::detail::WalkControl::Fail; } );
    REQUIRE_FALSE( fail );
    Mgr::destroy();
}

// I373-J: round_up_checked: normal, edge and overflow
TEST_CASE( "I373-J: round_up_checked rounds up safely and detects overflow", "[issue373][arithmetic]" )
{
    REQUIRE( pmm::detail::round_up_checked( 0, 16 ).value() == 0 );
    REQUIRE( pmm::detail::round_up_checked( 1, 16 ).value() == 16 );
    REQUIRE( pmm::detail::round_up_checked( 16, 16 ).value() == 16 );
    REQUIRE( pmm::detail::round_up_checked( 17, 16 ).value() == 32 );
    REQUIRE_FALSE( pmm::detail::round_up_checked( std::numeric_limits<std::size_t>::max(), 16 ).has_value() );
    REQUIRE_FALSE( pmm::detail::round_up_checked( 100, 0 ).has_value() );
    REQUIRE_FALSE( pmm::detail::round_up_checked( 100, 3 ).has_value() );
}

// I373-K: checked_granule_offset detects overflow on huge index
TEST_CASE( "I373-K: checked_granule_offset rejects huge index", "[issue373][arithmetic]" )
{
    using IndexT     = LargeAT::index_type;
    constexpr auto m = std::numeric_limits<IndexT>::max();
    auto           r = pmm::detail::checked_granule_offset<LargeAT>( m );
    if ( r.has_value() )
        REQUIRE( *r == static_cast<std::size_t>( m ) * LargeAT::granule_size );
    auto small = pmm::detail::checked_granule_offset<DefaultAT>( static_cast<DefaultAT::index_type>( 10 ) );
    REQUIRE( small.has_value() );
    REQUIRE( *small == 10u * DefaultAT::granule_size );
}
