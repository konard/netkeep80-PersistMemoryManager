#pragma once
#include "pmm/types.h"
#include <cstddef>
#include <cstdio>
namespace pmm
{
namespace logging
{
/*
### pmm-logging-nologging
*/
struct NoLogging
{
    static void on_allocation_failure( size_t, PmmError ) noexcept {}
    static void on_expand( size_t, size_t ) noexcept {}
    static void on_corruption_detected( PmmError ) noexcept {}
    static void on_create( size_t ) noexcept {}
    static void on_destroy() noexcept {}
    static void on_load() noexcept {}
};
/*
### pmm-logging-stderrlogging
*/
struct StderrLogging
{
    static void on_allocation_failure( size_t user_size, PmmError err ) noexcept
    {
        std::fprintf( stderr, "[pmm] allocation_failure: size=%zu error=%d\n", user_size, static_cast<int>( err ) );
    }
    static void on_expand( size_t old_size, size_t new_size ) noexcept
    {
        std::fprintf( stderr, "[pmm] expand: %zu -> %zu\n", old_size, new_size );
    }
    static void on_corruption_detected( PmmError err ) noexcept
    {
        std::fprintf( stderr, "[pmm] corruption_detected: error=%d\n", static_cast<int>( err ) );
    }
    static void on_create( size_t initial_size ) noexcept
    {
        std::fprintf( stderr, "[pmm] create: size=%zu\n", initial_size );
    }
    static void on_destroy() noexcept { std::fprintf( stderr, "[pmm] destroy\n" ); }
    static void on_load() noexcept { std::fprintf( stderr, "[pmm] load\n" ); }
};
}
}
