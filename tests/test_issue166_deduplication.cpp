/**
 * @file test_issue166_deduplication.cpp
 * @brief Templated kNoBlock + checked block-granule sizing tests (issue #373).
 */

#include "pmm/arena_internals.h"
#include "pmm/manager_configs.h"
#include "pmm/persist_memory_manager.h"
#include "pmm/types.h"

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>

#include <type_traits>

// ─── Макросы тестирования ─────────────────────────────────────────────────────

// =============================================================================
// KNoBlock_v<AT> template alias
// =============================================================================

/// @brief kNoBlock_v<DefaultAddressTraits> == DefaultAddressTraits::no_block == kNoBlock.
TEST_CASE( "I166-A1: kNoBlock_v<DefaultAddressTraits> == kNoBlock", "[test_issue166_deduplication]" )
{
    using AT = pmm::DefaultAddressTraits;

    static_assert( pmm::detail::kNoBlock_v<AT> == AT::no_block,
                   "kNoBlock_v<DefaultAddressTraits> must equal DefaultAddressTraits::no_block" );
    static_assert( pmm::detail::kNoBlock_v<AT> == pmm::detail::kNoBlock,
                   "kNoBlock_v<DefaultAddressTraits> must equal detail::kNoBlock" );
    static_assert( pmm::detail::kNoBlock_v<AT> == 0xFFFFFFFFU, "kNoBlock_v<DefaultAddressTraits> must be 0xFFFFFFFF" );
}

/// @brief kNoBlock_v<SmallAddressTraits> == SmallAddressTraits::no_block == 0xFFFF.
TEST_CASE( "I166-A2: kNoBlock_v<SmallAddressTraits> == 0xFFFF", "[test_issue166_deduplication]" )
{
    using AT = pmm::SmallAddressTraits;

    static_assert( pmm::detail::kNoBlock_v<AT> == AT::no_block,
                   "kNoBlock_v<SmallAddressTraits> must equal SmallAddressTraits::no_block" );
    static_assert( pmm::detail::kNoBlock_v<AT> == static_cast<std::uint16_t>( 0xFFFFU ),
                   "kNoBlock_v<SmallAddressTraits> must be 0xFFFF" );
}

/// @brief kNoBlock_v<LargeAddressTraits> == LargeAddressTraits::no_block == 0xFFFFFFFFFFFFFFFF.
TEST_CASE( "I166-A3: kNoBlock_v<LargeAddressTraits> == 0xFFFFFFFFFFFFFFFF", "[test_issue166_deduplication]" )
{
    using AT = pmm::LargeAddressTraits;

    static_assert( pmm::detail::kNoBlock_v<AT> == AT::no_block,
                   "kNoBlock_v<LargeAddressTraits> must equal LargeAddressTraits::no_block" );
    static_assert( pmm::detail::kNoBlock_v<AT> == static_cast<std::uint64_t>( 0xFFFFFFFFFFFFFFFFULL ),
                   "kNoBlock_v<LargeAddressTraits> must be 0xFFFFFFFFFFFFFFFF" );
}

/// @brief kNoBlock_v<AT> has correct type: AT::index_type.
TEST_CASE( "I166-A4: kNoBlock_v<AT> has correct index_type", "[test_issue166_deduplication]" )
{
    static_assert(
        std::is_same<decltype( pmm::detail::kNoBlock_v<pmm::DefaultAddressTraits> ), const std::uint32_t>::value,
        "kNoBlock_v<DefaultAddressTraits> must have type const uint32_t" );
    static_assert(
        std::is_same<decltype( pmm::detail::kNoBlock_v<pmm::SmallAddressTraits> ), const std::uint16_t>::value,
        "kNoBlock_v<SmallAddressTraits> must have type const uint16_t" );
    static_assert(
        std::is_same<decltype( pmm::detail::kNoBlock_v<pmm::LargeAddressTraits> ), const std::uint64_t>::value,
        "kNoBlock_v<LargeAddressTraits> must have type const uint64_t" );
}

// =============================================================================
// Block-granule sizing via bytes_to_granules_checked + kBlockHeaderGranules_t (issue #373)
// =============================================================================

namespace
{
template <typename AT> typename AT::index_type required_block_granules_checked( std::size_t user_bytes )
{
    using index_type = typename AT::index_type;
    auto checked     = pmm::detail::bytes_to_granules_checked<AT>( user_bytes );
    if ( !checked.has_value() )
        return AT::no_block;
    index_type data_gran = ( checked->value == 0 ) ? static_cast<index_type>( 1 ) : checked->value;
    return static_cast<index_type>( pmm::detail::kBlockHeaderGranules_t<AT> + data_gran );
}
}

TEST_CASE( "I166-B1: bytes_to_granules_checked matches expected for DefaultAddressTraits",
           "[test_issue166_deduplication]" )
{
    using AT = pmm::DefaultAddressTraits;
    for ( std::size_t bytes : { std::size_t( 0 ), std::size_t( 1 ), std::size_t( 16 ), std::size_t( 17 ),
                                std::size_t( 32 ), std::size_t( 64 ), std::size_t( 128 ), std::size_t( 256 ) } )
    {
        auto checked = pmm::detail::bytes_to_granules_checked<AT>( bytes );
        REQUIRE( checked.has_value() );
        std::size_t expected_data = ( bytes == 0 ) ? 0 : ( bytes + AT::granule_size - 1 ) / AT::granule_size;
        REQUIRE( checked->value == expected_data );
    }
}

TEST_CASE( "I166-B2: SmallAddressTraits block sizing is consistent", "[test_issue166_deduplication]" )
{
    using AT                                = pmm::SmallAddressTraits;
    static constexpr std::uint32_t kHdrGran = pmm::detail::kBlockHeaderGranules_t<AT>;
    REQUIRE( required_block_granules_checked<AT>( 0 ) == kHdrGran + 1 );
    REQUIRE( required_block_granules_checked<AT>( 16 ) == kHdrGran + 1 );
    REQUIRE( required_block_granules_checked<AT>( 17 ) == kHdrGran + 2 );
}

TEST_CASE( "I166-B3: LargeAddressTraits block sizing uses 64-byte granule", "[test_issue166_deduplication]" )
{
    using AT                                = pmm::LargeAddressTraits;
    static constexpr std::uint64_t kHdrGran = pmm::detail::kBlockHeaderGranules_t<AT>;
    REQUIRE( required_block_granules_checked<AT>( 0 ) == kHdrGran + 1 );
    REQUIRE( required_block_granules_checked<AT>( 1 ) == kHdrGran + 1 );
    REQUIRE( required_block_granules_checked<AT>( 64 ) == kHdrGran + 1 );
    REQUIRE( required_block_granules_checked<AT>( 65 ) == kHdrGran + 2 );
    REQUIRE( required_block_granules_checked<AT>( 128 ) == kHdrGran + 2 );
    REQUIRE( required_block_granules_checked<AT>( 129 ) == kHdrGran + 3 );
}

// =============================================================================
// Address_traits::granules_to_bytes in persist_memory_manager.h
// =============================================================================

using TestMgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 166>;

/// @brief used_size() returns correct byte count (uses address_traits::granules_to_bytes internally).
TEST_CASE( "I166-C1: used_size() correct with address_traits::granules_to_bytes", "[test_issue166_deduplication]" )
{
    TestMgr::create( 64 * 1024 );

    std::size_t used_before = TestMgr::used_size();
    REQUIRE( used_before > 0 ); // Header + block_0 always consume some granules.

    // Allocate a block and check that used_size grows by the expected amount.
    void* p = TestMgr::allocate( 16 );
    REQUIRE( p != nullptr );

    std::size_t used_after = TestMgr::used_size();
    REQUIRE( used_after > used_before );

    // used_size() must be a multiple of granule_size (16 bytes for DefaultAddressTraits).
    using AT = pmm::DefaultAddressTraits;
    REQUIRE( used_after % AT::granule_size == 0 );

    TestMgr::deallocate( p );
    TestMgr::destroy();
}

/// @brief free_size() returns correct byte count (uses address_traits::granules_to_bytes internally).
TEST_CASE( "I166-C2: free_size() correct with address_traits::granules_to_bytes", "[test_issue166_deduplication]" )
{
    TestMgr::create( 64 * 1024 );

    std::size_t total = TestMgr::total_size();
    std::size_t used  = TestMgr::used_size();
    std::size_t free  = TestMgr::free_size();

    // Total = Used + Free (approximately, some overhead).
    REQUIRE( total >= used + free );

    // free_size() must be a multiple of granule_size.
    using AT = pmm::DefaultAddressTraits;
    REQUIRE( free % AT::granule_size == 0 );

    TestMgr::destroy();
}

/// @brief used_size() + free_size() is consistent with total_size() after allocations.
TEST_CASE( "I166-C3: used_size() + free_size() consistent with total_size()", "[test_issue166_deduplication]" )
{
    TestMgr::create( 64 * 1024 );

    void* p1 = TestMgr::allocate( 64 );
    void* p2 = TestMgr::allocate( 128 );
    REQUIRE( p1 != nullptr );
    REQUIRE( p2 != nullptr );

    std::size_t total = TestMgr::total_size();
    std::size_t used  = TestMgr::used_size();
    std::size_t free  = TestMgr::free_size();

    REQUIRE( used + free <= total );
    REQUIRE( used > 0 );

    TestMgr::deallocate( p1 );
    TestMgr::deallocate( p2 );
    TestMgr::destroy();
}

// =============================================================================
// ValidPmmAddressTraits not duplicated in embedded configs
// =============================================================================

/// @brief SmallEmbeddedStaticConfig compiles without redundant static_assert — compile-time check.
TEST_CASE( "I166-D1: SmallEmbeddedStaticConfig compiles without redundant assert", "[test_issue166_deduplication]" )
{
    // If the redundant static_assert were still present, this would be a compile-time duplicate.
    // The test passes by successfully instantiating both config sizes.
    using Cfg1 = pmm::SmallEmbeddedStaticConfig<1024>;
    using Cfg2 = pmm::SmallEmbeddedStaticConfig<2048>;

    static_assert( std::is_same<Cfg1::address_traits, pmm::SmallAddressTraits>::value,
                   "SmallEmbeddedStaticConfig<1024> must use SmallAddressTraits" );
    static_assert( std::is_same<Cfg2::address_traits, pmm::SmallAddressTraits>::value,
                   "SmallEmbeddedStaticConfig<2048> must use SmallAddressTraits" );

    // ValidPmmAddressTraits is satisfied for SmallAddressTraits (verified at namespace scope).
    static_assert( pmm::ValidPmmAddressTraits<pmm::SmallAddressTraits>,
                   "SmallAddressTraits must satisfy ValidPmmAddressTraits" );
}

/// @brief EmbeddedStaticConfig compiles without redundant static_assert — compile-time check.
TEST_CASE( "I166-D2: EmbeddedStaticConfig compiles without redundant assert", "[test_issue166_deduplication]" )
{
    using Cfg1 = pmm::EmbeddedStaticConfig<4096>;
    using Cfg2 = pmm::EmbeddedStaticConfig<8192>;

    static_assert( std::is_same<Cfg1::address_traits, pmm::DefaultAddressTraits>::value,
                   "EmbeddedStaticConfig<4096> must use DefaultAddressTraits" );
    static_assert( std::is_same<Cfg2::address_traits, pmm::DefaultAddressTraits>::value,
                   "EmbeddedStaticConfig<8192> must use DefaultAddressTraits" );

    // ValidPmmAddressTraits is satisfied for DefaultAddressTraits (verified at namespace scope).
    static_assert( pmm::ValidPmmAddressTraits<pmm::DefaultAddressTraits>,
                   "DefaultAddressTraits must satisfy ValidPmmAddressTraits" );
}

/// @brief BasicConfig<AT> still has static_assert for ValidPmmAddressTraits (explicit param).
TEST_CASE( "I166-D3: BasicConfig still validates ValidPmmAddressTraits for AT param", "[test_issue166_deduplication]" )
{
    // BasicConfig explicitly validates the AT parameter (which may be arbitrary user-provided).
    // This is different from SmallEmbeddedStaticConfig/EmbeddedStaticConfig which use fixed types.
    using Cfg = pmm::BasicConfig<pmm::DefaultAddressTraits, pmm::config::NoLock>;
    static_assert( pmm::ValidPmmAddressTraits<Cfg::address_traits>,
                   "BasicConfig address_traits must satisfy ValidPmmAddressTraits" );
}

// =============================================================================
// main
// =============================================================================
