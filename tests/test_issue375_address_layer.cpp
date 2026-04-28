/**
 * @file test_issue375_address_layer.cpp
 * @brief Tests for the canonical ArenaAddress layer (issue #375).
 *
 * Verifies pointer/index conversions across SmallAddressTraits,
 * DefaultAddressTraits, LargeAddressTraits, including null/no_block
 * conventions, overflow guards, and round-trip identities.
 */

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pmm/arena_internals.h"
#include "pmm/types.h"

#include <cstdint>
#include <limits>
#include <type_traits>

using SmallAT   = pmm::SmallAddressTraits;
using DefaultAT = pmm::DefaultAddressTraits;
using LargeAT   = pmm::LargeAddressTraits;

namespace
{
template <typename AT> using Addr  = pmm::detail::ArenaAddress<AT>;
template <typename AT> using CAddr = pmm::detail::ConstArenaAddress<AT>;
} // namespace

TEMPLATE_TEST_CASE( "I375-A: null user idx (0) resolves to null user pointer", "[issue375][address]", SmallAT,
                    DefaultAT, LargeAT )
{
    std::uint8_t   dummy[1024] = {};
    Addr<TestType> addr{ dummy, std::size_t{ 1024 } };
    REQUIRE( addr.try_user_ptr( 0 ) == nullptr );
}

TEMPLATE_TEST_CASE( "I375-A: AT::no_block is rejected as physical block index", "[issue375][address]", SmallAT,
                    DefaultAT, LargeAT )
{
    std::uint8_t   dummy[1024] = {};
    Addr<TestType> addr{ dummy, std::size_t{ 1024 } };
    REQUIRE_FALSE( addr.valid_block( TestType::no_block ) );
    REQUIRE( addr.block( TestType::no_block ) == nullptr );
    REQUIRE( addr.try_user_ptr( TestType::no_block ) == nullptr );
}

TEMPLATE_TEST_CASE( "I375-A: valid first block index passes", "[issue375][address]", SmallAT, DefaultAT, LargeAT )
{
    std::uint8_t   dummy[2048] = {};
    Addr<TestType> addr{ dummy, std::size_t{ 2048 } };
    REQUIRE( addr.valid_block( 0 ) );
    REQUIRE( addr.block( 0 ) != nullptr );
}

TEMPLATE_TEST_CASE( "I375-A: out-of-range index fails without overflow", "[issue375][address]", SmallAT, DefaultAT,
                    LargeAT )
{
    std::uint8_t   dummy[1024] = {};
    Addr<TestType> addr{ dummy, std::size_t{ 1024 } };
    REQUIRE( addr.block( static_cast<typename TestType::index_type>( 1024 / TestType::granule_size + 10 ) ) ==
             nullptr );
}

TEMPLATE_TEST_CASE( "I375-A: idx*granule_size overflow fails cleanly", "[issue375][address]", SmallAT, DefaultAT,
                    LargeAT )
{
    std::uint8_t   dummy[1024] = {};
    Addr<TestType> addr{ dummy, std::size_t{ 1024 } };
    using IndexT = typename TestType::index_type;
    auto huge    = static_cast<IndexT>( std::numeric_limits<IndexT>::max() - 1 );
    REQUIRE( addr.block( huge ) == nullptr );
    REQUIRE( addr.try_user_ptr( huge ) == nullptr );
}

TEMPLATE_TEST_CASE( "I375-A: pointer before arena fails", "[issue375][address]", SmallAT, DefaultAT, LargeAT )
{
    std::uint8_t   dummy[1024] = {};
    Addr<TestType> addr{ dummy, std::size_t{ 1024 } };
    const void*    before = reinterpret_cast<const void*>( reinterpret_cast<std::uintptr_t>( dummy ) - 1 );
    REQUIRE_FALSE( addr.try_user_idx_from_raw( before ).has_value() );
}

TEMPLATE_TEST_CASE( "I375-A: pointer after arena fails", "[issue375][address]", SmallAT, DefaultAT, LargeAT )
{
    std::uint8_t   dummy[1024] = {};
    Addr<TestType> addr{ dummy, std::size_t{ 1024 } };
    const void*    after = reinterpret_cast<const void*>( reinterpret_cast<std::uintptr_t>( dummy ) + 2048 );
    REQUIRE_FALSE( addr.try_user_idx_from_raw( after ).has_value() );
}

TEMPLATE_TEST_CASE( "I375-A: unaligned user pointer fails", "[issue375][address]", SmallAT, DefaultAT, LargeAT )
{
    std::uint8_t   dummy[1024] = {};
    Addr<TestType> addr{ dummy, std::size_t{ 1024 } };
    REQUIRE_FALSE( addr.try_user_idx_from_raw( dummy + 1 ).has_value() );
}

TEMPLATE_TEST_CASE( "I375-A: valid user pointer round-trips to pptr index and back", "[issue375][address]", SmallAT,
                    DefaultAT, LargeAT )
{
    std::uint8_t   dummy[2048] = {};
    Addr<TestType> addr{ dummy, std::size_t{ 2048 } };
    auto*          target = dummy + TestType::granule_size * 5;
    auto           idx    = addr.try_user_idx_from_raw( target );
    REQUIRE( idx.has_value() );
    REQUIRE( addr.try_user_ptr( *idx ) == target );
}

TEMPLATE_TEST_CASE( "I375-A: block_idx <-> user_idx round-trip", "[issue375][address]", SmallAT, DefaultAT, LargeAT )
{
    std::uint8_t   dummy[2048] = {};
    Addr<TestType> addr{ dummy, std::size_t{ 2048 } };
    using IndexT = typename TestType::index_type;
    constexpr IndexT kHdrG =
        static_cast<IndexT>( ( sizeof( pmm::Block<TestType> ) + TestType::granule_size - 1 ) / TestType::granule_size );
    IndexT blk_idx  = 4;
    auto   user_idx = addr.try_user_idx_from_block_idx( blk_idx );
    REQUIRE( user_idx.has_value() );
    REQUIRE( *user_idx == static_cast<IndexT>( blk_idx + kHdrG ) );
    auto blk_back = addr.try_block_idx_from_user_idx( *user_idx );
    REQUIRE( blk_back.has_value() );
    REQUIRE( *blk_back == blk_idx );
}

TEMPLATE_TEST_CASE( "I375-A: ConstArenaAddress mirrors ArenaAddress on read paths", "[issue375][address]", SmallAT,
                    DefaultAT, LargeAT )
{
    std::uint8_t    dummy[1024] = {};
    CAddr<TestType> caddr{ dummy, std::size_t{ 1024 } };
    REQUIRE( caddr.block( 0 ) != nullptr );
    REQUIRE( caddr.try_user_ptr( 0 ) == nullptr );
    REQUIRE_FALSE( caddr.try_user_idx_from_raw( nullptr ).has_value() );
}

TEMPLATE_TEST_CASE( "I375-A: BlockHeader size/alignment preserved", "[issue375][address]", SmallAT, DefaultAT, LargeAT )
{
    REQUIRE( sizeof( pmm::Block<TestType> ) % TestType::granule_size == 0 );
}

TEST_CASE( "I375-A: kNullIdx_v == 0 and != AT::no_block for all traits", "[issue375][address]" )
{
    REQUIRE( pmm::detail::kNullIdx_v<SmallAT> == 0 );
    REQUIRE( pmm::detail::kNullIdx_v<DefaultAT> == 0 );
    REQUIRE( pmm::detail::kNullIdx_v<LargeAT> == 0 );
    REQUIRE( pmm::detail::kNullIdx_v<SmallAT> != SmallAT::no_block );
    REQUIRE( pmm::detail::kNullIdx_v<DefaultAT> != DefaultAT::no_block );
    REQUIRE( pmm::detail::kNullIdx_v<LargeAT> != LargeAT::no_block );
}

TEST_CASE( "I375-A: Default ArenaAddress (no header) can still answer total_size queries", "[issue375][address]" )
{
    std::uint8_t                         dummy[256] = {};
    pmm::detail::ArenaAddress<DefaultAT> addr{ dummy, std::size_t{ 256 } };
    REQUIRE( addr.total_size() == 256 );
    REQUIRE( addr.valid() );
    REQUIRE( addr.header() == nullptr );
}
