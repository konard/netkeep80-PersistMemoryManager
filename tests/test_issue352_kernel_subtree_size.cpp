#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <string>

namespace
{

std::string read_file( const std::filesystem::path& path )
{
    std::ifstream input( path );
    REQUIRE( input.good() );
    return std::string( std::istreambuf_iterator<char>( input ), std::istreambuf_iterator<char>() );
}

std::string regex_capture( const std::string& text, const std::string& pattern )
{
    const std::regex expression( pattern );
    std::smatch      match;
    REQUIRE( std::regex_search( text, match, expression ) );
    REQUIRE( match.size() == 2 );
    return match[1].str();
}

struct SizeRule
{
    std::string glob;
    std::size_t max_bytes;
};

SizeRule load_kernel_subtree_rules( const std::filesystem::path& repo_root )
{
    const auto policy    = read_file( repo_root / "repo-policy.json" );
    const auto byte_rule = regex_capture( policy, R"re((\{[^{}]*"id"\s*:\s*"kernel-subtree-max-bytes"[^{}]*\}))re" );

    return {
        regex_capture( byte_rule, R"re("glob"\s*:\s*"([^"]+)")re" ),
        static_cast<std::size_t>( std::stoull( regex_capture( byte_rule, R"re("max"\s*:\s*([0-9]+))re" ) ) ),
    };
}

std::filesystem::path directory_from_glob( const std::filesystem::path& repo_root, const std::string& glob )
{
    const std::string recursive_suffix = "/**";
    REQUIRE( glob.size() > recursive_suffix.size() );
    REQUIRE( glob.compare( glob.size() - recursive_suffix.size(), recursive_suffix.size(), recursive_suffix ) == 0 );
    return repo_root / glob.substr( 0, glob.size() - recursive_suffix.size() );
}

} // namespace

TEST_CASE( "issue352/360/365: include subtree stays below the kernel size budget",
           "[issue352][issue360][issue365][repo-guard]" )
{
    const std::filesystem::path repo_root = PMM_SOURCE_DIR;
    const auto                  rule      = load_kernel_subtree_rules( repo_root );
    const auto                  rule_root = directory_from_glob( repo_root, rule.glob );

    std::size_t total_bytes = 0;
    for ( const auto& entry : std::filesystem::recursive_directory_iterator( rule_root ) )
    {
        if ( entry.is_regular_file() )
            total_bytes += static_cast<std::size_t>( std::filesystem::file_size( entry.path() ) );
    }

    REQUIRE( total_bytes <= rule.max_bytes );
}
