#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace
{

std::string read_file( const std::filesystem::path& path )
{
    std::ifstream input( path );
    REQUIRE( input.good() );
    return std::string( std::istreambuf_iterator<char>( input ), std::istreambuf_iterator<char>() );
}

std::vector<std::string> release_single_header_commands( const std::string& workflow )
{
    std::vector<std::string> commands;
    std::istringstream       lines( workflow );
    std::string              line;

    while ( std::getline( lines, line ) )
    {
        if ( line.find( "run: bash scripts/generate-single-headers.sh" ) != std::string::npos )
            commands.push_back( line );
    }

    return commands;
}

} // namespace

TEST_CASE( "issue358: release workflow uses the supported single-header generator command", "[issue358][release]" )
{
    const auto workflow =
        read_file( std::filesystem::path( PMM_SOURCE_DIR ) / ".github" / "workflows" / "release.yml" );
    const auto commands = release_single_header_commands( workflow );

    REQUIRE( commands.size() == 2 );
    for ( const auto& command : commands )
    {
        REQUIRE( command == "        run: bash scripts/generate-single-headers.sh" );
        REQUIRE( command.find( "--strip-comments" ) == std::string::npos );
    }
}
