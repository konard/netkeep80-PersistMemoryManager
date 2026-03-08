/**
 * @file test_issue146_sh_embedded_static.cpp
 * @brief Тест самодостаточности pmm_embedded_static_heap.h (Issue #146).
 *
 * Проверяет, что single-header файл pmm_embedded_static_heap.h является
 * полностью автономным: пользователь может скопировать один файл в свой
 * проект и использовать пресет EmbeddedStaticHeap без дополнительных зависимостей.
 *
 * Этот файл намеренно не использует никаких других include из include/pmm/.
 *
 * @see single_include/pmm/pmm_embedded_static_heap.h
 * @version 0.1 (Issue #146 — переосмысление конфигов: EmbeddedStaticHeap single-header)
 */

#include "pmm_embedded_static_heap.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
    std::cout << "=== test_issue146_sh_embedded_static (pmm_embedded_static_heap.h) ===\n";

    // EmbeddedStaticHeap uses InstanceId=0 with default 4096-byte buffer.
    // We use a custom manager to avoid conflicts with other tests sharing InstanceId=0.
    using ESH = pmm::PersistMemoryManager<pmm::EmbeddedStaticConfig<4096>, 1465>;

    assert( !ESH::is_initialized() );
    bool created = ESH::create( 4096 );
    assert( created );
    assert( ESH::is_initialized() );
    assert( ESH::total_size() == 4096 );

    void* ptr = ESH::allocate( 128 );
    assert( ptr != nullptr );
    std::memset( ptr, 0xCC, 128 );
    ESH::deallocate( ptr );

    ESH::pptr<int> p = ESH::allocate_typed<int>();
    assert( !p.is_null() );
    *p.resolve() = 42;
    assert( *p.resolve() == 42 );
    ESH::deallocate_typed( p );

    ESH::destroy();
    assert( !ESH::is_initialized() );

    std::cout << "PASSED\n";
    return 0;
}
