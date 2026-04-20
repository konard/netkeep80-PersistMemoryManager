/**
 * @file pmm/io.h
 * @brief Утилиты файлового ввода/вывода для PersistMemoryManager
 *
 * Save/load helpers for a manager image.
 *
 * save_manager copies a locked manager snapshot, stores ManagerHeader.crc32 in
 * the copy, writes through filename + ".tmp", fsyncs the temp file, then uses
 * an atomic rename and fsyncs the parent directory where supported.
 * load_manager_from_file requires VerifyResult& and verifies CRC before
 * loading manager state.
 *
 * @version 0.3
 */

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
#include <cstdlib> // std::rename (POSIX)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace pmm
{

namespace detail
{

/// @brief Atomic file rename (write-then-rename pattern).
/// On POSIX: std::rename is atomic if source and dest are on the same filesystem.
/// On Windows: MoveFileExA with MOVEFILE_REPLACE_EXISTING and MOVEFILE_WRITE_THROUGH.
/// @return true on success.
inline bool atomic_rename( const char* tmp_path, const char* final_path ) noexcept
{
#if defined( _WIN32 ) || defined( _WIN64 )
    return MoveFileExA( tmp_path, final_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH ) != 0;
#else
    return std::rename( tmp_path, final_path ) == 0;
#endif
}

/// @brief Flush stdio buffers and force the temporary file contents to stable storage.
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

/// @brief Flush the directory entry that makes the atomic rename durable, where supported.
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

} // namespace detail

/**
 * @brief Сохранить образ PersistMemoryManager в файл.
 *
 * Takes the manager lock, copies a stable snapshot of the entire managed
 * region, computes CRC32 over that snapshot (treating the crc32 field as zero),
 * and stores the checksum only in the snapshot before writing.
 *
 * Uses atomic write-then-rename — writes to "filename.tmp",
 * fsyncs the temp file, then renames to "filename" on success. On POSIX the
 * parent directory is also fsynced after rename. If the process crashes before
 * rename, the original file remains intact.
 *
 * @tparam MgrT    Тип статического менеджера (PersistMemoryManager<ConfigT, Id>).
 * @param filename Путь к выходному файлу.
 * @return true при успешной записи, false при ошибке ввода/вывода или если не инициализирован.
 *
 * Предусловие:  filename != nullptr, MgrT::is_initialized() == true.
 * Постусловие: файл содержит консистентную копию управляемой области памяти с CRC32.
 */
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

    // Atomic save — write to temp file, then rename.
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

    // Atomic rename: tmp → final
    if ( !detail::atomic_rename( tmp_path.c_str(), filename ) )
    {
        std::remove( tmp_path.c_str() );
        return false;
    }
    if ( !detail::flush_parent_directory( filename ) )
        return false;
    return true;
}

/**
 * @brief Загрузить образ менеджера из файла в PersistMemoryManager с диагностикой.
 *
 * Читает файл, записанный функцией save_manager(), в буфер менеджера,
 * затем вызывает MgrT::load(result) для проверки заголовка и восстановления состояния.
 * Все обнаруженные нарушения и выполненные исправления записываются в result.
 *
 * If the CRC does not match, returns false without modifying manager state.
 *
 * Примечание: бэкенд менеджера должен уже иметь выделенный буфер достаточного размера.
 * Для HeapStorage вызовите MgrT::create(size) перед load_manager_from_file().
 *
 * @tparam MgrT    Тип статического менеджера (PersistMemoryManager<ConfigT, Id>).
 * @param filename Путь к файлу с образом.
 * @param result   VerifyResult, заполняемый обнаруженными нарушениями и выполненными исправлениями.
 * @return true при успешной загрузке, false при ошибке.
 *
 * Предусловие:  filename != nullptr, MgrT::backend().base_ptr() != nullptr.
 * Постусловие: менеджер восстановлен из файла, result содержит полную диагностику.
 */
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

    // Verify CRC32 before calling load().
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

} // namespace pmm
