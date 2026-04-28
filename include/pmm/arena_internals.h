#pragma once
#include "pmm/block.h"
#include "pmm/block_state.h"
#include "pmm/types.h"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>
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
constexpr std::optional<std::size_t> round_up_checked( std::size_t v, std::size_t a ) noexcept
{
    if ( a == 0 || ( a & ( a - 1 ) ) != 0 )
        return std::nullopt;
    auto p = checked_add( v, a - 1 );
    return p.has_value() ? std::optional<std::size_t>{ ( *p / a ) * a } : std::nullopt;
}
template <typename AT>
constexpr std::optional<std::size_t> checked_granule_offset( typename AT::index_type idx ) noexcept
{
    return checked_mul( static_cast<std::size_t>( idx ), AT::granule_size );
}
template <typename AT>
constexpr std::optional<typename AT::index_type> byte_off_to_idx_checked( std::size_t byte_off ) noexcept
{
    using IndexT             = typename AT::index_type;
    constexpr std::size_t kG = AT::granule_size;
    if ( byte_off % kG != 0 )
        return std::nullopt;
    std::size_t idx = byte_off / kG;
    if ( idx > static_cast<std::size_t>( std::numeric_limits<IndexT>::max() ) || idx == AT::no_block )
        return std::nullopt;
    return static_cast<IndexT>( idx );
}
template <typename AT> constexpr std::size_t max_arena_size() noexcept
{
    using IndexT             = typename AT::index_type;
    constexpr std::size_t kG = AT::granule_size;
    constexpr std::size_t mx = static_cast<std::size_t>( std::numeric_limits<IndexT>::max() );
    if ( mx > std::numeric_limits<std::size_t>::max() / kG )
        return std::numeric_limits<std::size_t>::max();
    return mx * kG;
}
template <typename AT> constexpr std::optional<GranuleCount<AT>> bytes_to_granules_checked( std::size_t bytes ) noexcept
{
    using IndexT             = typename AT::index_type;
    constexpr std::size_t kG = AT::granule_size;
    if ( bytes == 0 )
        return GranuleCount<AT>{ static_cast<IndexT>( 0 ) };
    auto plus = checked_add( bytes, kG - 1 );
    if ( !plus.has_value() )
        return std::nullopt;
    std::size_t g = *plus / kG;
    if ( g > static_cast<std::size_t>( std::numeric_limits<IndexT>::max() ) )
        return std::nullopt;
    return GranuleCount<AT>{ static_cast<IndexT>( g ) };
}
/*
### pmm-detail-arenaview
*/
template <typename AT, typename BytePtr, typename HdrPtr> class ArenaViewBase
{
  public:
    using index_type                   = typename AT::index_type;
    constexpr ArenaViewBase() noexcept = default;
    constexpr ArenaViewBase( BytePtr b, HdrPtr h ) noexcept : _base( b ), _hdr( h ) {}
    constexpr BytePtr base() const noexcept { return _base; }
    constexpr HdrPtr  header() const noexcept { return _hdr; }
    std::size_t       total_size() const noexcept { return _hdr ? _hdr->total_size : 0; }
    bool              valid() const noexcept { return _base && _hdr; }
    bool              valid_block( index_type idx ) const noexcept
    {
        if ( !_base || !_hdr || idx == AT::no_block )
            return false;
        return fits( idx, sizeof( Block<AT> ) );
    }
    bool fits( index_type idx, std::size_t len ) const noexcept
    {
        auto off = checked_granule_offset<AT>( idx );
        return off.has_value() && fits_range( *off, len, total_size() );
    }

  protected:
    BytePtr _base = nullptr;
    HdrPtr  _hdr  = nullptr;
};
template <typename AT> class ArenaView : public ArenaViewBase<AT, std::uint8_t*, ManagerHeader<AT>*>
{
    using Base = ArenaViewBase<AT, std::uint8_t*, ManagerHeader<AT>*>;

  public:
    using index_type = typename AT::index_type;
    using Base::Base;
    Block<AT>* block( index_type idx ) const noexcept
    {
        if ( !this->valid_block( idx ) )
            return nullptr;
        auto off = checked_granule_offset<AT>( idx );
        return off.has_value() ? reinterpret_cast<Block<AT>*>( this->_base + *off ) : nullptr;
    }
};
template <typename AT> class ConstArenaView : public ArenaViewBase<AT, const std::uint8_t*, const ManagerHeader<AT>*>
{
    using Base = ArenaViewBase<AT, const std::uint8_t*, const ManagerHeader<AT>*>;

  public:
    using index_type = typename AT::index_type;
    using Base::Base;
    const Block<AT>* block( index_type idx ) const noexcept
    {
        if ( !this->valid_block( idx ) )
            return nullptr;
        auto off = checked_granule_offset<AT>( idx );
        return off.has_value() ? reinterpret_cast<const Block<AT>*>( this->_base + *off ) : nullptr;
    }
};
/*
### pmm-detail-walkcontrol
*/
enum class WalkControl
{
    Continue,
    StopOk,
    Fail,
};
namespace walker_internal
{
template <typename AT, typename BytePtr, typename HdrPtr, typename Fn>
bool walk_chain( BytePtr base, HdrPtr hdr, Fn&& fn ) noexcept
{
    using BlockState = pmm::BlockStateBase<AT>;
    using IndexT     = typename AT::index_type;
    if ( !base || !hdr )
        return false;
    const std::size_t total = static_cast<std::size_t>( hdr->total_size );
    const std::size_t kBlk  = sizeof( pmm::Block<AT> );
    const std::size_t limit = total / AT::granule_size + 1;
    IndexT            idx   = hdr->first_block_offset;
    std::size_t       steps = 0;
    while ( idx != AT::no_block )
    {
        if ( ++steps > limit )
            return false;
        auto off = checked_granule_offset<AT>( idx );
        if ( !off.has_value() || !fits_range( *off, kBlk, total ) )
            return false;
        auto* blk    = base + *off;
        using Result = std::decay_t<decltype( fn( idx, blk ) )>;
        if constexpr ( std::is_same_v<Result, WalkControl> )
        {
            WalkControl c = fn( idx, blk );
            if ( c == WalkControl::StopOk )
                return true;
            if ( c == WalkControl::Fail )
                return false;
        }
        else
        {
            if ( !fn( idx, blk ) )
                return false;
        }
        IndexT next = BlockState::get_next_offset( blk );
        if ( next != AT::no_block && next <= idx )
            return false;
        idx = next;
    }
    return true;
}
}
/*
### pmm-detail-blockwalker
*/
template <typename AT, typename Fn> bool for_each_physical_block( ConstArenaView<AT> arena, Fn&& fn ) noexcept
{
    return walker_internal::walk_chain<AT>( arena.base(), arena.header(), std::forward<Fn>( fn ) );
}
template <typename AT, typename Fn> bool for_each_physical_block( ArenaView<AT> arena, Fn&& fn ) noexcept
{
    return for_each_physical_block<AT>( ConstArenaView<AT>{ arena.base(), arena.header() }, std::forward<Fn>( fn ) );
}
template <typename AT, typename Fn> bool for_each_physical_block_mut( ArenaView<AT> arena, Fn&& fn ) noexcept
{
    return walker_internal::walk_chain<AT>( arena.base(), arena.header(), std::forward<Fn>( fn ) );
}
/*
### pmm-detail-growthpolicy
*/
inline std::optional<std::size_t> compute_growth( std::size_t current, std::size_t min_required, std::size_t gran,
                                                  std::size_t num, std::size_t den, std::size_t max_gb,
                                                  std::size_t max_arena = 0 ) noexcept
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
    auto rounded = round_up_checked( new_size, gran );
    if ( !rounded || *rounded <= current )
        return std::nullopt;
    new_size = *rounded;
    if ( max_gb != 0 )
    {
        auto cap = checked_mul( max_gb, std::size_t{ 1024 } * 1024 * 1024 );
        if ( !cap || new_size > *cap )
            return std::nullopt;
    }
    if ( max_arena != 0 && new_size > max_arena )
        return std::nullopt;
    return new_size;
}
template <typename AT>
inline std::optional<std::size_t> compute_growth_for_traits( std::size_t current, std::size_t min_required,
                                                             std::size_t num, std::size_t den,
                                                             std::size_t max_gb ) noexcept
{
    return compute_growth( current, min_required, AT::granule_size, num, den, max_gb, max_arena_size<AT>() );
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
