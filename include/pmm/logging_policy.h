#pragma once

#include "pmm/types.h"

#include <cstddef>
#include <cstdio>

namespace pmm
{
namespace logging
{

/*
## pmm::logging::NoLogging
*/
struct NoLogging
{

/*
### pmm::logging::NoLogging::on_allocation_failure
*/
    static void on_allocation_failure( std::size_t  , PmmError   ) noexcept {}

/*
### pmm::logging::NoLogging::on_expand
*/
    static void on_expand( std::size_t  , std::size_t   ) noexcept {}

/*
### pmm::logging::NoLogging::on_corruption_detected
*/
    static void on_corruption_detected( PmmError   ) noexcept {}

/*
### pmm::logging::NoLogging::on_create
*/
    static void on_create( std::size_t   ) noexcept {}

/*
### pmm::logging::NoLogging::on_destroy
*/
    static void on_destroy() noexcept {}

/*
### pmm::logging::NoLogging::on_load
*/
    static void on_load() noexcept {}
};

/*
## pmm::logging::StderrLogging
*/
struct StderrLogging
{

/*
### pmm::logging::StderrLogging::on_allocation_failure
*/
    static void on_allocation_failure( std::size_t user_size, PmmError err ) noexcept
    {

        std::fprintf( stderr, "[pmm] allocation_failure: size=%zu error=%d\n", user_size, static_cast<int>( err ) );
    }

/*
### pmm::logging::StderrLogging::on_expand
*/
    static void on_expand( std::size_t old_size, std::size_t new_size ) noexcept
    {
        std::fprintf( stderr, "[pmm] expand: %zu -> %zu\n", old_size, new_size );
    }

/*
### pmm::logging::StderrLogging::on_corruption_detected
*/
    static void on_corruption_detected( PmmError err ) noexcept
    {
        std::fprintf( stderr, "[pmm] corruption_detected: error=%d\n", static_cast<int>( err ) );
    }

/*
### pmm::logging::StderrLogging::on_create
*/
    static void on_create( std::size_t initial_size ) noexcept
    {
        std::fprintf( stderr, "[pmm] create: size=%zu\n", initial_size );
    }

/*
### pmm::logging::StderrLogging::on_destroy
*/
    static void on_destroy() noexcept { std::fprintf( stderr, "[pmm] destroy\n" ); }

/*
### pmm::logging::StderrLogging::on_load
*/
    static void on_load() noexcept { std::fprintf( stderr, "[pmm] load\n" ); }
};

}
}
