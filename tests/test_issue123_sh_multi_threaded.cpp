/**
 * @file test_issue123_sh_multi_threaded.cpp
 * @brief Тест самодостаточности pmm_multi_threaded_heap.h (Issue #123).
 *
 * Проверяет, что single-header файл pmm_multi_threaded_heap.h является
 * полностью автономным: пользователь может скопировать один файл в свой
 * проект и использовать пресет MultiThreadedHeap без дополнительных зависимостей.
 *
 * Этот файл намеренно не использует никаких других include из include/pmm/.
 *
 * @see include/pmm_multi_threaded_heap.h
 * @version 0.1 (Issue #123 — single-header preset generation)
 */

#include "pmm_multi_threaded_heap.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
    std::cout << "=== test_issue123_sh_multi_threaded (pmm_multi_threaded_heap.h) ===\n";

    using MTH = pmm::presets::MultiThreadedHeap;

    assert( !MTH::is_initialized() );
    assert( MTH::create( 16 * 1024 ) );
    assert( MTH::is_initialized() );
    assert( MTH::total_size() >= 16 * 1024 );

    void* ptr = MTH::allocate( 128 );
    assert( ptr != nullptr );
    std::memset( ptr, 0xBB, 128 );
    MTH::deallocate( ptr );

    MTH::pptr<double> p = MTH::allocate_typed<double>();
    assert( !p.is_null() );
    *p.resolve() = 3.14;
    assert( *p.resolve() == 3.14 );
    MTH::deallocate_typed( p );

    MTH::destroy();
    assert( !MTH::is_initialized() );

    std::cout << "PASSED\n";
    return 0;
}
