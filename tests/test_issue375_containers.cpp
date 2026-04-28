/**
 * @file test_issue375_containers.cpp
 * @brief parray/pstring growth via reallocate_typed (issue #375).
 */

#include <catch2/catch_test_macros.hpp>

#include "pmm_single_threaded_heap.h"

#include <cstring>

namespace
{
using Mgr = pmm::PersistMemoryManager<pmm::CacheManagerConfig, 3751>;

void setup()
{
    if ( Mgr::is_initialized() )
        Mgr::destroy();
    REQUIRE( Mgr::create( 64 * 1024 ) );
}
} // namespace

TEST_CASE( "I375-P: empty parray grows to initial capacity", "[issue375][parray]" )
{
    setup();
    auto* arr = Mgr::create_typed<Mgr::parray<int>>().resolve();
    REQUIRE( arr != nullptr );
    REQUIRE( arr->capacity() == 0 );
    REQUIRE( arr->push_back( 1 ) );
    REQUIRE( arr->capacity() >= 1 );
    REQUIRE( arr->size() == 1 );
    Mgr::destroy();
}

TEST_CASE( "I375-P: repeated push_back preserves values", "[issue375][parray]" )
{
    setup();
    auto* arr = Mgr::create_typed<Mgr::parray<int>>().resolve();
    REQUIRE( arr != nullptr );
    for ( int i = 0; i < 100; ++i )
        REQUIRE( arr->push_back( i * 7 ) );
    REQUIRE( arr->size() == 100 );
    for ( int i = 0; i < 100; ++i )
    {
        auto* v = arr->at( static_cast<std::size_t>( i ) );
        REQUIRE( v != nullptr );
        REQUIRE( *v == i * 7 );
    }
    Mgr::destroy();
}

TEST_CASE( "I375-P: parray growth preserves only live elements (capacity > size)", "[issue375][parray]" )
{
    setup();
    auto* arr = Mgr::create_typed<Mgr::parray<int>>().resolve();
    REQUIRE( arr != nullptr );
    REQUIRE( arr->reserve( 16 ) );
    for ( int i = 0; i < 4; ++i )
        REQUIRE( arr->push_back( 100 + i ) );
    // Force several growths, each must preserve [0, _size) values.
    for ( int i = 4; i < 50; ++i )
        REQUIRE( arr->push_back( 100 + i ) );
    for ( int i = 0; i < 50; ++i )
    {
        auto* v = arr->at( static_cast<std::size_t>( i ) );
        REQUIRE( v != nullptr );
        REQUIRE( *v == 100 + i );
    }
    Mgr::destroy();
}

TEST_CASE( "I375-S: empty pstring allocation creates terminated empty string", "[issue375][pstring]" )
{
    setup();
    auto* s = Mgr::create_typed<Mgr::pstring>().resolve();
    REQUIRE( s != nullptr );
    REQUIRE( s->size() == 0 );
    REQUIRE( s->c_str() != nullptr );
    REQUIRE( s->c_str()[0] == '\0' );
    Mgr::destroy();
}

TEST_CASE( "I375-S: pstring append/growth preserves content and terminator", "[issue375][pstring]" )
{
    setup();
    auto* s = Mgr::create_typed<Mgr::pstring>().resolve();
    REQUIRE( s != nullptr );
    REQUIRE( s->assign( "hello" ) );
    for ( int i = 0; i < 50; ++i )
        REQUIRE( s->append( "-x" ) );
    // Length is 5 + 2*50 = 105
    REQUIRE( s->size() == 105 );
    REQUIRE( s->c_str()[s->size()] == '\0' );
    REQUIRE( std::strncmp( s->c_str(), "hello", 5 ) == 0 );
    Mgr::destroy();
}

TEST_CASE( "I375-S: pstring deallocation after growth leaves verify clean", "[issue375][pstring]" )
{
    setup();
    auto pp = Mgr::create_typed<Mgr::pstring>();
    REQUIRE( !pp.is_null() );
    auto* s = pp.resolve();
    REQUIRE( s != nullptr );
    for ( int i = 0; i < 30; ++i )
        REQUIRE( s->append( "data-" ) );
    s->free_data();
    Mgr::destroy_typed( pp );
    auto verify = Mgr::verify();
    REQUIRE( verify.entry_count == 0 );
    Mgr::destroy();
}

TEST_CASE( "I375-P: parray deallocation after growth leaves verify clean", "[issue375][parray]" )
{
    setup();
    auto pp = Mgr::create_typed<Mgr::parray<int>>();
    REQUIRE( !pp.is_null() );
    auto* arr = pp.resolve();
    REQUIRE( arr != nullptr );
    for ( int i = 0; i < 200; ++i )
        REQUIRE( arr->push_back( i ) );
    arr->free_data();
    Mgr::destroy_typed( pp );
    auto verify = Mgr::verify();
    REQUIRE( verify.entry_count == 0 );
    Mgr::destroy();
}
