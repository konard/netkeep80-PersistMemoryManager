/**
 * @file test_issue160_deduplication.cpp
 * @brief BasicConfig deduplication tests + checked granule conversion sanity (issue #373).
 */

#include "pmm/arena_internals.h"
#include "pmm/manager_configs.h"
#include "pmm/types.h"

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>

#include <type_traits>

// ─── Макросы тестирования ─────────────────────────────────────────────────────

// =============================================================================
// BasicConfig<> template
// =============================================================================

/// @brief BasicConfig создаёт конфигурацию с правильными типами по умолчанию.
TEST_CASE( "I160-A1: BasicConfig<> has correct default types", "[test_issue160_deduplication]" )
{
    using DefCfg = pmm::BasicConfig<>;

    static_assert( std::is_same<DefCfg::address_traits, pmm::DefaultAddressTraits>::value,
                   "BasicConfig<> address_traits must be DefaultAddressTraits" );
    static_assert( std::is_same<DefCfg::storage_backend, pmm::HeapStorage<pmm::DefaultAddressTraits>>::value,
                   "BasicConfig<> storage_backend must be HeapStorage<DefaultAddressTraits>" );
    static_assert( std::is_same<DefCfg::free_block_tree, pmm::AvlFreeTree<pmm::DefaultAddressTraits>>::value,
                   "BasicConfig<> free_block_tree must be AvlFreeTree<DefaultAddressTraits>" );
    static_assert( std::is_same<DefCfg::lock_policy, pmm::config::NoLock>::value,
                   "BasicConfig<> lock_policy must be NoLock" );
    static_assert( DefCfg::granule_size == 16, "BasicConfig<> granule_size must be 16" );
    static_assert( DefCfg::grow_numerator == pmm::config::kDefaultGrowNumerator,
                   "BasicConfig<> grow_numerator must be kDefaultGrowNumerator" );
    static_assert( DefCfg::grow_denominator == pmm::config::kDefaultGrowDenominator,
                   "BasicConfig<> grow_denominator must be kDefaultGrowDenominator" );
}

/// @brief BasicConfig с LargeAddressTraits и SharedMutexLock.
TEST_CASE( "I160-A2: BasicConfig<LargeAddressTraits,...> has correct types", "[test_issue160_deduplication]" )
{
    using LargeCfg = pmm::BasicConfig<pmm::LargeAddressTraits, pmm::config::SharedMutexLock, 2, 1, 0>;

    static_assert( std::is_same<LargeCfg::address_traits, pmm::LargeAddressTraits>::value,
                   "BasicConfig<LargeAddressTraits,...> address_traits must be LargeAddressTraits" );
    static_assert( std::is_same<LargeCfg::lock_policy, pmm::config::SharedMutexLock>::value,
                   "BasicConfig<...,SharedMutexLock,...> lock_policy must be SharedMutexLock" );
    static_assert( LargeCfg::granule_size == 64, "BasicConfig<LargeAddressTraits,...> granule_size must be 64" );
    static_assert( LargeCfg::grow_numerator == 2, "BasicConfig<...,2,...> grow_numerator must be 2" );
    static_assert( LargeCfg::grow_denominator == 1, "BasicConfig<...,1,...> grow_denominator must be 1" );
    static_assert( LargeCfg::max_memory_gb == 0, "BasicConfig<...,0> max_memory_gb must be 0" );
}

// =============================================================================
// Config aliases == BasicConfig specializations
// =============================================================================

/// @brief CacheManagerConfig — псевдоним BasicConfig<DefaultAddressTraits, NoLock, 5, 4, 64>.
TEST_CASE( "I160-B1: CacheManagerConfig is BasicConfig alias", "[test_issue160_deduplication]" )
{
    using Expected = pmm::BasicConfig<pmm::DefaultAddressTraits, pmm::config::NoLock,
                                      pmm::config::kDefaultGrowNumerator, pmm::config::kDefaultGrowDenominator, 64>;
    static_assert( std::is_same<pmm::CacheManagerConfig, Expected>::value,
                   "CacheManagerConfig must be BasicConfig<DefaultAddressTraits, NoLock, 5, 4, 64>" );
}

/// @brief PersistentDataConfig — псевдоним BasicConfig с SharedMutexLock.
TEST_CASE( "I160-B2: PersistentDataConfig is BasicConfig alias", "[test_issue160_deduplication]" )
{
    using Expected = pmm::BasicConfig<pmm::DefaultAddressTraits, pmm::config::SharedMutexLock,
                                      pmm::config::kDefaultGrowNumerator, pmm::config::kDefaultGrowDenominator, 64>;
    static_assert( std::is_same<pmm::PersistentDataConfig, Expected>::value,
                   "PersistentDataConfig must be BasicConfig<DefaultAddressTraits, SharedMutexLock, 5, 4, 64>" );
}

/// @brief EmbeddedManagerConfig — псевдоним BasicConfig с grow 3/2.
TEST_CASE( "I160-B3: EmbeddedManagerConfig is BasicConfig alias", "[test_issue160_deduplication]" )
{
    using Expected = pmm::BasicConfig<pmm::DefaultAddressTraits, pmm::config::NoLock, 3, 2, 64>;
    static_assert( std::is_same<pmm::EmbeddedManagerConfig, Expected>::value,
                   "EmbeddedManagerConfig must be BasicConfig<DefaultAddressTraits, NoLock, 3, 2, 64>" );
}

/// @brief IndustrialDBConfig — псевдоним BasicConfig с grow 2/1.
TEST_CASE( "I160-B4: IndustrialDBConfig is BasicConfig alias", "[test_issue160_deduplication]" )
{
    using Expected = pmm::BasicConfig<pmm::DefaultAddressTraits, pmm::config::SharedMutexLock, 2, 1, 64>;
    static_assert( std::is_same<pmm::IndustrialDBConfig, Expected>::value,
                   "IndustrialDBConfig must be BasicConfig<DefaultAddressTraits, SharedMutexLock, 2, 1, 64>" );
}

/// @brief LargeDBConfig — псевдоним BasicConfig с LargeAddressTraits.
TEST_CASE( "I160-B5: LargeDBConfig is BasicConfig alias", "[test_issue160_deduplication]" )
{
    using Expected = pmm::BasicConfig<pmm::LargeAddressTraits, pmm::config::SharedMutexLock, 2, 1, 0>;
    static_assert( std::is_same<pmm::LargeDBConfig, Expected>::value,
                   "LargeDBConfig must be BasicConfig<LargeAddressTraits, SharedMutexLock, 2, 1, 0>" );
}

// =============================================================================
// Byte/granule conversion deduplication (issue #373: checked-only API)
// =============================================================================

TEST_CASE( "I160-C1: bytes_to_granules_checked round-trip for DefaultAddressTraits", "[test_issue160_deduplication]" )
{
    using AT = pmm::DefaultAddressTraits;
    for ( std::size_t bytes : { std::size_t( 0 ), std::size_t( 1 ), std::size_t( 16 ), std::size_t( 17 ),
                                std::size_t( 128 ), std::size_t( 1000 ) } )
    {
        auto g = pmm::detail::bytes_to_granules_checked<AT>( bytes );
        REQUIRE( g.has_value() );
        std::size_t expected = ( bytes == 0 ) ? 0 : ( bytes + AT::granule_size - 1 ) / AT::granule_size;
        REQUIRE( g->value == expected );
    }
}

TEST_CASE( "I160-C2: granules_to_bytes is well-defined for DefaultAddressTraits", "[test_issue160_deduplication]" )
{
    using AT = pmm::DefaultAddressTraits;
    REQUIRE( AT::granules_to_bytes( 0 ) == 0 );
    REQUIRE( AT::granules_to_bytes( 1 ) == 16 );
    REQUIRE( AT::granules_to_bytes( 100 ) == 1600 );
}

TEST_CASE( "I160-C3: idx_to_byte_off is well-defined for DefaultAddressTraits", "[test_issue160_deduplication]" )
{
    using AT = pmm::DefaultAddressTraits;
    REQUIRE( AT::idx_to_byte_off( 0 ) == 0 );
    REQUIRE( AT::idx_to_byte_off( 1 ) == 16 );
    REQUIRE( AT::idx_to_byte_off( 100 ) == 1600 );
}

TEST_CASE( "I160-C4: byte_off_to_idx_checked round-trips for DefaultAddressTraits", "[test_issue160_deduplication]" )
{
    using AT = pmm::DefaultAddressTraits;
    for ( std::uint32_t idx : { 0u, 1u, 10u, 100u, 1000u } )
    {
        std::size_t byte_off = AT::idx_to_byte_off( idx );
        auto        checked  = pmm::detail::byte_off_to_idx_checked<AT>( byte_off );
        REQUIRE( checked.has_value() );
        REQUIRE( *checked == idx );
    }
}

TEST_CASE( "I160-C5: Conversion roundtrip idx->bytes->idx", "[test_issue160_deduplication]" )
{
    using AT = pmm::DefaultAddressTraits;
    for ( std::uint32_t idx : { 1u, 2u, 5u, 10u, 100u, 500u } )
    {
        std::size_t bytes = AT::granules_to_bytes( idx );
        auto        back  = pmm::detail::bytes_to_granules_checked<AT>( bytes );
        REQUIRE( back.has_value() );
        REQUIRE( back->value == idx );

        std::size_t byte_off = AT::idx_to_byte_off( idx );
        auto        idx_back = pmm::detail::byte_off_to_idx_checked<AT>( byte_off );
        REQUIRE( idx_back.has_value() );
        REQUIRE( *idx_back == idx );
    }
}

// =============================================================================
// Block_total_granules single templated implementation
// =============================================================================

/// @brief block_total_granules шаблон работает с DefaultAddressTraits.
TEST_CASE( "I160-D1: block_total_granules<AT> compiles for multiple traits", "[test_issue160_deduplication]" )
{
    // Verify that the templated block_total_granules compiles and instantiates correctly.
    // Direct invocation would require a live manager — just verify the template is available
    // for both default and custom traits at compile time.
    using AT1 = pmm::DefaultAddressTraits;
    using AT2 = pmm::SmallAddressTraits;

    // Verify function template specializations are reachable (compile-time check).
    // Return type is now AT::index_type and ManagerHeader is templated on AT.
    static_assert(
        std::is_same<decltype( &pmm::detail::block_total_granules<AT1> ),
                     typename AT1::index_type ( * )( const std::uint8_t*, const pmm::detail::ManagerHeader<AT1>*,
                                                     const pmm::Block<AT1>* )>::value,
        "block_total_granules<DefaultAddressTraits> must have correct signature " );
    static_assert(
        std::is_same<decltype( &pmm::detail::block_total_granules<AT2> ),
                     typename AT2::index_type ( * )( const std::uint8_t*, const pmm::detail::ManagerHeader<AT2>*,
                                                     const pmm::Block<AT2>* )>::value,
        "block_total_granules<SmallAddressTraits> must have correct signature " );
}

// =============================================================================
// main
// =============================================================================
