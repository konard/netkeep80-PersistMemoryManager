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
    typename AT::index_type value = 0;
};
constexpr std::optional<std::size_t> checked_add( std::size_t a, std::size_t b ) noexcept
{
    return ( a > std::numeric_limits<std::size_t>::max() - b ) ? std::nullopt : std::optional<std::size_t>{ a + b };
}
constexpr std::optional<std::size_t> checked_mul( std::size_t a, std::size_t b ) noexcept
{
    if ( a == 0 || b == 0 )
        return std::size_t{ 0 };
    return ( a > std::numeric_limits<std::size_t>::max() / b ) ? std::nullopt : std::optional<std::size_t>{ a * b };
}
constexpr bool fits_range( std::size_t off, std::size_t len, std::size_t total ) noexcept
{
    auto e = checked_add( off, len );
    return e && *e <= total;
}
template <typename AT> constexpr std::optional<GranuleCount<AT>> bytes_to_granules_checked( std::size_t bytes ) noexcept
{
    using IT                 = typename AT::index_type;
    constexpr std::size_t kG = AT::granule_size;
    if ( bytes == 0 )
        return GranuleCount<AT>{ static_cast<IT>( 0 ) };
    auto p = checked_add( bytes, kG - 1 );
    if ( !p )
        return std::nullopt;
    std::size_t g = *p / kG;
    if ( g > static_cast<std::size_t>( std::numeric_limits<IT>::max() ) )
        return std::nullopt;
    return GranuleCount<AT>{ static_cast<IT>( g ) };
}
/*
### pmm-detail-arenaview
*/
template <typename AT> class ArenaView
{
  public:
    using index_type               = typename AT::index_type;
    constexpr ArenaView() noexcept = default;
    constexpr ArenaView( std::uint8_t* b, ManagerHeader<AT>* h ) noexcept : _b( b ), _h( h ) {}
    constexpr std::uint8_t*      base() const noexcept { return _b; }
    constexpr ManagerHeader<AT>* header() const noexcept { return _h; }
    std::size_t                  total_size() const noexcept { return _h ? _h->total_size : 0; }
    bool                         valid() const noexcept { return _b && _h; }
    bool                         fits( index_type i, std::size_t n ) const noexcept
    {
        return fits_range( static_cast<std::size_t>( i ) * AT::granule_size, n, total_size() );
    }
    bool valid_block( index_type i ) const noexcept
    {
        return _b && _h && i != AT::no_block && fits( i, sizeof( Block<AT> ) );
    }
    Block<AT>* block( index_type i ) const noexcept
    {
        return valid_block( i ) ? reinterpret_cast<Block<AT>*>( _b + static_cast<std::size_t>( i ) * AT::granule_size )
                                : nullptr;
    }

  private:
    std::uint8_t*      _b = nullptr;
    ManagerHeader<AT>* _h = nullptr;
};
/*
### pmm-detail-blockwalker
*/
template <typename AT, typename Fn>
bool for_each_physical_block( const std::uint8_t* base, const ManagerHeader<AT>* hdr, Fn&& fn ) noexcept
{
    using BS = pmm::BlockStateBase<AT>;
    using IT = typename AT::index_type;
    if ( !base || !hdr )
        return false;
    const std::size_t total = static_cast<std::size_t>( hdr->total_size );
    const std::size_t kBlk  = sizeof( pmm::Block<AT> );
    const std::size_t limit = total / AT::granule_size + 1;
    IT                idx = hdr->first_block_offset, prev = AT::no_block;
    std::size_t       steps = 0;
    while ( idx != AT::no_block )
    {
        if ( ++steps > limit )
            return false;
        std::size_t off = static_cast<std::size_t>( idx ) * AT::granule_size;
        if ( !fits_range( off, kBlk, total ) )
            return false;
        const void* blk = base + off;
        if ( BS::get_prev_offset( blk ) != prev )
            return false;
        if ( !fn( idx, blk ) )
            return true;
        IT next = BS::get_next_offset( blk );
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
inline std::optional<std::size_t> compute_growth( std::size_t cur, std::size_t need, std::size_t g, std::size_t num,
                                                  std::size_t den, std::size_t max_gb ) noexcept
{
    if ( g == 0 || ( g & ( g - 1 ) ) != 0 || den == 0 )
        return std::nullopt;
    auto tn = checked_mul( cur, num );
    if ( !tn )
        return std::nullopt;
    std::size_t s = *tn / den;
    if ( s < cur )
        s = cur;
    auto n = checked_add( cur, need );
    if ( !n )
        return std::nullopt;
    if ( *n > s )
        s = *n;
    auto p = checked_add( s, g - 1 );
    if ( !p )
        return std::nullopt;
    s = ( *p / g ) * g;
    if ( s <= cur )
        return std::nullopt;
    if ( max_gb != 0 )
    {
        auto cap = checked_mul( max_gb, std::size_t{ 1024 } * 1024 * 1024 );
        if ( !cap || s > *cap )
            return std::nullopt;
    }
    return s;
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
/*
#### pmm-detail-cfg-grow
*/
template <typename, size_t D, typename = void> struct cfg_grow_num
{
    static constexpr size_t value = D;
};
template <typename C, size_t D> struct cfg_grow_num<C, D, std::void_t<decltype( C::grow_numerator )>>
{
    static constexpr size_t value = C::grow_numerator;
};
template <typename, size_t D, typename = void> struct cfg_grow_den
{
    static constexpr size_t value = D;
};
template <typename C, size_t D> struct cfg_grow_den<C, D, std::void_t<decltype( C::grow_denominator )>>
{
    static constexpr size_t value = C::grow_denominator;
};
template <typename, size_t D, typename = void> struct cfg_max_gb
{
    static constexpr size_t value = D;
};
template <typename C, size_t D> struct cfg_max_gb<C, D, std::void_t<decltype( C::max_memory_gb )>>
{
    static constexpr size_t value = C::max_memory_gb;
};
}
