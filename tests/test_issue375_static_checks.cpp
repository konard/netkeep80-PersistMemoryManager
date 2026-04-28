/**
 * @file test_issue375_static_checks.cpp
 * @brief Static (string) checks proving sentinel/legacy patterns are gone.
 *
 * These tests scan the production headers directly. They guard against
 * regressions of issue #375 acceptance criteria:
 *   - no `thread_local BlockHeader` sentinel in production code;
 *   - no `ManagerT::tree_node(pptr)` ref-returning overload;
 *   - no manual `allocate(...)` capacity-growth path inside parray/pstring.
 */

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef PMM_SOURCE_DIR
#error "PMM_SOURCE_DIR must be defined to run static checks"
#endif

namespace
{
std::string slurp( const std::filesystem::path& path )
{
    std::ifstream     in( path, std::ios::binary );
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::vector<std::filesystem::path> production_headers()
{
    std::vector<std::filesystem::path> paths;
    const std::filesystem::path        root = std::filesystem::path( PMM_SOURCE_DIR ) / "include" / "pmm";
    for ( const auto& entry : std::filesystem::recursive_directory_iterator( root ) )
    {
        if ( !entry.is_regular_file() )
            continue;
        const auto& p = entry.path();
        if ( p.extension() == ".h" || p.extension() == ".inc" )
            paths.push_back( p );
    }
    return paths;
}
} // namespace

TEST_CASE( "I375-X: no thread_local BlockHeader sentinel in production code", "[issue375][static]" )
{
    for ( const auto& path : production_headers() )
    {
        const std::string contents = slurp( path );
        INFO( path.string() );
        REQUIRE( contents.find( "thread_local BlockHeader" ) == std::string::npos );
    }
}

TEST_CASE( "I375-X: no ref-returning ManagerT::tree_node(pptr) declaration in production code", "[issue375][static]" )
{
    for ( const auto& path : production_headers() )
    {
        const std::string contents = slurp( path );
        INFO( path.string() );
        // The old API was `BlockHeader<address_traits>& tree_node( pptr<T> ...`.
        REQUIRE( contents.find( "& tree_node(" ) == std::string::npos );
    }
}

TEST_CASE( "I375-X: parray/pstring no longer call ManagerT::allocate directly", "[issue375][static]" )
{
    const std::filesystem::path root = std::filesystem::path( PMM_SOURCE_DIR ) / "include" / "pmm";
    for ( const auto& name : { "parray.h", "pstring.h" } )
    {
        const std::string contents = slurp( root / name );
        INFO( name );
        REQUIRE( contents.find( "ManagerT::allocate(" ) == std::string::npos );
    }
}

TEST_CASE( "I375-X: legacy pptr::tree_node() ref API is gone (only try_tree_node/tree_node_unchecked)",
           "[issue375][static]" )
{
    const std::filesystem::path root     = std::filesystem::path( PMM_SOURCE_DIR ) / "include" / "pmm" / "pptr.h";
    const std::string           contents = slurp( root );
    REQUIRE( contents.find( "try_tree_node" ) != std::string::npos );
    REQUIRE( contents.find( "tree_node_unchecked" ) != std::string::npos );
    // No bare `tree_node()` — this would only match if the legacy method came back.
    REQUIRE( contents.find( "auto& tree_node()" ) == std::string::npos );
}
