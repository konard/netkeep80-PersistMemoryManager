#pragma once

#include "pmm/avl_tree_mixin.h"
#include "pmm/block.h"
#include "pmm/block_state.h"
#include "pmm/types.h"

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace pmm
{

template <typename Policy, typename AddressTraitsT>
concept FreeBlockTreePolicyForTraitsConcept = requires( std::uint8_t* base, detail::ManagerHeader<AddressTraitsT>* hdr,
                                                        typename AddressTraitsT::index_type idx ) {
    { Policy::insert( base, hdr, idx ) };
    { Policy::remove( base, hdr, idx ) };
    { Policy::find_best_fit( base, hdr, idx ) } -> std::convertible_to<typename AddressTraitsT::index_type>;
};

/*
## pmm::AvlFreeTree
*/
template <typename AddressTraitsT = DefaultAddressTraits> struct AvlFreeTree
{

    using address_traits = AddressTraitsT;

    using index_type     = typename AddressTraitsT::index_type;

    using BlockT         = Block<AddressTraitsT>;

    using BlockState     = BlockStateBase<AddressTraitsT>;

    using BPPtr          = detail::BlockPPtr<AddressTraitsT>;

    static constexpr const char* kForestDomainName = "system/free_tree";

/*
### pmm::AvlFreeTree::AvlFreeTree
*/
    AvlFreeTree()                                = delete;
    AvlFreeTree( const AvlFreeTree& )            = delete;

    AvlFreeTree& operator=( const AvlFreeTree& ) = delete;

/*
### pmm::AvlFreeTree::insert
*/
    static void insert( std::uint8_t* base, detail::ManagerHeader<AddressTraitsT>* hdr, index_type blk_idx )
    {

        void* blk = detail::block_at<AddressTraitsT>( base, blk_idx );

        BlockState::set_left_offset_of( blk, AddressTraitsT::no_block );

        BlockState::set_right_offset_of( blk, AddressTraitsT::no_block );

        BlockState::set_parent_offset_of( blk, AddressTraitsT::no_block );

        BlockState::set_avl_height_of( blk, 1 );
        if ( hdr->free_tree_root == AddressTraitsT::no_block )
        {
            hdr->free_tree_root = blk_idx;
            return;
        }

        index_type total_gran = detail::byte_off_to_idx_t<AddressTraitsT>( hdr->total_size );

        index_type blk_next   = BlockState::get_next_offset( blk );
        index_type blk_gran =
            ( blk_next != AddressTraitsT::no_block ) ? ( blk_next - blk_idx ) : ( total_gran - blk_idx );

        index_type cur = hdr->free_tree_root, parent = AddressTraitsT::no_block;

        bool       go_left = false;
        while ( cur != AddressTraitsT::no_block )
        {
            parent             = cur;
            const void* n      = detail::block_at<AddressTraitsT>( base, cur );
            index_type  n_next = BlockState::get_next_offset( n );
            index_type  n_gran = ( n_next != AddressTraitsT::no_block ) ? ( n_next - cur ) : ( total_gran - cur );

            bool smaller = ( blk_gran < n_gran ) || ( blk_gran == n_gran && blk_idx < cur );
            go_left      = smaller;
            cur          = smaller ? BlockState::get_left_offset( n ) : BlockState::get_right_offset( n );
        }
        BlockState::set_parent_offset_of( blk, parent );
        if ( go_left )
            BlockState::set_left_offset_of( detail::block_at<AddressTraitsT>( base, parent ), blk_idx );
        else
            BlockState::set_right_offset_of( detail::block_at<AddressTraitsT>( base, parent ), blk_idx );

        detail::avl_rebalance_up( BPPtr( base, parent ), hdr->free_tree_root );
    }

/*
### pmm::AvlFreeTree::remove
*/
    static void remove( std::uint8_t* base, detail::ManagerHeader<AddressTraitsT>* hdr, index_type blk_idx )
    {
        void*      blk    = detail::block_at<AddressTraitsT>( base, blk_idx );

        index_type parent = BlockState::get_parent_offset( blk );

        index_type left   = BlockState::get_left_offset( blk );

        index_type right  = BlockState::get_right_offset( blk );

        index_type rebal  = AddressTraitsT::no_block;

        if ( left == AddressTraitsT::no_block && right == AddressTraitsT::no_block )
        {
            set_child( base, hdr, parent, blk_idx, AddressTraitsT::no_block );
            rebal = parent;
        }
        else if ( left == AddressTraitsT::no_block || right == AddressTraitsT::no_block )
        {
            index_type child = ( left != AddressTraitsT::no_block ) ? left : right;
            BlockState::set_parent_offset_of( detail::block_at<AddressTraitsT>( base, child ), parent );
            set_child( base, hdr, parent, blk_idx, child );
            rebal = parent;
        }
        else
        {

            BPPtr      succ        = detail::avl_min_node( BPPtr( base, right ) );
            index_type succ_idx    = succ.offset();
            void*      succ_raw    = detail::block_at<AddressTraitsT>( base, succ_idx );
            index_type succ_parent = BlockState::get_parent_offset( succ_raw );
            index_type succ_right  = BlockState::get_right_offset( succ_raw );

            if ( succ_parent != blk_idx )
            {
                set_child( base, hdr, succ_parent, succ_idx, succ_right );
                if ( succ_right != AddressTraitsT::no_block )
                    BlockState::set_parent_offset_of( detail::block_at<AddressTraitsT>( base, succ_right ),
                                                      succ_parent );
                BlockState::set_right_offset_of( succ_raw, right );
                BlockState::set_parent_offset_of( detail::block_at<AddressTraitsT>( base, right ), succ_idx );
                rebal = succ_parent;
            }
            else
            {
                rebal = succ_idx;
            }
            BlockState::set_left_offset_of( succ_raw, left );
            BlockState::set_parent_offset_of( detail::block_at<AddressTraitsT>( base, left ), succ_idx );
            BlockState::set_parent_offset_of( succ_raw, parent );
            set_child( base, hdr, parent, blk_idx, succ_idx );

            detail::avl_update_height( BPPtr( base, succ_idx ) );
        }
        BlockState::set_left_offset_of( blk, AddressTraitsT::no_block );
        BlockState::set_right_offset_of( blk, AddressTraitsT::no_block );
        BlockState::set_parent_offset_of( blk, AddressTraitsT::no_block );
        BlockState::set_avl_height_of( blk, 0 );

        detail::avl_rebalance_up( BPPtr( base, rebal ), hdr->free_tree_root );
    }

/*
### pmm::AvlFreeTree::find_best_fit
*/
    static index_type find_best_fit( std::uint8_t* base, detail::ManagerHeader<AddressTraitsT>* hdr,
                                     index_type needed_granules )
    {

        index_type total_gran = detail::byte_off_to_idx_t<AddressTraitsT>( hdr->total_size );
        index_type cur = hdr->free_tree_root, result = AddressTraitsT::no_block;
        while ( cur != AddressTraitsT::no_block )
        {
            const void* node      = detail::block_at<AddressTraitsT>( base, cur );
            index_type  node_next = BlockState::get_next_offset( node );

            index_type cur_gran =
                ( node_next != AddressTraitsT::no_block ) ? ( node_next - cur ) : ( total_gran - cur );
            if ( cur_gran >= needed_granules )
            {
                result = cur;
                cur    = BlockState::get_left_offset( node );
            }
            else
            {
                cur = BlockState::get_right_offset( node );
            }
        }
        return result;
    }

  private:

/*
### pmm::AvlFreeTree::set_child
*/
    static void set_child( std::uint8_t* base, detail::ManagerHeader<AddressTraitsT>* hdr, index_type parent,
                           index_type old_child, index_type new_child )
    {
        if ( parent == AddressTraitsT::no_block )
        {
            hdr->free_tree_root = new_child;
            return;
        }
        void* p = detail::block_at<AddressTraitsT>( base, parent );
        if ( BlockState::get_left_offset( p ) == old_child )
            BlockState::set_left_offset_of( p, new_child );
        else
            BlockState::set_right_offset_of( p, new_child );
    }
};

static_assert( FreeBlockTreePolicyForTraitsConcept<AvlFreeTree<DefaultAddressTraits>, DefaultAddressTraits>,
               "AvlFreeTree<DefaultAddressTraits> must satisfy FreeBlockTreePolicyForTraitsConcept" );

}
