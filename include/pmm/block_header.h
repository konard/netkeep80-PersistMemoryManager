#pragma once
#include "pmm/address_traits.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
namespace pmm
{
enum : uint16_t
{
    kNodeReadWrite = 0,
    kNodeReadOnly  = 1,
};
/*
## pmm-blockheader
*/
template <typename AT> struct BlockHeader
{
    using address_traits = AT;
    using index_type     = typename AT::index_type;
    index_type   weight;
    index_type   left_offset;
    index_type   right_offset;
    index_type   parent_offset;
    index_type   root_offset;
    std::int16_t avl_height;
    uint16_t     node_type;
    index_type   prev_offset;
    index_type   next_offset;
};
/*
## pmm-blocklayoutcontract
*/
template <typename AT> struct BlockLayoutContract
{
    using H = BlockHeader<AT>;
    using I = typename AT::index_type;
    static_assert( std::is_standard_layout_v<H> );
    static_assert( std::is_trivially_copyable_v<H> );
    static_assert( std::is_unsigned_v<I> );
    static_assert( AT::granule_size >= alignof( H ) );
    static_assert( ( AT::granule_size % alignof( H ) ) == 0 );
    static constexpr std::size_t tree_slot_size = offsetof( H, prev_offset );
    static constexpr std::size_t layout_size    = sizeof( H );
};
static_assert( std::is_standard_layout_v<BlockHeader<DefaultAddressTraits>> );
static_assert( std::is_trivially_copyable_v<BlockHeader<DefaultAddressTraits>> );
static_assert( sizeof( BlockHeader<DefaultAddressTraits> ) == 32 );
static_assert( offsetof( BlockHeader<DefaultAddressTraits>, weight ) == 0 );
static_assert( offsetof( BlockHeader<DefaultAddressTraits>, left_offset ) == 4 );
static_assert( offsetof( BlockHeader<DefaultAddressTraits>, right_offset ) == 8 );
static_assert( offsetof( BlockHeader<DefaultAddressTraits>, parent_offset ) == 12 );
static_assert( offsetof( BlockHeader<DefaultAddressTraits>, root_offset ) == 16 );
static_assert( offsetof( BlockHeader<DefaultAddressTraits>, avl_height ) == 20 );
static_assert( offsetof( BlockHeader<DefaultAddressTraits>, node_type ) == 22 );
static_assert( offsetof( BlockHeader<DefaultAddressTraits>, prev_offset ) == 24 );
static_assert( offsetof( BlockHeader<DefaultAddressTraits>, next_offset ) == 28 );
namespace detail
{
template <typename AT> inline BlockHeader<AT>* block_header_at( void* raw ) noexcept
{
    assert( raw != nullptr );
    assert( reinterpret_cast<std::uintptr_t>( raw ) % alignof( BlockHeader<AT> ) == 0 );
    return reinterpret_cast<BlockHeader<AT>*>( raw );
}
template <typename AT> inline const BlockHeader<AT>* block_header_at( const void* raw ) noexcept
{
    assert( raw != nullptr );
    assert( reinterpret_cast<std::uintptr_t>( raw ) % alignof( BlockHeader<AT> ) == 0 );
    return reinterpret_cast<const BlockHeader<AT>*>( raw );
}
}
/*
## pmm-blocktreeaccess
*/
template <typename AT> struct BlockTreeAccess
{
    using Header     = BlockHeader<AT>;
    using index_type = typename AT::index_type;
    static index_type   left( const Header& h ) noexcept { return h.left_offset; }
    static index_type   right( const Header& h ) noexcept { return h.right_offset; }
    static index_type   parent( const Header& h ) noexcept { return h.parent_offset; }
    static std::int16_t height( const Header& h ) noexcept { return h.avl_height; }
    static void         set_left( Header& h, index_type v ) noexcept { h.left_offset = v; }
    static void         set_right( Header& h, index_type v ) noexcept { h.right_offset = v; }
    static void         set_parent( Header& h, index_type v ) noexcept { h.parent_offset = v; }
    static void         set_height( Header& h, std::int16_t v ) noexcept { h.avl_height = v; }
};
}
