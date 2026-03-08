/**
 * @file test_issue123_sh_industrial_db.cpp
 * @brief Тест самодостаточности pmm_industrial_db_heap.h (Issue #123).
 *
 * Проверяет, что single-header файл pmm_industrial_db_heap.h является
 * полностью автономным: пользователь может скопировать один файл в свой
 * проект и использовать пресет IndustrialDBHeap без дополнительных зависимостей.
 *
 * Этот файл намеренно не использует никаких других include из include/pmm/.
 *
 * @see include/pmm_industrial_db_heap.h
 * @version 0.1 (Issue #123 — single-header preset generation)
 */

#include "pmm_industrial_db_heap.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
    std::cout << "=== test_issue123_sh_industrial_db (pmm_industrial_db_heap.h) ===\n";

    using IDB = pmm::presets::IndustrialDBHeap;

    assert( !IDB::is_initialized() );
    bool created = IDB::create( 32 * 1024 );
    assert( created );
    assert( IDB::is_initialized() );
    assert( IDB::total_size() >= 32 * 1024 );

    void* ptr = IDB::allocate( 256 );
    assert( ptr != nullptr );
    std::memset( ptr, 0xDD, 256 );
    IDB::deallocate( ptr );

    IDB::pptr<double> p = IDB::allocate_typed<double>();
    assert( !p.is_null() );
    *p.resolve() = 2.718;
    assert( *p.resolve() == 2.718 );
    IDB::deallocate_typed( p );

    IDB::destroy();
    assert( !IDB::is_initialized() );

    std::cout << "PASSED\n";
    return 0;
}
