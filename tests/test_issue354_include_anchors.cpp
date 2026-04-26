#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace
{

std::string read_file( const std::filesystem::path& path )
{
    std::ifstream input( path );
    REQUIRE( input.good() );
    return std::string( std::istreambuf_iterator<char>( input ), std::istreambuf_iterator<char>() );
}

std::size_t line_number_at( std::string_view text, std::size_t pos )
{
    std::size_t line = 1;
    for ( std::size_t i = 0; i < pos && i < text.size(); ++i )
    {
        if ( text[i] == '\n' )
            ++line;
    }
    return line;
}

bool is_anchor_comment( std::string_view comment )
{
    static const std::regex anchor_pattern(
        R"re(^/\*\n\s*(##|###) [A-Za-z_][A-Za-z0-9_]*(::[A-Za-z_~][A-Za-z0-9_~]*)*\n\s*\*/$)re" );
    return std::regex_match( std::string( comment ), anchor_pattern );
}

void skip_quoted_literal( std::string_view text, std::size_t& pos )
{
    const char quote = text[pos++];
    while ( pos < text.size() )
    {
        if ( text[pos] == '\\' )
        {
            pos += 2;
            continue;
        }
        if ( text[pos++] == quote )
            return;
    }
}

void skip_raw_string_literal( std::string_view text, std::size_t& pos )
{
    const auto open = text.find( '(', pos + 2 );
    if ( open == std::string_view::npos )
    {
        pos += 2;
        return;
    }

    const auto delimiter = text.substr( pos + 2, open - ( pos + 2 ) );
    const auto close     = text.find( std::string( ")" ) + std::string( delimiter ) + "\"", open + 1 );
    if ( close == std::string_view::npos )
    {
        pos = text.size();
        return;
    }

    pos = close + delimiter.size() + 2;
}

std::vector<std::filesystem::path> source_files_under( const std::filesystem::path& root )
{
    std::vector<std::filesystem::path> files;
    for ( const auto& entry : std::filesystem::recursive_directory_iterator( root ) )
    {
        if ( !entry.is_regular_file() )
            continue;

        const auto extension = entry.path().extension().string();
        if ( extension == ".h" || extension == ".inc" )
            files.push_back( entry.path() );
    }
    return files;
}

std::vector<std::filesystem::path> markdown_files_under( const std::filesystem::path& root )
{
    std::vector<std::filesystem::path> files;
    for ( const auto& entry : std::filesystem::recursive_directory_iterator( root ) )
    {
        if ( entry.is_regular_file() && entry.path().extension() == ".md" )
            files.push_back( entry.path() );
    }
    return files;
}

std::vector<std::string> validate_comments_are_anchors( const std::filesystem::path& path )
{
    const auto               text = read_file( path );
    std::vector<std::string> failures;

    std::size_t pos = 0;
    while ( pos < text.size() )
    {
        if ( text[pos] == '"' || text[pos] == '\'' )
        {
            skip_quoted_literal( text, pos );
            continue;
        }
        if ( text[pos] == 'R' && pos + 1 < text.size() && text[pos + 1] == '"' )
        {
            skip_raw_string_literal( text, pos );
            continue;
        }

        if ( pos + 1 < text.size() && text[pos] == '/' && text[pos + 1] == '/' )
        {
            std::ostringstream failure;
            failure << path << ':' << line_number_at( text, pos ) << ": line comment is not an anchor";
            failures.push_back( failure.str() );
            pos += 2;
            continue;
        }

        if ( pos + 1 < text.size() && text[pos] == '/' && text[pos + 1] == '*' )
        {
            const auto end = text.find( "*/", pos + 2 );
            if ( end == std::string::npos )
            {
                std::ostringstream failure;
                failure << path << ':' << line_number_at( text, pos ) << ": unterminated block comment";
                failures.push_back( failure.str() );
                break;
            }

            const auto comment = std::string_view( text ).substr( pos, end + 2 - pos );
            if ( !is_anchor_comment( comment ) )
            {
                std::ostringstream failure;
                failure << path << ':' << line_number_at( text, pos ) << ": block comment is not a PMM anchor";
                failures.push_back( failure.str() );
            }
            pos = end + 2;
            continue;
        }

        ++pos;
    }

    return failures;
}

std::vector<std::string> anchors_in( const std::string& text )
{
    static const std::regex  anchor_pattern(
        R"re(/\*\n\s*(?:##|###) ([A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_~][A-Za-z0-9_~]*)*)\n\s*\*/)re" );
    std::vector<std::string> anchors;
    for ( std::sregex_iterator it( text.begin(), text.end(), anchor_pattern ), end; it != end; ++it )
        anchors.push_back( ( *it )[1].str() );
    return anchors;
}

bool file_contains_anchor( const std::filesystem::path& path, const std::string& anchor )
{
    const auto file_anchors = anchors_in( read_file( path ) );
    return std::find( file_anchors.begin(), file_anchors.end(), anchor ) != file_anchors.end();
}

std::vector<std::string> validate_markdown_include_links( const std::filesystem::path& repo_root,
                                                          const std::filesystem::path& path )
{
    static const std::regex  link_pattern( R"re(\[[^\]]+\]\(([^)#]*include/pmm/[^)#]+\.(?:h|inc))#([A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_~][A-Za-z0-9_~]*)*)\))re" );
    const auto               text = read_file( path );
    std::vector<std::string> failures;

    for ( std::sregex_iterator it( text.begin(), text.end(), link_pattern ), end; it != end; ++it )
    {
        auto target = ( *it )[1].str();
        while ( target.rfind( "./", 0 ) == 0 )
            target.erase( 0, 2 );
        while ( target.rfind( "../", 0 ) == 0 )
            target.erase( 0, 3 );

        const auto target_path = repo_root / target;
        const auto anchor      = ( *it )[2].str();
        if ( !std::filesystem::exists( target_path ) )
        {
            std::ostringstream failure;
            failure << path << ": linked include file does not exist: " << target;
            failures.push_back( failure.str() );
            continue;
        }

        if ( !file_contains_anchor( target_path, anchor ) )
        {
            std::ostringstream failure;
            failure << path << ": linked anchor does not exist: " << target << '#' << anchor;
            failures.push_back( failure.str() );
        }
    }

    return failures;
}

} // namespace

TEST_CASE( "issue354: include comments are limited to code anchors", "[issue354][docs]" )
{
    const std::filesystem::path repo_root = PMM_SOURCE_DIR;

    std::vector<std::string> failures;
    for ( const auto& path : source_files_under( repo_root / "include" ) )
    {
        auto file_failures = validate_comments_are_anchors( path );
        failures.insert( failures.end(), file_failures.begin(), file_failures.end() );
    }

    INFO( "Failures:\n" << [&failures] {
        std::ostringstream out;
        for ( const auto& failure : failures )
            out << failure << '\n';
        return out.str();
    }() );
    REQUIRE( failures.empty() );
}

TEST_CASE( "issue354: markdown include links resolve to code anchors", "[issue354][docs]" )
{
    const std::filesystem::path repo_root = PMM_SOURCE_DIR;

    std::vector<std::string> failures;
    for ( const auto& path : markdown_files_under( repo_root ) )
    {
        auto file_failures = validate_markdown_include_links( repo_root, path );
        failures.insert( failures.end(), file_failures.begin(), file_failures.end() );
    }

    INFO( "Failures:\n" << [&failures] {
        std::ostringstream out;
        for ( const auto& failure : failures )
            out << failure << '\n';
        return out.str();
    }() );
    REQUIRE( failures.empty() );
}
