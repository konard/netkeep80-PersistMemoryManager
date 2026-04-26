#pragma once

#include "pmm/diagnostics.h"
#include "pmm/types.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined( _WIN32 ) || defined( _WIN64 )
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace pmm
{

namespace detail
{

inline bool atomic_rename( const char* tmp_path, const char* final_path ) noexcept
{
#if defined( _WIN32 ) || defined( _WIN64 )
    return MoveFileExA( tmp_path, final_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) != 0;
#else
    return std::rename( tmp_path, final_path ) == 0;
#endif
}

inline bool flush_file_to_storage( std::FILE* file ) noexcept
{
    if ( file == nullptr )
        return false;
    if ( std::fflush( file ) != 0 )
        return false;
#if defined( _WIN32 ) || defined( _WIN64 )
    int fd = ::_fileno( file );
    return fd >= 0 && ::_commit( fd ) == 0;
#else
    int fd = ::fileno( file );
    return fd >= 0 && ::fsync( fd ) == 0;
#endif
}

#if !defined( _WIN32 ) && !defined( _WIN64 )
inline std::string parent_directory_path( const char* path )
{
    std::string value( path );
    std::size_t slash = value.find_last_of( '/' );
    if ( slash == std::string::npos )
        return ".";
    if ( slash == 0 )
        return "/";
    return value.substr( 0, slash );
}
#endif

inline bool flush_parent_directory( const char* path )
{
#if defined( _WIN32 ) || defined( _WIN64 )
    (void)path;
    return true;
#else
    std::string parent = parent_directory_path( path );
    int         flags  = O_RDONLY;
#ifdef O_DIRECTORY
    flags |= O_DIRECTORY;
#endif
    int fd = ::open( parent.c_str(), flags );
    if ( fd < 0 )
        return false;
    bool ok = ::fsync( fd ) == 0;
    if ( ::close( fd ) != 0 )
        ok = false;
    return ok;
#endif
}

}

template <typename MgrT> inline bool save_manager( const char* filename )
{
    using address_traits = typename MgrT::address_traits;

    if ( filename == nullptr )
        return false;

    std::vector<std::uint8_t> snapshot;
    {
        typename MgrT::thread_policy::shared_lock_type lock( MgrT::_mutex );
        if ( !MgrT::is_initialized() )
            return false;
        const std::uint8_t* data  = MgrT::backend().base_ptr();
        std::size_t         total = MgrT::backend().total_size();
        if ( data == nullptr || total == 0 )
            return false;
        snapshot.resize( total );
        std::memcpy( snapshot.data(), data, total );
    }

    auto* hdr  = detail::manager_header_at<address_traits>( snapshot.data() );
    hdr->crc32 = detail::compute_image_crc32<address_traits>( snapshot.data(), snapshot.size() );

    std::string tmp_path = std::string( filename ) + ".tmp";

    std::FILE* f = std::fopen( tmp_path.c_str(), "wb" );
    if ( f == nullptr )
        return false;

    std::size_t written = std::fwrite( snapshot.data(), 1, snapshot.size(), f );
    bool        ok      = written == snapshot.size();
    if ( ok )
        ok = detail::flush_file_to_storage( f );
    if ( std::fclose( f ) != 0 )
        ok = false;

    if ( !ok )
    {
        std::remove( tmp_path.c_str() );
        return false;
    }

    if ( !detail::atomic_rename( tmp_path.c_str(), filename ) )
    {
        std::remove( tmp_path.c_str() );
        return false;
    }
    if ( !detail::flush_parent_directory( filename ) )
        return false;
    return true;
}

template <typename MgrT> inline bool load_manager_from_file( const char* filename, VerifyResult& result )
{
    using address_traits = typename MgrT::address_traits;

    if ( filename == nullptr )
        return false;

    std::uint8_t* buf  = MgrT::backend().base_ptr();
    std::size_t   size = MgrT::backend().total_size();
    if ( buf == nullptr || size < detail::kMinMemorySize )
        return false;

    std::FILE* f = std::fopen( filename, "rb" );
    if ( f == nullptr )
        return false;

    if ( std::fseek( f, 0, SEEK_END ) != 0 )
    {
        std::fclose( f );
        return false;
    }
    long file_size_long = std::ftell( f );
    if ( file_size_long <= 0 )
    {
        std::fclose( f );
        return false;
    }
    std::rewind( f );

    std::size_t file_size = static_cast<std::size_t>( file_size_long );
    if ( file_size > size )
    {
        std::fclose( f );
        return false;
    }

    std::size_t read_bytes = std::fread( buf, 1, file_size, f );
    std::fclose( f );

    if ( read_bytes != file_size )
        return false;

    constexpr std::size_t kHdrOffset = detail::manager_header_offset_bytes_v<address_traits>;
    if ( file_size >= kHdrOffset + sizeof( detail::ManagerHeader<address_traits> ) )
    {
        auto*         hdr          = detail::manager_header_at<address_traits>( buf );
        std::uint32_t stored_crc   = hdr->crc32;
        std::uint32_t computed_crc = detail::compute_image_crc32<address_traits>( buf, file_size );
        if ( stored_crc != computed_crc )
        {
            MgrT::set_last_error( PmmError::CrcMismatch );
            MgrT::logging_policy::on_corruption_detected( PmmError::CrcMismatch );
            return false;
        }
    }

    return MgrT::load( result );
}

}
