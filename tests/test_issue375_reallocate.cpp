/**
 * @file test_issue375_reallocate.cpp
 * @brief reallocate_typed must reject invalid/stale pptrs through the canonical
 *        checked-block helper instead of dereferencing them via raw block
 *        arithmetic (issue #375 review-2).
 *
 * After parray/pstring switched to reallocate_typed for growth, the unchecked
 * block_idx_from_pptr/block_at path that lived at the start of reallocate_typed
 * became the main container growth path. These tests pin down the new behavior:
 *
 *  - low/below-header pptr is rejected without out-of-arena access;
 *  - stale pptr after deallocate_typed is rejected with InvalidPointer;
 *  - parray with a corrupted persistent _data_idx returns false from push_back
 *    and surfaces InvalidPointer rather than scribbling on a stranger block.
 */

#include <catch2/catch_test_macros.hpp>

#include "pmm_single_threaded_heap.h"

namespace
{
using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 3752>;

void setup_clean_image()
{
    if ( Mgr::is_initialized() )
        Mgr::destroy();
    REQUIRE( Mgr::create( 64 * 1024 ) );
}
} // namespace

TEST_CASE( "I375-R: reallocate_typed rejects low invalid pptr without unchecked block access",
           "[issue375][reallocate]" )
{
    setup_clean_image();

    Mgr::pptr<int> bad( 1 ); // offset < kBlockHdrGranules → underflow path

    Mgr::clear_error();
    auto out = Mgr::reallocate_typed<int>( bad, 1, 2 );

    REQUIRE( out.is_null() );
    REQUIRE( Mgr::last_error() == pmm::PmmError::InvalidPointer );

    Mgr::destroy();
}

TEST_CASE( "I375-R: reallocate_typed rejects stale pptr after deallocate", "[issue375][reallocate]" )
{
    setup_clean_image();

    auto p = Mgr::allocate_typed<int>( 4 );
    REQUIRE( !p.is_null() );

    Mgr::deallocate_typed( p );

    Mgr::clear_error();
    auto out = Mgr::reallocate_typed<int>( p, 4, 8 );

    REQUIRE( out.is_null() );
    REQUIRE( Mgr::last_error() == pmm::PmmError::InvalidPointer );

    Mgr::destroy();
}

TEST_CASE( "I375-P: parray growth rejects corrupted data index without raw address arithmetic",
           "[issue375][parray][reallocate]" )
{
    setup_clean_image();

    auto pp = Mgr::create_typed<Mgr::parray<int>>();
    REQUIRE( !pp.is_null() );
    auto* arr = pp.resolve();
    REQUIRE( arr != nullptr );

    REQUIRE( arr->reserve( 4 ) );
    REQUIRE( arr->push_back( 1 ) );
    REQUIRE( arr->push_back( 2 ) );

    // Force the next push to grow via reallocate_typed, then corrupt the
    // persistent index so reallocate_typed must reject it.
    arr->_size     = arr->_capacity;
    arr->_data_idx = static_cast<Mgr::index_type>( 1 ); // below header — invalid

    Mgr::clear_error();
    REQUIRE_FALSE( arr->push_back( 42 ) );
    REQUIRE( Mgr::last_error() == pmm::PmmError::InvalidPointer );

    Mgr::destroy();
}
