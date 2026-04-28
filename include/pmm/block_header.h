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
namespace detail
{
template <typename AT, bool NeedsPadding> struct BlockHeaderStorage;
template <typename AT> struct BlockHeaderStorage<AT, false>
{
    using index_type = typename AT::index_type;
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
template <typename AT> constexpr std::size_t block_header_natural_size_v = sizeof( BlockHeaderStorage<AT, false> );
template <typename AT>
constexpr std::size_t block_header_target_size_v =
    ( ( block_header_natural_size_v<AT> + AT::granule_size - 1 ) / AT::granule_size ) * AT::granule_size;
template <typename AT>
constexpr bool block_header_needs_padding_v = block_header_natural_size_v<AT> != block_header_target_size_v<AT>;
template <typename AT> struct BlockHeaderStorage<AT, true>
{
    using index_type = typename AT::index_type;
    index_type   weight;
    index_type   left_offset;
    index_type   right_offset;
    index_type   parent_offset;
    index_type   root_offset;
    std::int16_t avl_height;
    uint16_t     node_type;
    index_type   prev_offset;
    index_type   next_offset;
    std::uint8_t _granule_padding[block_header_target_size_v<AT> - block_header_natural_size_v<AT>];
};
}
/*
## pmm-blockheader
*/
template <typename AT> struct BlockHeader : detail::BlockHeaderStorage<AT, detail::block_header_needs_padding_v<AT>>
{
    using address_traits = AT;
    using index_type     = typename AT::index_type;
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
    static_assert( ( sizeof( H ) % AT::granule_size ) == 0 );
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
static_assert( ( sizeof( BlockHeader<SmallAddressTraits> ) % SmallAddressTraits::granule_size ) == 0 );
static_assert( ( sizeof( BlockHeader<DefaultAddressTraits> ) % DefaultAddressTraits::granule_size ) == 0 );
static_assert( ( sizeof( BlockHeader<LargeAddressTraits> ) % LargeAddressTraits::granule_size ) == 0 );
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
}
