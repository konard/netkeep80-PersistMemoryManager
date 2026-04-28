/**
 * @file test_issue87_phase1.cpp
 * @brief Phase 1 tests: AddressTraits<IndexType, GranuleSize>.
 *
 * After issue #373 the legacy overflow-as-zero `AddressTraits::bytes_to_granules`
 * helper was removed. The same conversions are now exercised through
 * `pmm::detail::bytes_to_granules_checked<AT>()`, which returns
 * `std::optional<GranuleCount<AT>>` so overflow is `nullopt` rather than `0`.
 */

#include "pmm_single_threaded_heap.h"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace
{
template <typename AT> typename AT::index_type checked_or_throw( std::size_t bytes )
{
    auto g = pmm::detail::bytes_to_granules_checked<AT>( bytes );
    if ( !g.has_value() )
        throw std::runtime_error( "bytes_to_granules_checked overflow" );
    return g->value;
}
} // namespace

// =============================================================================
// Phase 1 tests: AddressTraits
// =============================================================================

// ─── P1-A: static fields and types ─────────────────────────────────────────

TEST_CASE( "P1-A1: AddressTraits<uint8_t, 8>", "[test_issue87_phase1]" )
{
    using A = pmm::AddressTraits<std::uint8_t, 8>;
    static_assert( std::is_same<A::index_type, std::uint8_t>::value, "index_type must be uint8_t" );
    static_assert( A::granule_size == 8, "granule_size must be 8" );
    static_assert( A::no_block == 0xFFU, "no_block must be 0xFF for uint8_t" );
    static_assert( A::no_block == std::numeric_limits<std::uint8_t>::max(), "no_block must be max(index_type)" );
}

TEST_CASE( "P1-A2: SmallAddressTraits (uint16_t, 16)", "[test_issue87_phase1]" )
{
    using A = pmm::AddressTraits<std::uint16_t, 16>;
    static_assert( std::is_same<A::index_type, std::uint16_t>::value, "index_type must be uint16_t" );
    static_assert( A::granule_size == 16, "granule_size must be 16" );
    static_assert( A::no_block == 0xFFFFU, "no_block must be 0xFFFF for uint16_t" );
    static_assert( A::no_block == std::numeric_limits<std::uint16_t>::max(), "no_block must be max(index_type)" );
    static_assert( std::is_same<pmm::SmallAddressTraits, A>::value,
                   "SmallAddressTraits must be AddressTraits<uint16_t,16>" );
}

TEST_CASE( "P1-A3: DefaultAddressTraits (uint32_t, 16)", "[test_issue87_phase1]" )
{
    using A32 = pmm::AddressTraits<std::uint32_t, 16>;
    static_assert( std::is_same<A32::index_type, std::uint32_t>::value, "index_type must be uint32_t" );
    static_assert( A32::granule_size == 16, "granule_size must be 16" );
    static_assert( A32::no_block == 0xFFFFFFFFU, "no_block must be 0xFFFFFFFF for uint32_t" );
    static_assert( std::is_same<pmm::DefaultAddressTraits, A32>::value,
                   "DefaultAddressTraits must be AddressTraits<uint32_t,16>" );
}

TEST_CASE( "P1-A4: LargeAddressTraits (uint64_t, 64)", "[test_issue87_phase1]" )
{
    using A = pmm::AddressTraits<std::uint64_t, 64>;
    static_assert( std::is_same<A::index_type, std::uint64_t>::value, "index_type must be uint64_t" );
    static_assert( A::granule_size == 64, "granule_size must be 64" );
    static_assert( A::no_block == std::numeric_limits<std::uint64_t>::max(), "no_block must be max(uint64_t)" );
    static_assert( std::is_same<pmm::LargeAddressTraits, A>::value,
                   "LargeAddressTraits must be AddressTraits<uint64_t,64>" );
}

// ─── P1-B: bytes_to_granules_checked ───────────────────────────────────────

TEST_CASE( "P1-B1: bytes_to_granules_checked (tiny, 8-byte granule)", "[test_issue87_phase1]" )
{
    using A = pmm::AddressTraits<std::uint8_t, 8>;
    REQUIRE( checked_or_throw<A>( 0 ) == 0 );
    REQUIRE( checked_or_throw<A>( 1 ) == 1 );
    REQUIRE( checked_or_throw<A>( 8 ) == 1 );
    REQUIRE( checked_or_throw<A>( 9 ) == 2 );
    REQUIRE( checked_or_throw<A>( 16 ) == 2 );
    REQUIRE( checked_or_throw<A>( 17 ) == 3 );
    REQUIRE( checked_or_throw<A>( 254 * 8 ) == 254 );
}

TEST_CASE( "P1-B2: bytes_to_granules_checked (default, 16-byte granule)", "[test_issue87_phase1]" )
{
    using A = pmm::DefaultAddressTraits;
    REQUIRE( checked_or_throw<A>( 0 ) == 0 );
    REQUIRE( checked_or_throw<A>( 1 ) == 1 );
    REQUIRE( checked_or_throw<A>( 16 ) == 1 );
    REQUIRE( checked_or_throw<A>( 17 ) == 2 );
    REQUIRE( checked_or_throw<A>( 32 ) == 2 );
    REQUIRE( checked_or_throw<A>( 33 ) == 3 );
}

// ─── P1-C: granules_to_bytes ───────────────────────────────────────────────

TEST_CASE( "P1-C:  granules_to_bytes", "[test_issue87_phase1]" )
{
    using A8  = pmm::AddressTraits<std::uint8_t, 8>;
    using A32 = pmm::DefaultAddressTraits;
    REQUIRE( A8::granules_to_bytes( 0 ) == 0 );
    REQUIRE( A8::granules_to_bytes( 1 ) == 8 );
    REQUIRE( A8::granules_to_bytes( 2 ) == 16 );
    REQUIRE( A8::granules_to_bytes( 10 ) == 80 );
    REQUIRE( A32::granules_to_bytes( 1 ) == 16 );
    REQUIRE( A32::granules_to_bytes( 2 ) == 32 );
    REQUIRE( A32::granules_to_bytes( 100 ) == 1600 );
}

// ─── P1-D: idx_to_byte_off / byte_off_to_idx roundtrip ────────────────────

TEST_CASE( "P1-D:  idx_to_byte_off / byte_off_to_idx roundtrip", "[test_issue87_phase1]" )
{
    using A8  = pmm::AddressTraits<std::uint8_t, 8>;
    using A16 = pmm::SmallAddressTraits;
    using A32 = pmm::DefaultAddressTraits;

    REQUIRE( A8::idx_to_byte_off( 0 ) == 0 );
    REQUIRE( A8::idx_to_byte_off( 1 ) == 8 );
    REQUIRE( A8::idx_to_byte_off( 10 ) == 80 );
    REQUIRE( A8::byte_off_to_idx( 0 ) == 0 );
    REQUIRE( A8::byte_off_to_idx( 8 ) == 1 );
    REQUIRE( A8::byte_off_to_idx( 80 ) == 10 );

    REQUIRE( A16::idx_to_byte_off( 5 ) == 80 );
    REQUIRE( A16::byte_off_to_idx( 80 ) == 5 );

    for ( std::uint32_t idx : { 0u, 1u, 10u, 100u, 1000u } )
    {
        std::size_t byte_off = A32::idx_to_byte_off( idx );
        REQUIRE( A32::byte_off_to_idx( byte_off ) == idx );
        auto checked = pmm::detail::byte_off_to_idx_checked<A32>( byte_off );
        REQUIRE( checked.has_value() );
        REQUIRE( *checked == idx );
    }
}

// ─── P1-E: backward compatibility checks ──────────────────────────────────

TEST_CASE( "P1-E: DefaultAddressTraits matches current constants", "[test_issue87_phase1]" )
{
    using A = pmm::DefaultAddressTraits;
    static_assert( A::granule_size == pmm::kGranuleSize,
                   "DefaultAddressTraits::granule_size must match pmm::kGranuleSize" );
    static_assert( A::no_block == pmm::detail::kNoBlock,
                   "DefaultAddressTraits::no_block must match pmm::detail::kNoBlock" );
    static_assert( std::is_same<A::index_type, std::uint32_t>::value,
                   "DefaultAddressTraits::index_type must be uint32_t" );
    for ( std::size_t bytes : { std::size_t( 0 ), std::size_t( 1 ), std::size_t( 16 ), std::size_t( 17 ),
                                std::size_t( 256 ), std::size_t( 1024 ) } )
    {
        auto g = pmm::detail::bytes_to_granules_checked<A>( bytes );
        REQUIRE( g.has_value() );
        std::size_t expected = ( bytes + A::granule_size - 1 ) / A::granule_size;
        REQUIRE( g->value == expected );
    }
}

// ─── P1-F: various granule sizes ──────────────────────────────────────────

TEST_CASE( "P1-F: Various power-of-2 granule sizes (1, 4, 32, 512)", "[test_issue87_phase1]" )
{
    using A4 = pmm::AddressTraits<std::uint32_t, 4>;
    static_assert( A4::granule_size == 4 );
    REQUIRE( checked_or_throw<A4>( 4 ) == 1 );
    REQUIRE( checked_or_throw<A4>( 5 ) == 2 );

    using A32bytes = pmm::AddressTraits<std::uint32_t, 32>;
    static_assert( A32bytes::granule_size == 32 );
    REQUIRE( checked_or_throw<A32bytes>( 32 ) == 1 );
    REQUIRE( checked_or_throw<A32bytes>( 33 ) == 2 );

    using A512 = pmm::AddressTraits<std::uint32_t, 512>;
    static_assert( A512::granule_size == 512 );
    REQUIRE( checked_or_throw<A512>( 512 ) == 1 );
    REQUIRE( checked_or_throw<A512>( 513 ) == 2 );
    REQUIRE( A512::granules_to_bytes( 1 ) == 512 );
}
