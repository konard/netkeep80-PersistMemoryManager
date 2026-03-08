/**
 * @file test_issue123_sh_single_threaded.cpp
 * @brief Тест самодостаточности pmm_single_threaded_heap.h (Issue #123).
 *
 * Проверяет, что single-header файл pmm_single_threaded_heap.h является
 * полностью автономным: пользователь может скопировать один файл в свой
 * проект и использовать пресет SingleThreadedHeap без дополнительных зависимостей.
 *
 * Этот файл намеренно не использует никаких других include из include/pmm/.
 *
 * @see include/pmm_single_threaded_heap.h
 * @version 0.1 (Issue #123 — single-header preset generation)
 */

#include "pmm_single_threaded_heap.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
    std::cout << "=== test_issue123_sh_single_threaded (pmm_single_threaded_heap.h) ===\n";

    using STH = pmm::presets::SingleThreadedHeap;

    assert( !STH::is_initialized() );
    assert( STH::create( 16 * 1024 ) );
    assert( STH::is_initialized() );
    assert( STH::total_size() >= 16 * 1024 );

    void* ptr = STH::allocate( 128 );
    assert( ptr != nullptr );
    std::memset( ptr, 0xAA, 128 );
    STH::deallocate( ptr );

    STH::pptr<int> p = STH::allocate_typed<int>();
    assert( !p.is_null() );
    *p.resolve() = 42;
    assert( *p.resolve() == 42 );
    STH::deallocate_typed( p );

    STH::destroy();
    assert( !STH::is_initialized() );

    std::cout << "PASSED\n";
    return 0;
}
