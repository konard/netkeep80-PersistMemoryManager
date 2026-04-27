#pragma once
#include "pmm/address_traits.h"
#include "pmm/block.h"
#include "pmm/block_field.h"
#include "pmm/diagnostics.h"
#include "pmm/tree_node.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>
namespace pmm
{
template <typename AT> class FreeBlock;
template <typename AT> class AllocatedBlock;
template <typename AT> class FreeBlockRemovedAVL;
template <typename AT> class FreeBlockNotInAVL;
template <typename AT> class SplittingBlock;
template <typename AT> class CoalescingBlock;
/*
## pmm-blockstatebase
*/
template <typename AT> class BlockStateBase : private Block<AT>
{
  private:
    using TNode = TreeNode<AT>;

  public:
    using address_traits                                              = AT;
    using index_type                                                  = typename AT::index_type;
    using BaseBlock                                                   = Block<AT>;
    template <typename FieldTag> using field_value_type               = detail::block_field_value_t<AT, FieldTag>;
    template <typename FieldTag> static constexpr size_t field_offset = detail::block_field_offset_v<AT, FieldTag>;
    template <typename FieldTag> static field_value_type<FieldTag> get_field_of( const void* raw_blk ) noexcept
    {
        return detail::read_block_field<AT, FieldTag>( raw_blk );
    }
    template <typename FieldTag> static void set_field_of( void* raw_blk, field_value_type<FieldTag> value ) noexcept
    {
        detail::write_block_field<AT, FieldTag>( raw_blk, value );
    }
    static bool is_free_raw( const void* raw_blk ) noexcept
    {
        return get_weight( raw_blk ) == 0 && get_root_offset( raw_blk ) == 0;
    }
    static bool is_allocated_raw( const void* raw_blk, index_type own_idx ) noexcept
    {
        return get_weight( raw_blk ) > 0 && get_root_offset( raw_blk ) == own_idx;
    }
    static constexpr size_t kOffsetPrevOffset   = field_offset<detail::BlockPrevOffsetField>;
    static constexpr size_t kOffsetNextOffset   = field_offset<detail::BlockNextOffsetField>;
    static constexpr size_t kOffsetWeight       = field_offset<detail::BlockWeightField>;
    static constexpr size_t kOffsetLeftOffset   = field_offset<detail::BlockLeftOffsetField>;
    static constexpr size_t kOffsetRightOffset  = field_offset<detail::BlockRightOffsetField>;
    static constexpr size_t kOffsetParentOffset = field_offset<detail::BlockParentOffsetField>;
    static constexpr size_t kOffsetRootOffset   = field_offset<detail::BlockRootOffsetField>;
    static constexpr size_t kOffsetAvlHeight    = field_offset<detail::BlockAvlHeightField>;
    static constexpr size_t kOffsetNodeType     = field_offset<detail::BlockNodeTypeField>;
    static_assert( detail::block_tree_slot_size_v<AT> == sizeof( TNode ), "" );
    static_assert( detail::block_layout_size_v<AT> == sizeof( BaseBlock ), "" );
    BlockStateBase() = delete;
    index_type   weight() const noexcept { return get_weight( this ); }
    index_type   prev_offset() const noexcept { return get_prev_offset( this ); }
    index_type   next_offset() const noexcept { return get_next_offset( this ); }
    index_type   left_offset() const noexcept { return get_left_offset( this ); }
    index_type   right_offset() const noexcept { return get_right_offset( this ); }
    index_type   parent_offset() const noexcept { return get_parent_offset( this ); }
    std::int16_t avl_height() const noexcept { return get_avl_height( this ); }
    index_type   root_offset() const noexcept { return get_root_offset( this ); }
    uint16_t     node_type() const noexcept { return get_node_type( this ); }
    /*
    ### pmm-blockstatebase-is_free
    */
    bool is_free() const noexcept { return is_free_raw( this ); }
    bool is_allocated( index_type own_idx ) const noexcept { return is_allocated_raw( this, own_idx ); }
    bool is_permanently_locked() const noexcept { return node_type() == pmm::kNodeReadOnly; }
    /*
    ### pmm-blockstatebase-recover_state
    */
    static void recover_state( void* raw_blk, index_type own_idx ) noexcept
    {
        const index_type weight_val = get_weight( raw_blk );
        const index_type root_val   = get_root_offset( raw_blk );
        if ( weight_val > 0 && root_val != own_idx )
            set_root_offset_of( raw_blk, own_idx );
        if ( weight_val == 0 && root_val != 0 )
            set_root_offset_of( raw_blk, 0 );
    }
    /*
    ### pmm-blockstatebase-verify_state
    */
    static void verify_state( const void* raw_blk, index_type own_idx, VerifyResult& result ) noexcept
    {
        const index_type weight_val = get_weight( raw_blk );
        const index_type root_val   = get_root_offset( raw_blk );
        if ( weight_val > 0 && root_val != own_idx )
        {
            result.add( ViolationType::BlockStateInconsistent, DiagnosticAction::NoAction,
                        static_cast<uint64_t>( own_idx ), static_cast<uint64_t>( own_idx ),
                        static_cast<uint64_t>( root_val ) );
        }
        if ( weight_val == 0 && root_val != 0 )
        {
            result.add( ViolationType::BlockStateInconsistent, DiagnosticAction::NoAction,
                        static_cast<uint64_t>( own_idx ), 0, static_cast<uint64_t>( root_val ) );
        }
    }
    static void reset_avl_fields_of( void* raw_blk ) noexcept
    {
        set_left_offset_of( raw_blk, AT::no_block );
        set_right_offset_of( raw_blk, AT::no_block );
        set_parent_offset_of( raw_blk, AT::no_block );
        set_avl_height_of( raw_blk, 0 );
    }
    static void repair_prev_offset( void* raw_blk, index_type prev_idx ) noexcept
    {
        set_prev_offset_of( raw_blk, prev_idx );
    }
    static index_type get_prev_offset( const void* raw_blk ) noexcept
    {
        return get_field_of<detail::BlockPrevOffsetField>( raw_blk );
    }
    static index_type get_next_offset( const void* raw_blk ) noexcept
    {
        return get_field_of<detail::BlockNextOffsetField>( raw_blk );
    }
    static index_type get_weight( const void* raw_blk ) noexcept
    {
        return get_field_of<detail::BlockWeightField>( raw_blk );
    }
    static void init_fields( void* raw_blk, index_type prev_idx, index_type next_idx, std::int16_t avl_height_val,
                             index_type weight_val, index_type root_offset_val ) noexcept
    {
        set_prev_offset_of( raw_blk, prev_idx );
        set_next_offset_of( raw_blk, next_idx );
        set_left_offset_of( raw_blk, AT::no_block );
        set_right_offset_of( raw_blk, AT::no_block );
        set_parent_offset_of( raw_blk, AT::no_block );
        set_avl_height_of( raw_blk, avl_height_val );
        set_weight_of( raw_blk, weight_val );
        set_root_offset_of( raw_blk, root_offset_val );
    }
    static void set_next_offset_of( void* raw_blk, index_type next_idx ) noexcept
    {
        set_field_of<detail::BlockNextOffsetField>( raw_blk, next_idx );
    }
    static index_type get_left_offset( const void* b ) noexcept
    {
        return get_field_of<detail::BlockLeftOffsetField>( b );
    }
    static index_type get_right_offset( const void* b ) noexcept
    {
        return get_field_of<detail::BlockRightOffsetField>( b );
    }
    static index_type get_parent_offset( const void* b ) noexcept
    {
        return get_field_of<detail::BlockParentOffsetField>( b );
    }
    static index_type get_root_offset( const void* b ) noexcept
    {
        return get_field_of<detail::BlockRootOffsetField>( b );
    }
    static void set_left_offset_of( void* b, index_type v ) noexcept
    {
        set_field_of<detail::BlockLeftOffsetField>( b, v );
    }
    static void set_right_offset_of( void* b, index_type v ) noexcept
    {
        set_field_of<detail::BlockRightOffsetField>( b, v );
    }
    static void set_parent_offset_of( void* b, index_type v ) noexcept
    {
        set_field_of<detail::BlockParentOffsetField>( b, v );
    }
    static void set_prev_offset_of( void* b, index_type v ) noexcept
    {
        set_field_of<detail::BlockPrevOffsetField>( b, v );
    }
    static void set_weight_of( void* b, index_type v ) noexcept { set_field_of<detail::BlockWeightField>( b, v ); }
    static void set_root_offset_of( void* b, index_type v ) noexcept
    {
        set_field_of<detail::BlockRootOffsetField>( b, v );
    }
    static std::int16_t get_avl_height( const void* raw_blk ) noexcept
    {
        return get_field_of<detail::BlockAvlHeightField>( raw_blk );
    }
    static void set_avl_height_of( void* raw_blk, std::int16_t v ) noexcept
    {
        set_field_of<detail::BlockAvlHeightField>( raw_blk, v );
    }
    static uint16_t get_node_type( const void* raw_blk ) noexcept
    {
        return get_field_of<detail::BlockNodeTypeField>( raw_blk );
    }
    static void set_node_type_of( void* raw_blk, uint16_t v ) noexcept
    {
        set_field_of<detail::BlockNodeTypeField>( raw_blk, v );
    }

  protected:
    template <typename StateT> static StateT* state_from_raw( void* raw ) noexcept
    {
        return reinterpret_cast<StateT*>( raw );
    }
    template <typename StateT> static const StateT* state_from_raw( const void* raw ) noexcept
    {
        return reinterpret_cast<const StateT*>( raw );
    }
    template <typename StateT> StateT* state_as() noexcept { return reinterpret_cast<StateT*>( this ); }
    void                               set_weight( index_type v ) noexcept { set_weight_of( this, v ); }
    void                               set_prev_offset( index_type v ) noexcept { set_prev_offset_of( this, v ); }
    void                               set_next_offset( index_type v ) noexcept { set_next_offset_of( this, v ); }
    void                               set_left_offset( index_type v ) noexcept { set_left_offset_of( this, v ); }
    void                               set_right_offset( index_type v ) noexcept { set_right_offset_of( this, v ); }
    void                               set_parent_offset( index_type v ) noexcept { set_parent_offset_of( this, v ); }
    void                               set_avl_height( std::int16_t v ) noexcept { set_avl_height_of( this, v ); }
    void                               set_root_offset( index_type v ) noexcept { set_root_offset_of( this, v ); }
    void                               set_node_type( uint16_t v ) noexcept { set_node_type_of( this, v ); }
    void                               reset_avl_fields() noexcept
    {
        set_left_offset( AT::no_block );
        set_right_offset( AT::no_block );
        set_parent_offset( AT::no_block );
        set_avl_height( 0 );
    }
};
static_assert( sizeof( BlockStateBase<DefaultAddressTraits> ) == sizeof( Block<DefaultAddressTraits> ), "" );
static_assert( sizeof( BlockStateBase<DefaultAddressTraits> ) == 32, "" );
/*
## pmm-freeblock
*/
template <typename AT> class FreeBlock : public BlockStateBase<AT>
{
  public:
    using Base       = BlockStateBase<AT>;
    using index_type = typename AT::index_type;
    /*
    ### pmm-freeblock-cast_from_raw
    */
    static FreeBlock* cast_from_raw( void* raw ) noexcept
    {
        if ( raw == nullptr )
            return nullptr;
        if ( !Base::is_free_raw( raw ) )
        {
            assert( false && "cast_from_raw<FreeBlock>: block is not in FreeBlock state" );
            return nullptr;
        }
        return Base::template state_from_raw<FreeBlock<AT>>( raw );
    }
    static const FreeBlock* cast_from_raw( const void* raw ) noexcept
    {
        if ( raw == nullptr )
            return nullptr;
        if ( !Base::is_free_raw( raw ) )
        {
            assert( false && "cast_from_raw<FreeBlock>: block is not in FreeBlock state" );
            return nullptr;
        }
        return Base::template state_from_raw<FreeBlock<AT>>( raw );
    }
    /*
    ### pmm-freeblock-verify_invariants
    */
    bool                     verify_invariants() const noexcept { return Base::is_free(); }
    FreeBlockRemovedAVL<AT>* remove_from_avl() noexcept { return this->template state_as<FreeBlockRemovedAVL<AT>>(); }
};
/*
## pmm-freeblockremovedavl
*/
template <typename AT> class FreeBlockRemovedAVL : public BlockStateBase<AT>
{
  public:
    using Base       = BlockStateBase<AT>;
    using index_type = typename AT::index_type;
    static FreeBlockRemovedAVL* cast_from_raw( void* raw ) noexcept
    {
        return Base::template state_from_raw<FreeBlockRemovedAVL<AT>>( raw );
    }
    AllocatedBlock<AT>* mark_as_allocated( index_type data_granules, index_type own_idx ) noexcept
    {
        Base::set_weight( data_granules );
        Base::set_root_offset( own_idx );
        Base::reset_avl_fields();
        return this->template state_as<AllocatedBlock<AT>>();
    }
    SplittingBlock<AT>* begin_splitting() noexcept { return this->template state_as<SplittingBlock<AT>>(); }
    FreeBlock<AT>*      insert_to_avl() noexcept { return this->template state_as<FreeBlock<AT>>(); }
};
/*
## pmm-splittingblock
*/
template <typename AT> class SplittingBlock : public BlockStateBase<AT>
{
  public:
    using Base       = BlockStateBase<AT>;
    using index_type = typename AT::index_type;
    static SplittingBlock* cast_from_raw( void* raw ) noexcept
    {
        return Base::template state_from_raw<SplittingBlock<AT>>( raw );
    }
    void initialize_new_block( void* new_blk_ptr, [[maybe_unused]] index_type new_idx, index_type own_idx ) noexcept
    {
        std::memset( new_blk_ptr, 0, sizeof( Block<AT> ) );
        Base::init_fields( new_blk_ptr, own_idx, this->next_offset(), 1, 0, 0 );
    }
    void link_new_block( void* old_next_blk, index_type new_idx ) noexcept
    {
        if ( old_next_blk != nullptr )
        {
            Base::set_prev_offset_of( old_next_blk, new_idx );
        }
        Base::set_next_offset( new_idx );
    }
    AllocatedBlock<AT>* finalize_split( index_type data_granules, index_type own_idx ) noexcept
    {
        Base::set_weight( data_granules );
        Base::set_root_offset( own_idx );
        Base::reset_avl_fields();
        return this->template state_as<AllocatedBlock<AT>>();
    }
};
/*
## pmm-allocatedblock
*/
template <typename AT> class AllocatedBlock : public BlockStateBase<AT>
{
  public:
    using Base       = BlockStateBase<AT>;
    using index_type = typename AT::index_type;
    /*
    ### pmm-allocatedblock-cast_from_raw
    */
    static AllocatedBlock* cast_from_raw( void* raw ) noexcept
    {
        if ( raw == nullptr )
            return nullptr;
        if ( Base::get_weight( raw ) == 0 )
        {
            assert( false && "cast_from_raw<AllocatedBlock>: block is not allocated (weight==0)" );
            return nullptr;
        }
        return Base::template state_from_raw<AllocatedBlock<AT>>( raw );
    }
    static const AllocatedBlock* cast_from_raw( const void* raw ) noexcept
    {
        if ( raw == nullptr )
            return nullptr;
        if ( Base::get_weight( raw ) == 0 )
        {
            assert( false && "cast_from_raw<AllocatedBlock>: block is not allocated (weight==0)" );
            return nullptr;
        }
        return Base::template state_from_raw<AllocatedBlock<AT>>( raw );
    }
    /*
    ### pmm-allocatedblock-verify_invariants
    */
    bool        verify_invariants( index_type own_idx ) const noexcept { return Base::is_allocated( own_idx ); }
    void*       user_ptr() noexcept { return reinterpret_cast<uint8_t*>( this ) + sizeof( Block<AT> ); }
    const void* user_ptr() const noexcept { return reinterpret_cast<const uint8_t*>( this ) + sizeof( Block<AT> ); }
    FreeBlockNotInAVL<AT>* mark_as_free() noexcept
    {
        Base::set_weight( 0 );
        Base::set_root_offset( 0 );
        return this->template state_as<FreeBlockNotInAVL<AT>>();
    }
};
/*
## pmm-freeblocknotinavl
*/
template <typename AT> class FreeBlockNotInAVL : public BlockStateBase<AT>
{
  public:
    using Base       = BlockStateBase<AT>;
    using index_type = typename AT::index_type;
    static FreeBlockNotInAVL* cast_from_raw( void* raw ) noexcept
    {
        return Base::template state_from_raw<FreeBlockNotInAVL<AT>>( raw );
    }
    CoalescingBlock<AT>* begin_coalescing() noexcept { return this->template state_as<CoalescingBlock<AT>>(); }
    FreeBlock<AT>*       insert_to_avl() noexcept
    {
        Base::set_avl_height( 1 );
        return this->template state_as<FreeBlock<AT>>();
    }
};
/*
## pmm-coalescingblock
*/
template <typename AT> class CoalescingBlock : public BlockStateBase<AT>
{
  public:
    using Base       = BlockStateBase<AT>;
    using index_type = typename AT::index_type;
    static CoalescingBlock* cast_from_raw( void* raw ) noexcept
    {
        return Base::template state_from_raw<CoalescingBlock<AT>>( raw );
    }
    void coalesce_with_next( void* next_blk, void* next_next_blk, index_type own_idx ) noexcept
    {
        Base::set_next_offset( Base::get_next_offset( next_blk ) );
        if ( next_next_blk != nullptr )
        {
            Base::set_prev_offset_of( next_next_blk, own_idx );
        }
        std::memset( next_blk, 0, sizeof( Block<AT> ) );
    }
    CoalescingBlock<AT>* coalesce_with_prev( void* prev_blk, void* next_blk, index_type prev_idx ) noexcept
    {
        Base::set_next_offset_of( prev_blk, Base::next_offset() );
        if ( next_blk != nullptr )
        {
            Base::set_prev_offset_of( next_blk, prev_idx );
        }
        std::memset( this, 0, sizeof( Block<AT> ) );
        return Base::template state_from_raw<CoalescingBlock<AT>>( prev_blk );
    }
    FreeBlock<AT>* finalize_coalesce() noexcept
    {
        Base::set_avl_height( 1 );
        return this->template state_as<FreeBlock<AT>>();
    }
};
template <typename AT> int detect_block_state( const void* raw_blk, typename AT::index_type own_idx ) noexcept
{
    using BlockState = BlockStateBase<AT>;
    if ( BlockState::is_free_raw( raw_blk ) )
        return 0;
    if ( BlockState::is_allocated_raw( raw_blk, own_idx ) )
        return 1;
    return -1;
}
template <typename AT> inline void recover_block_state( void* raw_blk, typename AT::index_type own_idx ) noexcept
{
    BlockStateBase<AT>::recover_state( raw_blk, own_idx );
}
template <typename AT>
inline void verify_block_state( const void* raw_blk, typename AT::index_type own_idx, VerifyResult& result ) noexcept
{
    BlockStateBase<AT>::verify_state( raw_blk, own_idx, result );
}
}
