#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
namespace pmm
{
using std::size_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uint8_t;
/*
## pmm-addresstraits
*/
template <typename IndexT, size_t GranuleSz> struct AddressTraits
{
    static_assert( std::is_unsigned<IndexT>::value, "" );
    static_assert( GranuleSz >= 4, "" );
    static_assert( ( GranuleSz & ( GranuleSz - 1 ) ) == 0, "" );
    using index_type                         = IndexT;
    static constexpr size_t     granule_size = GranuleSz;
    static constexpr index_type no_block     = std::numeric_limits<IndexT>::max();
    static constexpr index_type bytes_to_granules( size_t bytes ) noexcept
    {
        if ( bytes == 0 )
            return static_cast<index_type>( 0 );
        if ( bytes > std::numeric_limits<size_t>::max() - ( granule_size - 1 ) )
            return static_cast<index_type>( 0 );
        size_t granules = ( bytes + granule_size - 1 ) / granule_size;
        if ( granules > static_cast<size_t>( std::numeric_limits<IndexT>::max() ) )
            return static_cast<index_type>( 0 );
        return static_cast<index_type>( granules );
    }
    static constexpr size_t granules_to_bytes( index_type granules ) noexcept
    {
        return static_cast<size_t>( granules ) * granule_size;
    }
    static constexpr size_t idx_to_byte_off( index_type idx ) noexcept
    {
        return static_cast<size_t>( idx ) * granule_size;
    }
    static index_type byte_off_to_idx( size_t byte_off ) noexcept
    {
        assert( byte_off % granule_size == 0 );
        assert( byte_off / granule_size <= static_cast<size_t>( std::numeric_limits<IndexT>::max() ) );
        return static_cast<index_type>( byte_off / granule_size );
    }
};
using SmallAddressTraits   = AddressTraits<uint16_t, 16>;
using DefaultAddressTraits = AddressTraits<uint32_t, 16>;
using LargeAddressTraits   = AddressTraits<uint64_t, 64>;
}
