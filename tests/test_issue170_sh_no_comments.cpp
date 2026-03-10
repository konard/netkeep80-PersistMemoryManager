/**
 * @file test_issue170_sh_no_comments.cpp
 * @brief Self-sufficiency test for pmm_no_comments.h (Issue #170).
 *
 * Verifies that the comment-stripped single-header file pmm_no_comments.h is
 * fully self-contained and functionally equivalent to the full pmm.h:
 * the user can copy one file into their project and use PMM without any other
 * dependencies.
 *
 * This file intentionally does not use any other includes from include/pmm/.
 */

#include "pmm_no_comments.h"

#include <cassert>
#include <cstring>
#include <iostream>

int main()
{
    std::cout << "=== test_issue170_sh_no_comments (pmm_no_comments.h) ===\n";

    using MyHeap = pmm::PersistMemoryManager<pmm::CacheManagerConfig>;

    assert( !MyHeap::is_initialized() );
    bool created = MyHeap::create( 32 * 1024 );
    assert( created );
    assert( MyHeap::is_initialized() );
    assert( MyHeap::total_size() >= 32 * 1024 );

    void* ptr = MyHeap::allocate( 256 );
    assert( ptr != nullptr );
    std::memset( ptr, 0xBB, 256 );
    MyHeap::deallocate( ptr );

    MyHeap::pptr<int> p = MyHeap::allocate_typed<int>();
    assert( !p.is_null() );
    *p.resolve() = 99;
    assert( *p.resolve() == 99 );
    MyHeap::deallocate_typed( p );

    MyHeap::destroy();
    assert( !MyHeap::is_initialized() );

    std::cout << "PASSED\n";
    return 0;
}
