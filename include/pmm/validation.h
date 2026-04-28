#pragma once
#include "pmm/address_traits.h"
#include "pmm/block.h"
#include "pmm/block_state.h"
#include "pmm/diagnostics.h"
#include <cstddef>
#include <cstdint>
#include <limits>
namespace pmm
{
namespace detail
{
template <typename AT> inline bool validate_block_index( size_t total_size, typename AT::index_type idx ) noexcept
{
    if ( idx == AT::no_block )
        return false;
    size_t byte_off = static_cast<size_t>( idx ) * AT::granule_size;
    if ( idx != 0 && byte_off / AT::granule_size != static_cast<size_t>( idx ) )
        return false;
    if ( byte_off + sizeof( pmm::Block<AT> ) > total_size )
        return false;
    return true;
}
template <typename AT>
inline bool validate_user_ptr( const uint8_t* base, size_t total_size, const void* ptr,
                               size_t min_user_offset ) noexcept
{
    if ( ptr == nullptr || base == nullptr )
        return false;
    if ( min_user_offset < sizeof( pmm::Block<AT> ) )
        return false;
    if ( total_size < min_user_offset )
        return false;
    const auto*          raw_ptr   = static_cast<const uint8_t*>( ptr );
    const std::uintptr_t raw_addr  = reinterpret_cast<std::uintptr_t>( raw_ptr );
    const std::uintptr_t base_addr = reinterpret_cast<std::uintptr_t>( base );
    if ( raw_addr < base_addr )
        return false;
    const size_t byte_off = static_cast<size_t>( raw_addr - base_addr );
    if ( byte_off >= total_size )
        return false;
    if ( byte_off < min_user_offset )
        return false;
    static constexpr size_t kBlockSize = sizeof( pmm::Block<AT> );
    size_t                  cand_off   = byte_off - kBlockSize;
    if ( cand_off % AT::granule_size != 0 )
        return false;
    return true;
}
template <typename AT> inline bool validate_link_index( size_t total_size, typename AT::index_type idx ) noexcept
{
    if ( idx == AT::no_block )
        return true;
    return validate_block_index<AT>( total_size, idx );
}
template <typename AT>
inline void validate_block_header_full( const uint8_t* base, size_t total_size, typename AT::index_type idx,
                                        VerifyResult& result ) noexcept
{
    using BlockState = pmm::BlockStateBase<AT>;
    using index_type = typename AT::index_type;
    if ( !validate_block_index<AT>( total_size, idx ) )
    {
        result.add( ViolationType::BlockStateInconsistent, DiagnosticAction::NoAction, static_cast<uint64_t>( idx ), 0,
                    0 );
        return;
    }
    const void* blk_raw = base + static_cast<size_t>( idx ) * AT::granule_size;
    BlockState::verify_state( blk_raw, idx, result );
    index_type next = BlockState::get_next_offset( blk_raw );
    if ( next != AT::no_block && !validate_block_index<AT>( total_size, next ) )
    {
        result.add( ViolationType::BlockStateInconsistent, DiagnosticAction::NoAction, static_cast<uint64_t>( idx ),
                    static_cast<uint64_t>( AT::no_block ), static_cast<uint64_t>( next ) );
    }
    index_type prev = BlockState::get_prev_offset( blk_raw );
    if ( prev != AT::no_block && !validate_block_index<AT>( total_size, prev ) )
    {
        result.add( ViolationType::BlockStateInconsistent, DiagnosticAction::NoAction, static_cast<uint64_t>( idx ),
                    static_cast<uint64_t>( AT::no_block ), static_cast<uint64_t>( prev ) );
    }
    pmm::NodeType nt = BlockState::get_node_type( blk_raw );
    if ( !pmm::is_known_node_type( static_cast<std::uint8_t>( nt ) ) )
    {
        result.add( ViolationType::BlockStateInconsistent, DiagnosticAction::NoAction, static_cast<uint64_t>( idx ), 0,
                    static_cast<uint64_t>( static_cast<std::uint8_t>( nt ) ) );
    }
    index_type w = BlockState::get_weight( blk_raw );
    if ( pmm::is_allocated( nt ) && w > 0 )
    {
        static constexpr size_t kBlkHdrBytes = sizeof( pmm::Block<AT> );
        size_t                  blk_byte_off = static_cast<size_t>( idx ) * AT::granule_size;
        size_t                  data_bytes   = static_cast<size_t>( w ) * AT::granule_size;
        if ( blk_byte_off + kBlkHdrBytes + data_bytes > total_size )
        {
            result.add( ViolationType::BlockStateInconsistent, DiagnosticAction::NoAction, static_cast<uint64_t>( idx ),
                        total_size, static_cast<uint64_t>( blk_byte_off + kBlkHdrBytes + data_bytes ) );
        }
    }
}
}
}
