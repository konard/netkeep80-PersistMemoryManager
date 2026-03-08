/**
 * @file test_issue123_sh_embedded.cpp
 * @brief Тест самодостаточности pmm_embedded_heap.h (Issue #123).
 *
 * Проверяет, что single-header файл pmm_embedded_heap.h является
 * полностью автономным: пользователь может скопировать один файл в свой
 * проект и использовать пресет EmbeddedHeap без дополнительных зависимостей.
 *
 * Этот файл намеренно не использует никаких других include из include/pmm/.
 *
 * @see include/pmm_embedded_heap.h
 * @version 0.1 (Issue #123 — single-header preset generation)
 */

#include "pmm_embedded_heap.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
    std::cout << "=== test_issue123_sh_embedded (pmm_embedded_heap.h) ===\n";

    using EMB = pmm::presets::EmbeddedHeap;

    assert( !EMB::is_initialized() );
    assert( EMB::create( 16 * 1024 ) );
    assert( EMB::is_initialized() );
    assert( EMB::total_size() >= 16 * 1024 );

    void* ptr = EMB::allocate( 128 );
    assert( ptr != nullptr );
    std::memset( ptr, 0xCC, 128 );
    EMB::deallocate( ptr );

    EMB::pptr<int> p = EMB::allocate_typed<int>();
    assert( !p.is_null() );
    *p.resolve() = 99;
    assert( *p.resolve() == 99 );
    EMB::deallocate_typed( p );

    EMB::destroy();
    assert( !EMB::is_initialized() );

    std::cout << "PASSED\n";
    return 0;
}
