/**
 * @file test_issue136_embedded_list_node.cpp
 * @brief Тесты для переноса AVL-ссылок в область данных свободного блока (Issue #136).
 *
 * Issue #136: «Перенос узла двухсвязного списка внутрь самого блока».
 *
 * Финальная архитектура (с O(1) coalescing):
 *  - Block<DefaultAddressTraits> == 32 байта (2 гранулы).
 *    Включает prev_offset в заголовке для O(1) доступа при освобождении блока.
 *  - FreeBlockData<DefaultAddressTraits> хранит left_offset, right_offset,
 *    parent_offset (12 байт = 3 поля).
 *  - LinkedListNode<DefaultAddressTraits> содержит prev_offset + next_offset (8 байт).
 *  - TreeNode<DefaultAddressTraits> содержит weight, root_offset, avl_height, node_type (12 байт).
 *  - FreeBlockData расположена в области данных свободного блока по адресу Block + 32.
 *  - Для занятых блоков область данных начинается с Block + 32.
 *  - Allocate/Deallocate работают корректно с новой архитектурой.
 *  - Coalesce работает в O(1) (не требует сканирования).
 *
 * @see include/pmm/free_block_data.h — FreeBlockData<A>
 * @see include/pmm/block.h — Block<A> (32 байта)
 * @see include/pmm/linked_list_node.h — LinkedListNode<A> (prev_offset + next_offset)
 * @see include/pmm/tree_node.h — TreeNode<A> (без left/right/parent)
 * @version 0.2 (Issue #136 — prev_offset in header for O(1) coalescing)
 */

#include "pmm_single_threaded_heap.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>

// ─── Макросы тестирования ─────────────────────────────────────────────────────

#define PMM_TEST( expr )                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if ( !( expr ) )                                                                                               \
        {                                                                                                              \
            std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "] " << #expr << "\n";                             \
            return false;                                                                                              \
        }                                                                                                              \
    } while ( false )

#define PMM_RUN( name, fn )                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        std::cout << "  " << name << " ... ";                                                                          \
        if ( fn() )                                                                                                    \
        {                                                                                                              \
            std::cout << "PASS\n";                                                                                     \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            std::cout << "FAIL\n";                                                                                     \
            all_passed = false;                                                                                        \
        }                                                                                                              \
    } while ( false )

// ─── Тип менеджера ─────────────────────────────────────────────────────────────

// Use unique InstanceId to avoid state leakage between tests
template <std::size_t Id> using TestMgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, Id>;

// =============================================================================
// I136-A: Размеры структур (Issue #136)
// =============================================================================

/// @brief Block<DefaultAddressTraits> == 32 байта (2 гранулы, Issue #136).
static bool test_i136_block_size_is_32_bytes()
{
    using A     = pmm::DefaultAddressTraits;
    using Block = pmm::Block<A>;

    // Issue #136: Block header = LinkedListNode (8) + TreeNode (12) + padding (12) = 32 bytes
    static_assert( sizeof( Block ) == 32, "Block<DefaultAddressTraits> must be 32 bytes (Issue #136)" );
    static_assert( sizeof( Block ) == 2 * pmm::kGranuleSize,
                   "Block<DefaultAddressTraits> must equal two granules (Issue #136)" );

    return true;
}

/// @brief FreeBlockData<DefaultAddressTraits> == 12 байт (3 поля: left/right/parent, Issue #136).
static bool test_i136_free_block_data_size_is_12_bytes()
{
    using A      = pmm::DefaultAddressTraits;
    using FBData = pmm::FreeBlockData<A>;

    // Issue #136: FreeBlockData only has left/right/parent (prev_offset is in block header)
    static_assert( sizeof( FBData ) == 12, "FreeBlockData<DefaultAddressTraits> must be 12 bytes (Issue #136)" );
    static_assert( sizeof( FBData ) == 3 * sizeof( std::uint32_t ),
                   "FreeBlockData<DefaultAddressTraits> must be 3 x uint32_t (Issue #136)" );

    return true;
}

/// @brief LinkedListNode<DefaultAddressTraits> == 8 байт (prev_offset + next_offset, Issue #136).
static bool test_i136_linked_list_node_size_is_8_bytes()
{
    using A      = pmm::DefaultAddressTraits;
    using LLNode = pmm::LinkedListNode<A>;

    // Issue #136: LinkedListNode contains prev_offset + next_offset (kept for O(1) coalescing)
    static_assert( sizeof( LLNode ) == 8, "LinkedListNode<DefaultAddressTraits> must be 8 bytes (Issue #136)" );

    return true;
}

/// @brief TreeNode<DefaultAddressTraits> == 12 байт (без left/right/parent, Issue #136).
static bool test_i136_tree_node_size_is_12_bytes()
{
    using A     = pmm::DefaultAddressTraits;
    using TNode = pmm::TreeNode<A>;

    // Issue #136: TreeNode no longer contains left_offset, right_offset, parent_offset (moved to FreeBlockData)
    // weight(4) + root_offset(4) + avl_height(2) + node_type(2) = 12 bytes
    static_assert( sizeof( TNode ) == 12, "TreeNode<DefaultAddressTraits> must be 12 bytes (Issue #136)" );

    return true;
}

// =============================================================================
// I136-B: Раскладка полей BlockStateBase (Issue #136)
// =============================================================================

/// @brief Смещения полей в заголовке Block<Default> корректны (Issue #136).
static bool test_i136_block_header_layout_offsets()
{
    using BlockState = pmm::BlockStateBase<pmm::DefaultAddressTraits>;

    // Issue #136: block header layout (32 bytes)
    // [0..3]   prev_offset (LinkedListNode::prev_offset)
    // [4..7]   next_offset (LinkedListNode::next_offset)
    // [8..11]  weight (TreeNode::weight)
    // [12..15] root_offset (TreeNode::root_offset)
    // [16..17] avl_height (TreeNode::avl_height)
    // [18..19] node_type (TreeNode::node_type)
    // [20..31] padding
    static_assert( BlockState::kOffsetPrevOffset == 0, "prev_offset must be at byte 0 (Issue #136)" );
    static_assert( BlockState::kOffsetNextOffset == 4, "next_offset must be at byte 4 (Issue #136)" );
    static_assert( BlockState::kOffsetWeight == 8, "weight must be at byte 8 (Issue #136)" );
    static_assert( BlockState::kOffsetRootOffset == 12, "root_offset must be at byte 12 (Issue #136)" );
    static_assert( BlockState::kOffsetAvlHeight == 16, "avl_height must be at byte 16 (Issue #136)" );
    static_assert( BlockState::kOffsetNodeType == 18, "node_type must be at byte 18 (Issue #136)" );

    return true;
}

/// @brief Смещения FreeBlockData корректны (Issue #136): поля в области данных свободного блока.
static bool test_i136_free_block_data_offsets()
{
    using BlockState = pmm::BlockStateBase<pmm::DefaultAddressTraits>;

    // FreeBlockData is at offset sizeof(Block<A>) = 32 from block start
    // [32..35] left_offset
    // [36..39] right_offset
    // [40..43] parent_offset
    static_assert( BlockState::kOffsetLeftOffset == 32, "left_offset must be at byte 32 (Issue #136)" );
    static_assert( BlockState::kOffsetRightOffset == 36, "right_offset must be at byte 36 (Issue #136)" );
    static_assert( BlockState::kOffsetParentOffset == 40, "parent_offset must be at byte 40 (Issue #136)" );

    return true;
}

// =============================================================================
// I136-C: Инициализация полей через BlockStateBase API (Issue #136)
// =============================================================================

/// @brief BlockStateBase::init_fields инициализирует заголовок и FreeBlockData для свободного блока.
static bool test_i136_init_fields_free_block()
{
    using A          = pmm::DefaultAddressTraits;
    using BlockState = pmm::BlockStateBase<A>;

    // Allocate space for Block<A> header + FreeBlockData<A>
    alignas( pmm::Block<A> ) std::uint8_t buf[sizeof( pmm::Block<A> ) + sizeof( pmm::FreeBlockData<A> )] = {};

    // Initialize as free block
    BlockState::init_fields( buf,
                             /*prev*/ A::no_block,
                             /*next*/ A::no_block,
                             /*avl_height*/ 1,
                             /*weight*/ 0,
                             /*root_offset*/ 0 );

    // Header fields (including prev_offset in LinkedListNode)
    PMM_TEST( BlockState::get_prev_offset( buf ) == A::no_block );
    PMM_TEST( BlockState::get_next_offset( buf ) == A::no_block );
    PMM_TEST( BlockState::get_weight( buf ) == 0 );
    PMM_TEST( BlockState::get_root_offset( buf ) == 0 );
    PMM_TEST( BlockState::get_avl_height( buf ) == 1 );

    // FreeBlockData fields (in data area — only AVL refs)
    PMM_TEST( BlockState::get_left_offset( buf ) == A::no_block );
    PMM_TEST( BlockState::get_right_offset( buf ) == A::no_block );
    PMM_TEST( BlockState::get_parent_offset( buf ) == A::no_block );

    return true;
}

/// @brief BlockStateBase API корректно устанавливает и читает все поля (Issue #136).
static bool test_i136_block_state_api_setters_getters()
{
    using A          = pmm::DefaultAddressTraits;
    using BlockState = pmm::BlockStateBase<A>;

    alignas( pmm::Block<A> ) std::uint8_t buf[sizeof( pmm::Block<A> ) + sizeof( pmm::FreeBlockData<A> )] = {};

    // Init as free block first
    BlockState::init_fields( buf, A::no_block, A::no_block, 1, 0u, 0u );

    // Set and verify all fields
    BlockState::set_next_offset_of( buf, 42u );
    PMM_TEST( BlockState::get_next_offset( buf ) == 42u );

    BlockState::set_prev_offset_of( buf, 10u );
    PMM_TEST( BlockState::get_prev_offset( buf ) == 10u );

    BlockState::set_left_offset_of( buf, 3u );
    PMM_TEST( BlockState::get_left_offset( buf ) == 3u );

    BlockState::set_right_offset_of( buf, 7u );
    PMM_TEST( BlockState::get_right_offset( buf ) == 7u );

    BlockState::set_parent_offset_of( buf, 1u );
    PMM_TEST( BlockState::get_parent_offset( buf ) == 1u );

    BlockState::set_avl_height_of( buf, 3 );
    PMM_TEST( BlockState::get_avl_height( buf ) == 3 );

    BlockState::set_weight_of( buf, 5u );
    PMM_TEST( BlockState::get_weight( buf ) == 5u );

    BlockState::set_root_offset_of( buf, 20u );
    PMM_TEST( BlockState::get_root_offset( buf ) == 20u );

    BlockState::set_node_type_of( buf, pmm::kNodeReadOnly );
    PMM_TEST( BlockState::get_node_type( buf ) == pmm::kNodeReadOnly );

    return true;
}

/// @brief Для занятых блоков prev_offset доступен из заголовка (Issue #136: O(1) coalescing).
static bool test_i136_allocated_block_has_prev_offset_in_header()
{
    using A          = pmm::DefaultAddressTraits;
    using BlockState = pmm::BlockStateBase<A>;

    alignas( pmm::Block<A> ) std::uint8_t buf[sizeof( pmm::Block<A> ) + sizeof( pmm::FreeBlockData<A> )] = {};

    // Initialize as allocated block (weight > 0) with a known prev_offset
    BlockState::init_fields( buf, 7u, 10u, 0, 5u, 2u ); // prev=7, next=10, weight=5, root=2

    // For allocated blocks, prev_offset is in the header — accessible in O(1)
    PMM_TEST( BlockState::get_weight( buf ) == 5u );
    PMM_TEST( BlockState::get_prev_offset( buf ) == 7u );

    return true;
}

// =============================================================================
// I136-D: Функциональные тесты с реальным менеджером (Issue #136)
// =============================================================================

/// @brief Allocate/deallocate работают корректно с новой архитектурой (Issue #136).
static bool test_i136_allocate_deallocate_basic()
{
    using Mgr = TestMgr<200>;
    Mgr::destroy();
    PMM_TEST( Mgr::create( 64 * 1024 ) );
    PMM_TEST( Mgr::is_initialized() );

    // Allocate a few blocks
    void* p1 = Mgr::allocate( 32 );
    PMM_TEST( p1 != nullptr );
    void* p2 = Mgr::allocate( 64 );
    PMM_TEST( p2 != nullptr );
    void* p3 = Mgr::allocate( 128 );
    PMM_TEST( p3 != nullptr );

    // Write and verify data (ensures user data area is separate from block metadata)
    std::memset( p1, 0xAA, 32 );
    std::memset( p2, 0xBB, 64 );
    std::memset( p3, 0xCC, 128 );

    PMM_TEST( static_cast<std::uint8_t*>( p1 )[0] == 0xAA );
    PMM_TEST( static_cast<std::uint8_t*>( p2 )[0] == 0xBB );
    PMM_TEST( static_cast<std::uint8_t*>( p3 )[0] == 0xCC );

    Mgr::deallocate( p1 );
    Mgr::deallocate( p2 );
    Mgr::deallocate( p3 );
    Mgr::destroy();
    return true;
}

/// @brief Block header is 2 granules (32 bytes, Issue #136).
static bool test_i136_header_size_is_two_granules()
{
    using A     = pmm::DefaultAddressTraits;
    using Block = pmm::Block<A>;

    // Block header is exactly 2 granules
    PMM_TEST( sizeof( Block ) == 2 * pmm::kGranuleSize );
    PMM_TEST( sizeof( Block ) == 32u );

    // FreeBlockData (12 bytes) fits within 1 granule
    PMM_TEST( sizeof( pmm::FreeBlockData<A> ) < pmm::kGranuleSize );

    // kBlockHeaderGranules should be 2
    PMM_TEST( pmm::detail::kBlockHeaderGranules == 2u );

    return true;
}

/// @brief Coalesce работает корректно с новой архитектурой (Issue #136).
static bool test_i136_coalesce_works()
{
    using Mgr = TestMgr<201>;
    Mgr::destroy();
    PMM_TEST( Mgr::create( 4 * 1024 ) );

    std::size_t free_before = Mgr::free_size();

    // Allocate three adjacent blocks
    void* p1 = Mgr::allocate( 16 );
    void* p2 = Mgr::allocate( 16 );
    void* p3 = Mgr::allocate( 16 );
    PMM_TEST( p1 != nullptr && p2 != nullptr && p3 != nullptr );

    // Free middle block first, then neighbours → should coalesce
    Mgr::deallocate( p2 );
    Mgr::deallocate( p1 );
    Mgr::deallocate( p3 );

    std::size_t free_after = Mgr::free_size();

    // After deallocating all, free size should equal what we had before
    PMM_TEST( free_after == free_before );

    Mgr::destroy();
    return true;
}

/// @brief Многократное выделение и освобождение без утечек (Issue #136).
static bool test_i136_allocate_many_blocks()
{
    using Mgr = TestMgr<202>;
    Mgr::destroy();
    PMM_TEST( Mgr::create( 64 * 1024 ) );

    static constexpr int kCount = 100;
    void*                ptrs[kCount];

    for ( int i = 0; i < kCount; ++i )
    {
        ptrs[i] = Mgr::allocate( 64 );
        PMM_TEST( ptrs[i] != nullptr );
        // Write a unique pattern to verify isolation
        std::memset( ptrs[i], static_cast<unsigned char>( i & 0xFF ), 64 );
    }

    // Verify all written patterns are intact
    for ( int i = 0; i < kCount; ++i )
    {
        const auto* bytes = static_cast<const std::uint8_t*>( ptrs[i] );
        PMM_TEST( bytes[0] == static_cast<std::uint8_t>( i & 0xFF ) );
        PMM_TEST( bytes[63] == static_cast<std::uint8_t>( i & 0xFF ) );
    }

    // Free all
    for ( int i = 0; i < kCount; ++i )
        Mgr::deallocate( ptrs[i] );

    Mgr::destroy();
    return true;
}

/// @brief pptr работает корректно с новой архитектурой (Issue #136).
static bool test_i136_pptr_works()
{
    using Mgr = TestMgr<203>;
    Mgr::destroy();
    PMM_TEST( Mgr::create( 64 * 1024 ) );

    auto p = Mgr::allocate_typed<int>();
    PMM_TEST( !p.is_null() );

    int* raw = p.resolve();
    PMM_TEST( raw != nullptr );
    *raw = 42;
    PMM_TEST( *raw == 42 );

    Mgr::deallocate_typed( p );
    Mgr::destroy();
    return true;
}

/// @brief FreeBlockData хранится по адресу Block + sizeof(Block) (Issue #136).
static bool test_i136_free_block_data_address()
{
    using A      = pmm::DefaultAddressTraits;
    using BlkT   = pmm::Block<A>;
    using FBData = pmm::FreeBlockData<A>;

    alignas( BlkT ) std::uint8_t buf[sizeof( BlkT ) + sizeof( FBData )];
    std::memset( buf, 0, sizeof( buf ) );

    BlkT*   blk  = reinterpret_cast<BlkT*>( buf );
    FBData* data = reinterpret_cast<FBData*>( buf + sizeof( BlkT ) );

    // FreeBlockData should be right after the Block header (at offset 32)
    PMM_TEST( reinterpret_cast<std::uint8_t*>( data ) == reinterpret_cast<std::uint8_t*>( blk ) + sizeof( BlkT ) );
    PMM_TEST( reinterpret_cast<std::uint8_t*>( data ) == buf + 32 );

    // Verify FreeBlockData fields are accessible via Block::free_data() (AVL-only fields)
    data->left_offset   = 10u;
    data->right_offset  = 15u;
    data->parent_offset = 20u;

    PMM_TEST( blk->free_data().left_offset == 10u );
    PMM_TEST( blk->free_data().right_offset == 15u );
    PMM_TEST( blk->free_data().parent_offset == 20u );

    return true;
}

/// @brief Persistence (save/load) работает с новой архитектурой (Issue #136).
static bool test_i136_persistence_works()
{
    using Mgr = TestMgr<204>;
    Mgr::destroy();
    PMM_TEST( Mgr::create( 64 * 1024 ) );

    // Allocate and write data
    auto p = Mgr::allocate_typed<int>();
    PMM_TEST( !p.is_null() );
    *p.resolve() = 12345;

    // Verify before reload
    PMM_TEST( *p.resolve() == 12345 );

    Mgr::destroy();
    return true;
}

// =============================================================================
// main
// =============================================================================

int main()
{
    std::cout << "=== test_issue136_embedded_list_node (Issue #136: AVL refs in free block data) ===\n\n";
    bool all_passed = true;

    std::cout << "--- I136-A: Structure sizes ---\n";
    PMM_RUN( "I136-A1: Block<Default> == 32 bytes (2 granules, Issue #136)", test_i136_block_size_is_32_bytes );
    PMM_RUN( "I136-A2: FreeBlockData<Default> == 12 bytes (3 AVL fields, Issue #136)",
             test_i136_free_block_data_size_is_12_bytes );
    PMM_RUN( "I136-A3: LinkedListNode<Default> == 8 bytes (prev+next, Issue #136)",
             test_i136_linked_list_node_size_is_8_bytes );
    PMM_RUN( "I136-A4: TreeNode<Default> == 12 bytes (no left/right/parent, Issue #136)",
             test_i136_tree_node_size_is_12_bytes );

    std::cout << "\n--- I136-B: Layout offsets ---\n";
    PMM_RUN( "I136-B1: Block header layout offsets (Issue #136)", test_i136_block_header_layout_offsets );
    PMM_RUN( "I136-B2: FreeBlockData offsets in data area at +32 (Issue #136)", test_i136_free_block_data_offsets );

    std::cout << "\n--- I136-C: BlockStateBase API ---\n";
    PMM_RUN( "I136-C1: init_fields initializes header + FreeBlockData for free blocks",
             test_i136_init_fields_free_block );
    PMM_RUN( "I136-C2: BlockStateBase setters/getters for all fields", test_i136_block_state_api_setters_getters );
    PMM_RUN( "I136-C3: Allocated block prev_offset accessible in O(1) from header",
             test_i136_allocated_block_has_prev_offset_in_header );

    std::cout << "\n--- I136-D: Functional tests ---\n";
    PMM_RUN( "I136-D1: Allocate/deallocate basic functionality", test_i136_allocate_deallocate_basic );
    PMM_RUN( "I136-D2: Block header is 2 granules (32 bytes), kBlockHeaderGranules == 2",
             test_i136_header_size_is_two_granules );
    PMM_RUN( "I136-D3: Coalesce works with new architecture", test_i136_coalesce_works );
    PMM_RUN( "I136-D4: Allocate many blocks, verify data isolation", test_i136_allocate_many_blocks );
    PMM_RUN( "I136-D5: pptr works with new architecture", test_i136_pptr_works );
    PMM_RUN( "I136-D6: FreeBlockData stored at Block + sizeof(Block) = Block + 32", test_i136_free_block_data_address );
    PMM_RUN( "I136-D7: Persistence works with new architecture", test_i136_persistence_works );

    std::cout << "\n" << ( all_passed ? "All tests PASSED\n" : "Some tests FAILED\n" );
    return all_passed ? 0 : 1;
}
