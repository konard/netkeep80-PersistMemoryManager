#pragma once
#include "pmm/block.h"
#include "pmm/block_state.h"
#include "pmm/types.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
namespace pmm::detail
{
/*
### pmm-detail-checkedarithmetic
*/
template <typename AT> struct GranuleCount
{
    using index_type = typename AT::index_type;
    index_type value = 0;
};
constexpr std::optional<std::size_t> checked_add( std::size_t a, std::size_t b ) noexcept
{
    if ( a > std::numeric_limits<std::size_t>::max() - b )
        return std::nullopt;
    return a + b;
}
constexpr std::optional<std::size_t> checked_mul( std::size_t a, std::size_t b ) noexcept
{
    if ( a == 0 || b == 0 )
        return std::size_t{ 0 };
    if ( a > std::numeric_limits<std::size_t>::max() / b )
        return std::nullopt;
    return a * b;
}
constexpr bool fits_range( std::size_t off, std::size_t len, std::size_t total ) noexcept
{
    auto end = checked_add( off, len );
    return end.has_value() && *end <= total;
}
template <typename AT>
constexpr std::optional<GranuleCount<AT>> bytes_to_granules_checked( std::size_t bytes ) noexcept
{
    using IndexT             = typename AT::index_type;
    constexpr std::size_t kG = AT::granule_size;
    if ( bytes == 0 )
        return GranuleCount<AT>{ static_cast<IndexT>( 0 ) };
    auto plus = checked_add( bytes, kG - 1 );
    if ( !plus.has_value() )
        return std::nullopt;
    std::size_t granules = *plus / kG;
    if ( granules > static_cast<std::size_t>( std::numeric_limits<IndexT>::max() ) )
        return std::nullopt;
    return GranuleCount<AT>{ static_cast<IndexT>( granules ) };
}
/*
### pmm-detail-arenaview
*/
template <typename AT> class ArenaView
{
  public:
    using index_type                       = typename AT::index_type;
    constexpr ArenaView() noexcept         = default;
    constexpr ArenaView( std::uint8_t* b, ManagerHeader<AT>* h ) noexcept : _base( b ), _hdr( h ) {}
    constexpr std::uint8_t*      base() const noexcept { return _base; }
    constexpr ManagerHeader<AT>* header() const noexcept { return _hdr; }
    std::size_t                  total_size() const noexcept { return _hdr ? _hdr->total_size : 0; }
    Block<AT>*                   block( index_type idx ) const noexcept
    {
        if ( !valid_block( idx ) )
            return nullptr;
        return reinterpret_cast<Block<AT>*>( _base + static_cast<std::size_t>( idx ) * AT::granule_size );
    }
    bool valid_block( index_type idx ) const noexcept
    {
        if ( !_base || !_hdr || idx == AT::no_block )
            return false;
        return fits( idx, sizeof( Block<AT> ) );
    }
    bool fits( index_type idx, std::size_t len ) const noexcept
    {
        return fits_range( static_cast<std::size_t>( idx ) * AT::granule_size, len, total_size() );
    }
    bool valid() const noexcept { return _base && _hdr; }

  private:
    std::uint8_t*      _base = nullptr;
    ManagerHeader<AT>* _hdr  = nullptr;
};
/*
### pmm-detail-blockwalker
*/
template <typename AT, typename Fn>
bool for_each_physical_block( const std::uint8_t* base, const ManagerHeader<AT>* hdr, Fn&& fn ) noexcept
{
    using BlockState = pmm::BlockStateBase<AT>;
    using IndexT     = typename AT::index_type;
    if ( !base || !hdr )
        return false;
    const std::size_t total = static_cast<std::size_t>( hdr->total_size );
    const std::size_t kBlk  = sizeof( pmm::Block<AT> );
    const std::size_t limit = total / AT::granule_size + 1;
    IndexT            idx   = hdr->first_block_offset;
    IndexT            prev  = AT::no_block;
    std::size_t       steps = 0;
    while ( idx != AT::no_block )
    {
        if ( ++steps > limit )
            return false;
        std::size_t off = static_cast<std::size_t>( idx ) * AT::granule_size;
        if ( !fits_range( off, kBlk, total ) )
            return false;
        const void* blk = base + off;
        if ( BlockState::get_prev_offset( blk ) != prev )
            return false;
        if ( !fn( idx, blk ) )
            return true;
        IndexT next = BlockState::get_next_offset( blk );
        if ( next != AT::no_block && next <= idx )
            return false;
        prev = idx;
        idx  = next;
    }
    return true;
}
/*
### pmm-detail-growthpolicy
*/
inline std::optional<std::size_t> compute_growth( std::size_t current, std::size_t min_required, std::size_t gran,
                                                  std::size_t num, std::size_t den, std::size_t max_gb ) noexcept
{
    if ( gran == 0 || ( gran & ( gran - 1 ) ) != 0 || den == 0 )
        return std::nullopt;
    auto tn = checked_mul( current, num );
    if ( !tn )
        return std::nullopt;
    std::size_t target   = *tn / den;
    std::size_t new_size = target > current ? target : current;
    auto        need     = checked_add( current, min_required );
    if ( !need )
        return std::nullopt;
    if ( *need > new_size )
        new_size = *need;
    auto plus = checked_add( new_size, gran - 1 );
    if ( !plus )
        return std::nullopt;
    new_size = ( *plus / gran ) * gran;
    if ( new_size <= current )
        return std::nullopt;
    if ( max_gb != 0 )
    {
        auto cap = checked_mul( max_gb, std::size_t{ 1024 } * 1024 * 1024 );
        if ( !cap || new_size > *cap )
            return std::nullopt;
    }
    return new_size;
}
/*
### pmm-detail-initguard
*/
struct InitGuard
{
    std::atomic<bool>& initialized;
    bool               committed = false;
    explicit InitGuard( std::atomic<bool>& a ) noexcept : initialized( a ) {}
    ~InitGuard() noexcept
    {
        if ( !committed )
            initialized.store( false, std::memory_order_release );
    }
    InitGuard( const InitGuard& )            = delete;
    InitGuard& operator=( const InitGuard& ) = delete;
    void       commit() noexcept { committed = true; }
};
}
