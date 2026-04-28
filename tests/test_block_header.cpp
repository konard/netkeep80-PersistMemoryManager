/**
 * @file test_block_header.cpp
 * @brief Issue #369: Tests for the refactored BlockHeader<AT> layout, NodeType enum
 *        and weight-as-cached-size accounting.
 *
 * Verifies:
 *  - BlockHeader<AT> standard-layout, trivially-copyable invariants.
 *  - Required size and field offsets for DefaultAddressTraits — `avl_height`
 *    and `node_type` live at the very end of the header as compact `uint8_t`/
 *    `NodeType` fields.
 *  - BlockLayoutContract<AT> compiles for Small/Default/Large traits.
 *  - block_header_at<AT>() returns correctly aligned pointer.
 *  - Direct field reads/writes work and are observable via BlockStateBase API.
 *  - State transitions use direct typed access on BlockHeader<AT>&.
 *  - detect_block_state and recover_block_state remain functional.
 *
 * @see include/pmm/block_header.h
 * @see include/pmm/block_state.h
 */

#include "pmm/block_header.h"
#include "pmm/block_state.h"
#include "pmm/types.h"

#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

// ─── Compile-time layout contract ─────────────────────────────────────────────

TEST_CASE( "BlockHeader<DefaultAddressTraits> is standard-layout and trivially-copyable", "[issue369][block_header]" )
{
    using H = pmm::BlockHeader<pmm::DefaultAddressTraits>;
    static_assert( std::is_standard_layout_v<H> );
    static_assert( std::is_trivially_copyable_v<H> );
    static_assert( sizeof( H ) == 32 );
}

TEST_CASE( "BlockHeader<DefaultAddressTraits> field offsets match the binary contract", "[issue369][block_header]" )
{
    using H = pmm::BlockHeader<pmm::DefaultAddressTraits>;
    static_assert( offsetof( H, weight ) == 0 );
    static_assert( offsetof( H, left_offset ) == 4 );
    static_assert( offsetof( H, right_offset ) == 8 );
    static_assert( offsetof( H, parent_offset ) == 12 );
    static_assert( offsetof( H, root_offset ) == 16 );
    static_assert( offsetof( H, prev_offset ) == 20 );
    static_assert( offsetof( H, next_offset ) == 24 );
    static_assert( offsetof( H, avl_height ) == sizeof( H ) - 2 );
    static_assert( offsetof( H, node_type ) == sizeof( H ) - 1 );
}

TEST_CASE( "BlockHeader<DefaultAddressTraits> avl_height/node_type are compact and at the very end",
           "[issue369][block_header]" )
{
    using H = pmm::BlockHeader<pmm::DefaultAddressTraits>;
    static_assert( std::is_same_v<decltype( std::declval<H>().avl_height ), std::uint8_t> );
    static_assert( std::is_same_v<decltype( std::declval<H>().node_type ), pmm::NodeType> );
    static_assert( sizeof( std::declval<H>().avl_height ) == 1 );
    static_assert( sizeof( std::declval<H>().node_type ) == 1 );
    static_assert( offsetof( H, node_type ) == sizeof( H ) - 1 );
    static_assert( offsetof( H, avl_height ) == sizeof( H ) - 2 );
}

TEST_CASE( "BlockLayoutContract compiles for Small/Default/Large address traits", "[issue369][block_header]" )
{
    using SC = pmm::BlockLayoutContract<pmm::SmallAddressTraits>;
    using DC = pmm::BlockLayoutContract<pmm::DefaultAddressTraits>;
    using LC = pmm::BlockLayoutContract<pmm::LargeAddressTraits>;
    static_assert( DC::layout_size == 32 );
    static_assert( DC::tree_slot_size == 20 );
    static_assert( SC::layout_size > 0 );
    static_assert( LC::layout_size > 0 );
}

TEST_CASE( "BlockHeader<AT> size is a whole number of granules for every public AddressTraits",
           "[issue369][block_header][granule_contract]" )
{
    static_assert( sizeof( pmm::BlockHeader<pmm::SmallAddressTraits> ) % pmm::SmallAddressTraits::granule_size == 0 );
    static_assert( sizeof( pmm::BlockHeader<pmm::DefaultAddressTraits> ) % pmm::DefaultAddressTraits::granule_size ==
                   0 );
    static_assert( sizeof( pmm::BlockHeader<pmm::LargeAddressTraits> ) % pmm::LargeAddressTraits::granule_size == 0 );
    static_assert( sizeof( pmm::BlockHeader<pmm::DefaultAddressTraits> ) == 32 );
}

// ─── Direct access via block_header_at<AT> ────────────────────────────────────

TEST_CASE( "block_header_at<AT> returns aligned pointer to BlockHeader", "[issue367][block_header]" )
{
    using A = pmm::DefaultAddressTraits;
    using H = pmm::BlockHeader<A>;
    alignas( H ) std::uint8_t buffer[sizeof( H )]{};

    H* h = pmm::detail::block_header_at<A>( buffer );
    REQUIRE( h != nullptr );
    REQUIRE( reinterpret_cast<std::uintptr_t>( h ) % alignof( H ) == 0 );

    h->weight       = 7;
    h->left_offset  = 11;
    h->right_offset = 13;

    const H* ch = pmm::detail::block_header_at<A>( static_cast<const void*>( buffer ) );
    REQUIRE( ch->weight == 7 );
    REQUIRE( ch->left_offset == 11 );
    REQUIRE( ch->right_offset == 13 );
}

TEST_CASE( "BlockStateBase reads observe direct field writes", "[issue367][block_header]" )
{
    using A          = pmm::DefaultAddressTraits;
    using H          = pmm::BlockHeader<A>;
    using BlockState = pmm::BlockStateBase<A>;
    alignas( H ) std::uint8_t buffer[sizeof( H )]{};

    H* h             = pmm::detail::block_header_at<A>( buffer );
    h->weight        = 5;
    h->left_offset   = 11;
    h->right_offset  = 13;
    h->parent_offset = 17;
    h->root_offset   = 19;
    h->avl_height    = 3;
    h->node_type     = pmm::NodeType::ReadOnlyLocked;
    h->prev_offset   = 23;
    h->next_offset   = 29;

    REQUIRE( BlockState::get_weight( buffer ) == 5u );
    REQUIRE( BlockState::get_left_offset( buffer ) == 11u );
    REQUIRE( BlockState::get_right_offset( buffer ) == 13u );
    REQUIRE( BlockState::get_parent_offset( buffer ) == 17u );
    REQUIRE( BlockState::get_root_offset( buffer ) == 19u );
    REQUIRE( BlockState::get_avl_height( buffer ) == 3 );
    REQUIRE( BlockState::get_node_type( buffer ) == pmm::NodeType::ReadOnlyLocked );
    REQUIRE( BlockState::get_prev_offset( buffer ) == 23u );
    REQUIRE( BlockState::get_next_offset( buffer ) == 29u );
}

// ─── Direct AVL-slot field reads/writes on BlockHeader<AT> ────────────────────

TEST_CASE( "BlockHeader AVL slot fields are read and written directly", "[issue367][block_header]" )
{
    using A = pmm::DefaultAddressTraits;
    using H = pmm::BlockHeader<A>;
    H h{};
    h.left_offset   = 11;
    h.right_offset  = 13;
    h.parent_offset = 17;
    h.avl_height    = 4;
    REQUIRE( h.left_offset == 11u );
    REQUIRE( h.right_offset == 13u );
    REQUIRE( h.parent_offset == 17u );
    REQUIRE( h.avl_height == 4 );
}

// ─── State transitions over BlockHeader views ─────────────────────────────────

TEST_CASE( "FreeBlock -> FreeBlockRemovedAVL -> AllocatedBlock transitions", "[issue367][block_header]" )
{
    using A = pmm::DefaultAddressTraits;
    using H = pmm::BlockHeader<A>;
    alignas( H ) std::uint8_t buffer[sizeof( H )]{};

    auto fb      = pmm::FreeBlock<A>::cast_from_raw( buffer );
    auto removed = fb.remove_from_avl();
    auto alloc   = removed.mark_as_allocated( /*data_granules=*/5, /*own_idx=*/6 );
    REQUIRE( alloc.weight() == 5u );
    REQUIRE( alloc.root_offset() == 6u );
    REQUIRE( alloc.verify_invariants( 6 ) );
    REQUIRE_FALSE( alloc.verify_invariants( 7 ) );
}

TEST_CASE( "AllocatedBlock -> FreeBlockNotInAVL -> FreeBlock transitions", "[issue369][block_header]" )
{
    using A          = pmm::DefaultAddressTraits;
    using H          = pmm::BlockHeader<A>;
    using BlockState = pmm::BlockStateBase<A>;
    alignas( H ) std::uint8_t buffer[sizeof( H )]{};
    BlockState::set_weight_of( buffer, 5u );
    BlockState::set_root_offset_of( buffer, 6u );
    BlockState::set_node_type_of( buffer, pmm::NodeType::Generic );

    auto alloc   = pmm::AllocatedBlock<A>::cast_from_raw( buffer );
    auto not_avl = alloc.mark_as_free( /*total_granules=*/8u );
    REQUIRE( not_avl.weight() == 8u );
    REQUIRE( not_avl.root_offset() == 0u );
    REQUIRE( BlockState::get_node_type( buffer ) == pmm::NodeType::Free );

    auto fb = not_avl.insert_to_avl();
    REQUIRE( fb.avl_height() == 1 );
    REQUIRE( fb.is_free() );
}

TEST_CASE( "SplittingBlock initializes new block and links it into the chain", "[issue369][block_header]" )
{
    using A          = pmm::DefaultAddressTraits;
    using H          = pmm::BlockHeader<A>;
    using BlockState = pmm::BlockStateBase<A>;

    alignas( H ) std::uint8_t buffer_curr[sizeof( H )]{};
    alignas( H ) std::uint8_t buffer_new[sizeof( H )]{};
    alignas( H ) std::uint8_t buffer_old_next[sizeof( H )]{};

    BlockState::set_next_offset_of( buffer_curr, 100u );
    BlockState::set_prev_offset_of( buffer_old_next, 6u );

    auto splitting = pmm::SplittingBlock<A>::cast_from_raw( buffer_curr );
    splitting.initialize_new_block( buffer_new, /*new_idx=*/8, /*own_idx=*/6, /*new_block_total_granules=*/4u );
    REQUIRE( BlockState::get_prev_offset( buffer_new ) == 6u );
    REQUIRE( BlockState::get_next_offset( buffer_new ) == 100u );
    REQUIRE( BlockState::get_avl_height( buffer_new ) == 1 );
    REQUIRE( BlockState::get_weight( buffer_new ) == 4u );
    REQUIRE( BlockState::get_node_type( buffer_new ) == pmm::NodeType::Free );

    splitting.link_new_block( buffer_old_next, 8 );
    REQUIRE( splitting.next_offset() == 8u );
    REQUIRE( BlockState::get_prev_offset( buffer_old_next ) == 8u );

    auto alloc = splitting.finalize_split( /*data_granules=*/5, /*own_idx=*/6 );
    REQUIRE( alloc.weight() == 5u );
    REQUIRE( alloc.root_offset() == 6u );
    REQUIRE( BlockState::get_node_type( buffer_curr ) == pmm::NodeType::Generic );
}

TEST_CASE( "CoalescingBlock merges with a free next neighbour", "[issue369][block_header]" )
{
    using A          = pmm::DefaultAddressTraits;
    using H          = pmm::BlockHeader<A>;
    using BlockState = pmm::BlockStateBase<A>;

    alignas( H ) std::uint8_t buffer_curr[sizeof( H )]{};
    alignas( H ) std::uint8_t buffer_next[sizeof( H )]{};
    alignas( H ) std::uint8_t buffer_nxt_nxt[sizeof( H )]{};

    // Both blocks start free with cached sizes 4 and 6.
    BlockState::set_next_offset_of( buffer_curr, 10u );
    BlockState::set_weight_of( buffer_curr, 4u );
    BlockState::set_node_type_of( buffer_curr, pmm::NodeType::Free );
    BlockState::set_prev_offset_of( buffer_next, 6u );
    BlockState::set_next_offset_of( buffer_next, 20u );
    BlockState::set_weight_of( buffer_next, 6u );
    BlockState::set_node_type_of( buffer_next, pmm::NodeType::Free );
    BlockState::set_prev_offset_of( buffer_nxt_nxt, 10u );

    auto coalescing = pmm::CoalescingBlock<A>::cast_from_raw( buffer_curr );
    coalescing.coalesce_with_next( buffer_next, buffer_nxt_nxt, /*own_idx=*/6, /*next_block_granules=*/6u );
    REQUIRE( coalescing.next_offset() == 20u );
    REQUIRE( BlockState::get_prev_offset( buffer_nxt_nxt ) == 6u );
    REQUIRE( BlockState::get_weight( buffer_curr ) == 10u );
    for ( size_t i = 0; i < sizeof( buffer_next ); ++i )
        REQUIRE( buffer_next[i] == 0 );
}

TEST_CASE( "detect_block_state and recover_block_state behave consistently", "[issue369][block_header]" )
{
    using A          = pmm::DefaultAddressTraits;
    using H          = pmm::BlockHeader<A>;
    using BlockState = pmm::BlockStateBase<A>;

    alignas( H ) std::uint8_t buffer[sizeof( H )]{};
    REQUIRE( pmm::detect_block_state<A>( buffer, 6 ) == 0 );

    BlockState::set_weight_of( buffer, 5u );
    BlockState::set_root_offset_of( buffer, 6u );
    BlockState::set_node_type_of( buffer, pmm::NodeType::Generic );
    REQUIRE( pmm::detect_block_state<A>( buffer, 6 ) == 1 );
    REQUIRE( pmm::detect_block_state<A>( buffer, 7 ) == -1 );

    // Inconsistent: free node_type but root_offset!=0 — recover_block_state must repair.
    BlockState::set_weight_of( buffer, 0u );
    BlockState::set_root_offset_of( buffer, 6u );
    BlockState::set_node_type_of( buffer, pmm::NodeType::Free );
    pmm::recover_block_state<A>( buffer, 6 );
    REQUIRE( BlockState::get_root_offset( buffer ) == 0u );

    // Inconsistent: allocated node_type but root_offset!=own_idx — recover_block_state must repair.
    BlockState::set_weight_of( buffer, 5u );
    BlockState::set_root_offset_of( buffer, 10u );
    BlockState::set_node_type_of( buffer, pmm::NodeType::Generic );
    pmm::recover_block_state<A>( buffer, 6 );
    REQUIRE( BlockState::get_root_offset( buffer ) == 6u );
}

// ─── Checked cast_from_raw boundaries ────────────────────────────────────────

TEST_CASE( "FreeBlock::can_cast_from_raw / try_cast_from_raw reject invalid input", "[issue369][block_header][safety]" )
{
    using A          = pmm::DefaultAddressTraits;
    using H          = pmm::BlockHeader<A>;
    using BlockState = pmm::BlockStateBase<A>;
    alignas( H ) std::uint8_t buffer[sizeof( H )]{};

    // nullptr is rejected without UB even in release builds.
    REQUIRE_FALSE( pmm::FreeBlock<A>::can_cast_from_raw( nullptr ) );
    REQUIRE_FALSE( pmm::FreeBlock<A>::try_cast_from_raw( nullptr ).has_value() );

    // Misaligned raw pointer is rejected.
    alignas( H ) std::uint8_t big_buffer[sizeof( H ) + alignof( H )]{};
    void*                     misaligned = big_buffer + 1;
    REQUIRE_FALSE( pmm::FreeBlock<A>::can_cast_from_raw( misaligned ) );
    REQUIRE_FALSE( pmm::FreeBlock<A>::try_cast_from_raw( misaligned ).has_value() );

    // Zero-initialized buffer is a free block (NodeType::Free == 0).
    REQUIRE( pmm::FreeBlock<A>::can_cast_from_raw( buffer ) );
    auto opt = pmm::FreeBlock<A>::try_cast_from_raw( buffer );
    REQUIRE( opt.has_value() );
    REQUIRE( opt->is_free() );

    // After becoming allocated, FreeBlock cast must be rejected.
    BlockState::set_weight_of( buffer, 5u );
    BlockState::set_root_offset_of( buffer, 6u );
    BlockState::set_node_type_of( buffer, pmm::NodeType::Generic );
    REQUIRE_FALSE( pmm::FreeBlock<A>::can_cast_from_raw( buffer ) );
    REQUIRE_FALSE( pmm::FreeBlock<A>::try_cast_from_raw( buffer ).has_value() );
}

TEST_CASE( "AllocatedBlock::can_cast_from_raw / try_cast_from_raw reject invalid input",
           "[issue369][block_header][safety]" )
{
    using A          = pmm::DefaultAddressTraits;
    using H          = pmm::BlockHeader<A>;
    using BlockState = pmm::BlockStateBase<A>;
    alignas( H ) std::uint8_t buffer[sizeof( H )]{};

    // nullptr is rejected without UB even in release builds.
    REQUIRE_FALSE( pmm::AllocatedBlock<A>::can_cast_from_raw( nullptr ) );
    REQUIRE_FALSE( pmm::AllocatedBlock<A>::try_cast_from_raw( nullptr ).has_value() );

    // Misaligned raw pointer is rejected.
    alignas( H ) std::uint8_t big_buffer[sizeof( H ) + alignof( H )]{};
    void*                     misaligned = big_buffer + 1;
    REQUIRE_FALSE( pmm::AllocatedBlock<A>::can_cast_from_raw( misaligned ) );
    REQUIRE_FALSE( pmm::AllocatedBlock<A>::try_cast_from_raw( misaligned ).has_value() );

    // Zero-initialized buffer is a free block, not an allocated one.
    REQUIRE_FALSE( pmm::AllocatedBlock<A>::can_cast_from_raw( buffer ) );
    REQUIRE_FALSE( pmm::AllocatedBlock<A>::try_cast_from_raw( buffer ).has_value() );

    // After being marked allocated, AllocatedBlock cast must succeed.
    BlockState::set_weight_of( buffer, 5u );
    BlockState::set_root_offset_of( buffer, 6u );
    BlockState::set_node_type_of( buffer, pmm::NodeType::Generic );
    REQUIRE( pmm::AllocatedBlock<A>::can_cast_from_raw( buffer ) );
    auto opt = pmm::AllocatedBlock<A>::try_cast_from_raw( buffer );
    REQUIRE( opt.has_value() );
    REQUIRE( opt->verify_invariants( 6 ) );
}

// ─── Image version contract ───────────────────────────────────────────────────

TEST_CASE( "Current image version rejects legacy unversioned images", "[issue367][block_header]" )
{
    using namespace pmm::detail;
    static_assert( kCurrentImageVersion >= 2 );
    REQUIRE( is_supported_image_version( kCurrentImageVersion ) );
    REQUIRE_FALSE( is_supported_image_version( kLegacyUnversionedImageVersion ) );
    REQUIRE_FALSE( image_version_requires_migration( kCurrentImageVersion ) );
    REQUIRE_FALSE( image_version_requires_migration( kLegacyUnversionedImageVersion ) );
}
