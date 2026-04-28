#pragma once
#include "pmm/arena_internals.h"
#include "pmm/block_state.h"
#include "pmm/diagnostics.h"
#include "pmm/free_block_tree.h"
#include "pmm/types.h"
#include "pmm/validation.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
namespace pmm
{
template <typename FT = AvlFreeTree<DefaultAddressTraits>, typename AT = DefaultAddressTraits>
/*
## pmm-allocatorpolicy
*/
class AllocatorPolicy
{
    static_assert( FreeBlockTreePolicyForTraitsConcept<FT, AT>, "" );

  public:
    using address_traits                                 = AT;
    using free_block_tree                                = FT;
    using index_type                                     = typename AT::index_type;
    using BlockT                                         = Block<AT>;
    using BlockState                                     = BlockStateBase<AT>;
    using Arena                                          = detail::ArenaView<AT>;
    using ConstArena                                     = detail::ConstArenaView<AT>;
    AllocatorPolicy()                                    = delete;
    AllocatorPolicy( const AllocatorPolicy& )            = delete;
    AllocatorPolicy& operator=( const AllocatorPolicy& ) = delete;
    static void* allocate_from_block( Arena arena, index_type blk_idx, index_type data_gran )
    {
        static constexpr index_type kBlkHdrGran = detail::kBlockHeaderGranules_t<AT>;
        if ( data_gran == 0 )
            data_gran = 1;
        if ( data_gran > std::numeric_limits<index_type>::max() - kBlkHdrGran )
            return nullptr;
        std::uint8_t*                  base = arena.base();
        detail::ManagerHeader<AT>*     hdr  = arena.header();
        FT::remove( base, hdr, blk_idx );
        FreeBlock<AT>           fb             = FreeBlock<AT>::cast_from_raw( detail::block_at<AT>( base, blk_idx ) );
        FreeBlockRemovedAVL<AT> removed        = fb.remove_from_avl();
        index_type              blk_total_gran = BlockState::get_weight( detail::block_at<AT>( base, blk_idx ) );
        index_type              needed_gran    = kBlkHdrGran + data_gran;
        index_type              min_rem_gran   = kBlkHdrGran + 1;
        bool                    can_split      = false;
        if ( needed_gran <= std::numeric_limits<index_type>::max() - min_rem_gran )
            can_split = ( blk_total_gran >= needed_gran + min_rem_gran );
        if ( can_split )
        {
            SplittingBlock<AT> splitting   = removed.begin_splitting();
            index_type         new_idx     = blk_idx + needed_gran;
            void*              new_blk_ptr = detail::block_at<AT>( base, new_idx );
            index_type         curr_next   = splitting.next_offset();
            BlockT*    old_next = ( curr_next != AT::no_block ) ? detail::block_at<AT>( base, curr_next ) : nullptr;
            index_type new_blk_total_gran = static_cast<index_type>( blk_total_gran - needed_gran );
            splitting.initialize_new_block( new_blk_ptr, new_idx, blk_idx, new_blk_total_gran );
            splitting.link_new_block( old_next, new_idx );
            if ( old_next == nullptr )
                hdr->last_block_offset = new_idx;
            hdr->block_count++;
            hdr->free_count++;
            hdr->used_size += kBlkHdrGran;
            FT::insert( base, hdr, new_idx );
            (void)splitting.finalize_split( data_gran, blk_idx );
        }
        else
        {
            (void)removed.mark_as_allocated( data_gran, blk_idx );
        }
        hdr->alloc_count++;
        hdr->free_count--;
        hdr->used_size += data_gran;
        return detail::user_ptr<AT>( detail::block_at<AT>( base, blk_idx ) );
    }
    static void coalesce( Arena arena, index_type blk_idx )
    {
        std::uint8_t*               base    = arena.base();
        detail::ManagerHeader<AT>*  hdr     = arena.header();
        FreeBlockNotInAVL<AT>       not_avl = FreeBlockNotInAVL<AT>::cast_from_raw( detail::block_at<AT>( base, blk_idx ) );
        CoalescingBlock<AT>         coalescing = not_avl.begin_coalescing();
        static constexpr index_type kBlkHdrGran = detail::kBlockHeaderGranules_t<AT>;
        index_type                  b_idx       = blk_idx;
        index_type                  curr_next   = coalescing.next_offset();
        if ( curr_next != AT::no_block )
        {
            void* nxt_raw = detail::block_at<AT>( base, curr_next );
            if ( pmm::is_free( BlockState::get_node_type( nxt_raw ) ) )
            {
                index_type nxt_idx  = curr_next;
                index_type nxt_gran = BlockState::get_weight( nxt_raw );
                index_type nxt_next = BlockState::get_next_offset( nxt_raw );
                BlockT* nxt_nxt_blk = ( nxt_next != AT::no_block ) ? detail::block_at<AT>( base, nxt_next ) : nullptr;
                FT::remove( base, hdr, nxt_idx );
                coalescing.coalesce_with_next( detail::block_at<AT>( base, nxt_idx ), nxt_nxt_blk, b_idx, nxt_gran );
                if ( nxt_nxt_blk == nullptr )
                    hdr->last_block_offset = b_idx;
                hdr->block_count--;
                hdr->free_count--;
                if ( hdr->used_size >= kBlkHdrGran )
                    hdr->used_size -= kBlkHdrGran;
            }
        }
        index_type curr_prev = coalescing.prev_offset();
        if ( curr_prev != AT::no_block )
        {
            void* prv_raw = detail::block_at<AT>( base, curr_prev );
            if ( pmm::is_free( BlockState::get_node_type( prv_raw ) ) )
            {
                index_type prv_idx   = curr_prev;
                index_type self_gran = coalescing.weight();
                index_type blk_next  = coalescing.next_offset();
                BlockT*    next_blk  = ( blk_next != AT::no_block ) ? detail::block_at<AT>( base, blk_next ) : nullptr;
                FT::remove( base, hdr, prv_idx );
                CoalescingBlock<AT> result_coalescing =
                    coalescing.coalesce_with_prev( prv_raw, next_blk, prv_idx, self_gran );
                if ( next_blk == nullptr )
                    hdr->last_block_offset = prv_idx;
                hdr->block_count--;
                hdr->free_count--;
                if ( hdr->used_size >= kBlkHdrGran )
                    hdr->used_size -= kBlkHdrGran;
                (void)result_coalescing.finalize_coalesce();
                FT::insert( base, hdr, prv_idx );
                return;
            }
        }
        (void)coalescing.finalize_coalesce();
        FT::insert( base, hdr, b_idx );
    }
    static void rebuild_free_tree( Arena arena )
    {
        std::uint8_t*              base   = arena.base();
        detail::ManagerHeader<AT>* hdr    = arena.header();
        hdr->free_tree_root               = AT::no_block;
        hdr->last_block_offset            = AT::no_block;
        index_type idx                    = hdr->first_block_offset;
        while ( idx != AT::no_block )
        {
            void* blk_ptr = detail::block_at<AT>( base, idx );
            BlockState::recover_state( blk_ptr, idx );
            if ( pmm::is_free( BlockState::get_node_type( blk_ptr ) ) )
            {
                index_type total_gran = compute_total_block_granules( hdr, idx, blk_ptr );
                BlockState::set_weight_of( blk_ptr, total_gran );
                BlockState::reset_avl_fields_of( blk_ptr );
                FT::insert( base, hdr, idx );
            }
            index_type next_idx = BlockState::get_next_offset( blk_ptr );
            if ( next_idx == AT::no_block )
                hdr->last_block_offset = idx;
            idx = next_idx;
        }
    }
    static void repair_linked_list( Arena arena )
    {
        std::uint8_t*              base = arena.base();
        detail::ManagerHeader<AT>* hdr  = arena.header();
        index_type                 idx  = hdr->first_block_offset;
        index_type                 prev = AT::no_block;
        const std::size_t          total = static_cast<std::size_t>( hdr->total_size );
        while ( idx != AT::no_block )
        {
            auto off = detail::checked_granule_offset<AT>( idx );
            if ( !off.has_value() || !detail::fits_range( *off, sizeof( BlockT ), total ) )
                break;
            void* blk_ptr = detail::block_at<AT>( base, idx );
            BlockState::repair_prev_offset( blk_ptr, prev );
            prev                   = idx;
            index_type next_offset = BlockState::get_next_offset( blk_ptr );
            idx                    = next_offset;
        }
    }
    static void recompute_counters( Arena arena )
    {
        static constexpr index_type kBlkHdrGran = detail::kBlockHeaderGranules_t<AT>;
        index_type                  block_count = 0, free_count = 0, alloc_count = 0;
        index_type                  used_gran   = 0;
        ConstArena cview{ arena.base(), arena.header() };
        (void)detail::for_each_physical_block<AT>( cview, [&]( index_type, const void* blk_ptr ) noexcept
            {
                ++block_count;
                used_gran = static_cast<index_type>( used_gran + kBlkHdrGran );
                if ( pmm::is_allocated( BlockState::get_node_type( blk_ptr ) ) )
                {
                    ++alloc_count;
                    used_gran = static_cast<index_type>( used_gran + BlockState::get_weight( blk_ptr ) );
                }
                else
                {
                    ++free_count;
                }
                return true;
            } );
        detail::ManagerHeader<AT>* hdr = arena.header();
        hdr->block_count               = block_count;
        hdr->free_count                = free_count;
        hdr->alloc_count               = alloc_count;
        hdr->used_size                 = used_gran;
    }
    static void verify_linked_list( ConstArena arena, VerifyResult& result ) noexcept
    {
        index_type expected_prev = AT::no_block;
        const bool ok = detail::for_each_physical_block<AT>(
            arena, [&]( index_type idx, const void* blk_ptr ) noexcept
            {
                index_type stored_prev = BlockState::get_prev_offset( blk_ptr );
                if ( stored_prev != expected_prev )
                {
                    result.add( ViolationType::PrevOffsetMismatch, DiagnosticAction::NoAction,
                                static_cast<uint64_t>( idx ), static_cast<uint64_t>( expected_prev ),
                                static_cast<uint64_t>( stored_prev ) );
                }
                expected_prev = idx;
                return true;
            } );
        if ( !ok )
        {
            result.add( ViolationType::HeaderCorruption, DiagnosticAction::NoAction );
        }
    }
    static void verify_counters( ConstArena arena, VerifyResult& result ) noexcept
    {
        static constexpr index_type kBlkHdrGran = detail::kBlockHeaderGranules_t<AT>;
        index_type                  block_count = 0, free_count = 0, alloc_count = 0;
        index_type                  used_gran   = 0;
        const bool ok = detail::for_each_physical_block<AT>( arena, [&]( index_type, const void* blk_ptr ) noexcept
            {
                ++block_count;
                used_gran = static_cast<index_type>( used_gran + kBlkHdrGran );
                if ( pmm::is_allocated( BlockState::get_node_type( blk_ptr ) ) )
                {
                    ++alloc_count;
                    used_gran = static_cast<index_type>( used_gran + BlockState::get_weight( blk_ptr ) );
                }
                else
                {
                    ++free_count;
                }
                return true;
            } );
        if ( !ok )
        {
            result.add( ViolationType::HeaderCorruption, DiagnosticAction::NoAction );
            return;
        }
        const detail::ManagerHeader<AT>* hdr = arena.header();
        if ( hdr->block_count != block_count || hdr->free_count != free_count || hdr->alloc_count != alloc_count ||
             hdr->used_size != used_gran )
        {
            result.add( ViolationType::CounterMismatch, DiagnosticAction::NoAction, 0,
                        static_cast<uint64_t>( block_count ), static_cast<uint64_t>( hdr->block_count ) );
        }
    }
    static void verify_block_states( ConstArena arena, VerifyResult& result ) noexcept
    {
        const std::uint8_t*              base = arena.base();
        const detail::ManagerHeader<AT>* hdr  = arena.header();
        const bool ok = detail::for_each_physical_block<AT>( arena, [&]( index_type idx, const void* blk_ptr ) noexcept
            {
                BlockState::verify_state( blk_ptr, idx, result );
                if ( pmm::is_free( BlockState::get_node_type( blk_ptr ) ) )
                {
                    const auto* blk      = reinterpret_cast<const BlockT*>( blk_ptr );
                    index_type  cached   = BlockState::get_weight( blk_ptr );
                    index_type  physical = detail::physical_block_total_granules<AT>( base, hdr, blk );
                    if ( cached != physical )
                    {
                        result.add( ViolationType::BlockStateInconsistent, DiagnosticAction::NoAction,
                                    static_cast<uint64_t>( idx ), static_cast<uint64_t>( physical ),
                                    static_cast<uint64_t>( cached ) );
                    }
                }
                return true;
            } );
        if ( !ok )
        {
            result.add( ViolationType::HeaderCorruption, DiagnosticAction::NoAction );
        }
    }
    static void verify_free_tree( ConstArena arena, VerifyResult& result ) noexcept
    {
        const std::uint8_t*              base           = arena.base();
        const detail::ManagerHeader<AT>* hdr            = arena.header();
        std::size_t                      expected_count = 0;
        const bool walk_ok = detail::for_each_physical_block<AT>( arena, [&]( index_type, const void* blk_ptr ) noexcept
            {
                if ( pmm::is_free( BlockState::get_node_type( blk_ptr ) ) )
                    ++expected_count;
                return true;
            } );
        if ( !walk_ok )
        {
            result.add( ViolationType::HeaderCorruption, DiagnosticAction::NoAction );
            return;
        }
        const bool root_present = ( hdr->free_tree_root != AT::no_block );
        if ( expected_count == 0 )
        {
            if ( root_present )
            {
                result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction, 0, 0,
                            static_cast<uint64_t>( hdr->free_tree_root ) );
            }
            return;
        }
        if ( !root_present )
        {
            result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction, 0,
                        static_cast<uint64_t>( expected_count ), static_cast<uint64_t>( hdr->free_tree_root ) );
            return;
        }
        if ( !detail::validate_block_index<AT>( hdr->total_size, hdr->free_tree_root ) )
        {
            result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
                        static_cast<uint64_t>( hdr->free_tree_root ), 1, 0 );
            return;
        }
        const void* root = detail::block_at<AT>( base, hdr->free_tree_root );
        if ( !pmm::is_free( BlockState::get_node_type( root ) ) ||
             BlockState::get_parent_offset( root ) != AT::no_block )
        {
            result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction,
                        static_cast<uint64_t>( hdr->free_tree_root ), 0,
                        static_cast<uint64_t>( BlockState::get_parent_offset( root ) ) );
        }
        std::size_t visited_count = 0;
        verify_free_tree_node( base, hdr, hdr->free_tree_root, AT::no_block, {}, false, {}, false, expected_count,
                               visited_count, result );
        (void)detail::for_each_physical_block<AT>( arena, [&]( index_type idx, const void* blk_ptr ) noexcept
            {
                if ( pmm::is_free( BlockState::get_node_type( blk_ptr ) ) &&
                     !free_tree_contains( base, hdr, hdr->free_tree_root, idx, expected_count ) )
                {
                    result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction, static_cast<uint64_t>( idx ),
                                1, 0 );
                }
                return true;
            } );
        if ( visited_count != expected_count )
        {
            result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction, 0,
                        static_cast<uint64_t>( expected_count ), static_cast<uint64_t>( visited_count ) );
        }
    }
    static index_type free_tree_block_granules( const std::uint8_t* base, const detail::ManagerHeader<AT>* hdr,
                                                index_type block_idx ) noexcept
    {
        (void)hdr;
        const void* n = detail::block_at<AT>( base, block_idx );
        return BlockState::get_weight( n );
    }
    static bool free_tree_less_key( const std::uint8_t* base, const detail::ManagerHeader<AT>* hdr, index_type a,
                                    index_type b ) noexcept
    {
        index_type a_gran = free_tree_block_granules( base, hdr, a );
        index_type b_gran = free_tree_block_granules( base, hdr, b );
        return ( a_gran < b_gran ) || ( a_gran == b_gran && a < b );
    }
    static bool free_tree_contains( const std::uint8_t* base, const detail::ManagerHeader<AT>* hdr, index_type node_idx,
                                    index_type target, std::size_t step_limit ) noexcept
    {
        while ( node_idx != AT::no_block && step_limit-- > 0 )
        {
            if ( !detail::validate_block_index<AT>( hdr->total_size, node_idx ) )
                return false;
            const void* node = detail::block_at<AT>( base, node_idx );
            if ( !pmm::is_free( BlockState::get_node_type( node ) ) )
                return false;
            if ( node_idx == target )
                return true;
            node_idx = free_tree_less_key( base, hdr, target, node_idx ) ? BlockState::get_left_offset( node )
                                                                         : BlockState::get_right_offset( node );
        }
        return false;
    }
    static std::int16_t verify_free_tree_node( const std::uint8_t* base, const detail::ManagerHeader<AT>* hdr,
                                               index_type node_idx, index_type parent, index_type lower, bool has_lower,
                                               index_type upper, bool has_upper, std::size_t expected_count,
                                               std::size_t& visited_count, VerifyResult& result ) noexcept
    {
        if ( node_idx == AT::no_block )
            return 0;
        if ( visited_count >= expected_count )
        {
            result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction, static_cast<uint64_t>( node_idx ), 1,
                        2 );
            return 0;
        }
        ++visited_count;
        if ( !detail::validate_block_index<AT>( hdr->total_size, node_idx ) )
        {
            result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction, static_cast<uint64_t>( parent ), 0,
                        static_cast<uint64_t>( node_idx ) );
            return 0;
        }
        const void* node = detail::block_at<AT>( base, node_idx );
        if ( !pmm::is_free( BlockState::get_node_type( node ) ) )
        {
            result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction, static_cast<uint64_t>( node_idx ), 0,
                        static_cast<uint64_t>( BlockState::get_weight( node ) ) );
            return 0;
        }
        if ( BlockState::get_parent_offset( node ) != parent )
        {
            result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction, static_cast<uint64_t>( node_idx ),
                        static_cast<uint64_t>( parent ),
                        static_cast<uint64_t>( BlockState::get_parent_offset( node ) ) );
        }
        if ( has_lower && !free_tree_less_key( base, hdr, lower, node_idx ) )
        {
            result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction, static_cast<uint64_t>( node_idx ),
                        static_cast<uint64_t>( lower ), static_cast<uint64_t>( node_idx ) );
        }
        if ( has_upper && !free_tree_less_key( base, hdr, node_idx, upper ) )
        {
            result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction, static_cast<uint64_t>( node_idx ),
                        static_cast<uint64_t>( node_idx ), static_cast<uint64_t>( upper ) );
        }
        index_type   left       = BlockState::get_left_offset( node );
        index_type   right      = BlockState::get_right_offset( node );
        std::int16_t left_h     = verify_free_tree_node( base, hdr, left, node_idx, lower, has_lower, node_idx, true,
                                                         expected_count, visited_count, result );
        std::int16_t right_h    = verify_free_tree_node( base, hdr, right, node_idx, node_idx, true, upper, has_upper,
                                                         expected_count, visited_count, result );
        std::int16_t expected_h = static_cast<std::int16_t>( 1 + ( left_h > right_h ? left_h : right_h ) );
        std::int16_t stored_h   = static_cast<std::int16_t>( BlockState::get_avl_height( node ) );
        if ( stored_h != expected_h || left_h - right_h > 1 || right_h - left_h > 1 )
        {
            result.add( ViolationType::FreeTreeStale, DiagnosticAction::NoAction, static_cast<uint64_t>( node_idx ),
                        static_cast<uint64_t>( expected_h ), static_cast<uint64_t>( stored_h ) );
        }
        return expected_h;
    }
    static void realloc_shrink( Arena arena, index_type blk_idx, void* blk_raw, index_type old_data_gran,
                                index_type new_data_gran ) noexcept
    {
        std::uint8_t*               base        = arena.base();
        detail::ManagerHeader<AT>*  hdr         = arena.header();
        static constexpr index_type kBlkHdrGran = detail::kBlockHeaderGranules_t<AT>;
        index_type                  remainder   = old_data_gran - new_data_gran;
        if ( remainder >= kBlkHdrGran + 1 )
        {
            index_type new_free_idx  = blk_idx + kBlkHdrGran + new_data_gran;
            void*      new_free_blk  = detail::block_at<AT>( base, new_free_idx );
            index_type new_free_gran = remainder;
            index_type old_next      = BlockState::get_next_offset( blk_raw );
            auto*      old_next_blk  = ( old_next != AT::no_block ) ? detail::block_at<AT>( base, old_next ) : nullptr;
            std::memset( new_free_blk, 0, sizeof( BlockT ) );
            BlockState::init_fields( new_free_blk, blk_idx, old_next, 1, new_free_gran, 0, NodeType::Free );
            BlockState::set_next_offset_of( blk_raw, new_free_idx );
            if ( old_next_blk != nullptr )
                BlockState::set_prev_offset_of( old_next_blk, new_free_idx );
            else
                hdr->last_block_offset = new_free_idx;
            BlockState::set_weight_of( blk_raw, new_data_gran );
            hdr->block_count++;
            hdr->free_count++;
            hdr->used_size += kBlkHdrGran;
            hdr->used_size -= ( old_data_gran - new_data_gran );
            coalesce( arena, new_free_idx );
        }
        else
        {
            BlockState::set_weight_of( blk_raw, new_data_gran );
            hdr->used_size -= ( old_data_gran - new_data_gran );
        }
    }
    static bool realloc_grow( Arena arena, index_type blk_idx, void* blk_raw, index_type old_data_gran,
                              index_type new_data_gran ) noexcept
    {
        std::uint8_t*               base        = arena.base();
        detail::ManagerHeader<AT>*  hdr         = arena.header();
        static constexpr index_type kBlkHdrGran = detail::kBlockHeaderGranules_t<AT>;
        index_type                  next_idx    = BlockState::get_next_offset( blk_raw );
        if ( next_idx == AT::no_block )
            return false;
        void* next_blk = detail::block_at<AT>( base, next_idx );
        if ( !pmm::is_free( BlockState::get_node_type( next_blk ) ) )
            return false;
        index_type next_total = BlockState::get_weight( next_blk );
        index_type available  = old_data_gran + next_total;
        if ( available < new_data_gran )
            return false;
        FT::remove( base, hdr, next_idx );
        index_type next_next = BlockState::get_next_offset( next_blk );
        BlockState::set_next_offset_of( blk_raw, next_next );
        if ( next_next != AT::no_block )
            BlockState::set_prev_offset_of( detail::block_at<AT>( base, next_next ), blk_idx );
        else
            hdr->last_block_offset = blk_idx;
        std::memset( next_blk, 0, sizeof( BlockT ) );
        hdr->block_count--;
        hdr->free_count--;
        if ( hdr->used_size >= kBlkHdrGran )
            hdr->used_size -= kBlkHdrGran;
        index_type rem = available - new_data_gran;
        if ( rem >= kBlkHdrGran + 1 )
        {
            index_type rem_idx      = blk_idx + kBlkHdrGran + new_data_gran;
            void*      rem_blk      = detail::block_at<AT>( base, rem_idx );
            index_type blk_new_next = BlockState::get_next_offset( blk_raw );
            std::memset( rem_blk, 0, sizeof( BlockT ) );
            BlockState::init_fields( rem_blk, blk_idx, blk_new_next, 1, rem, 0, NodeType::Free );
            BlockState::set_next_offset_of( blk_raw, rem_idx );
            if ( blk_new_next != AT::no_block )
                BlockState::set_prev_offset_of( detail::block_at<AT>( base, blk_new_next ), rem_idx );
            else
                hdr->last_block_offset = rem_idx;
            hdr->block_count++;
            hdr->free_count++;
            hdr->used_size += kBlkHdrGran;
            FT::insert( base, hdr, rem_idx );
        }
        BlockState::set_weight_of( blk_raw, new_data_gran );
        hdr->used_size += ( new_data_gran - old_data_gran );
        return true;
    }

  private:
    static index_type compute_total_block_granules( const detail::ManagerHeader<AT>* hdr, index_type idx,
                                                    const void* blk ) noexcept
    {
        index_type next       = BlockState::get_next_offset( blk );
        index_type total_gran = static_cast<index_type>( hdr->total_size / AT::granule_size );
        return ( next != AT::no_block ) ? static_cast<index_type>( next - idx )
                                        : static_cast<index_type>( total_gran - idx );
    }
};
using DefaultAllocatorPolicy = AllocatorPolicy<AvlFreeTree<DefaultAddressTraits>, DefaultAddressTraits>;
}
