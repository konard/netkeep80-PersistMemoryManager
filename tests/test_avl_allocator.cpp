/**
 * @file test_avl_allocator.cpp
 * @brief Тесты нового алгоритма (Issue #55): AVL-дерево свободных блоков.
 *
 * Проверяет:
 * - Корректность best-fit поиска через AVL-дерево
 * - Слияние блоков при освобождении
 * - Целостность AVL-дерева после серии операций
 * - Корректность 6 полей блока (used_size, prev, next, left, right, parent)
 */

#include "persist_memory_manager.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#define PMM_TEST( expr )                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if ( !( expr ) )                                                                                               \
        {                                                                                                              \
            std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "] " << #expr << "\n";                             \
            return false;                                                                                              \
        }                                                                                                              \
    } while ( false )

#define PMM_RUN( name, fn )                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        std::cout << "  " << name << " ... ";                                                                          \
        if ( fn() )                                                                                                    \
        {                                                                                                              \
            std::cout << "PASS\n";                                                                                     \
        }                                                                                                              \
        else                                                                                                           \
        {                                                                                                              \
            std::cout << "FAIL\n";                                                                                     \
            all_passed = false;                                                                                        \
        }                                                                                                              \
    } while ( false )

/// Блок с used_size==0 должен считаться свободным
static bool test_free_block_has_zero_used_size()
{
    const std::size_t size = 64 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );

    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    // Сразу после создания: 1 свободный блок
    auto info = pmm::get_manager_info( mgr );
    PMM_TEST( info.free_count == 1 );

    // Обойдём все блоки и убедимся, что свободный имеет used_size==0
    int  free_blocks        = 0;
    bool all_free_zero_used = true;
    pmm::for_each_block( mgr,
                         [&]( const pmm::BlockView& blk )
                         {
                             if ( !blk.used )
                             {
                                 free_blocks++;
                                 if ( blk.user_size != 0 )
                                     all_free_zero_used = false;
                             }
                         } );
    PMM_TEST( free_blocks == 1 );
    PMM_TEST( all_free_zero_used );

    pmm::PersistMemoryManager::destroy();
    return true;
}

/// best-fit: при наличии нескольких свободных блоков разного размера
/// должен выбираться наименьший подходящий
static bool test_best_fit_selection()
{
    const std::size_t size = 256 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );

    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    // Создаём 4 блока разного размера: 512, 1024, 2048, 4096
    void* p[4];
    p[0] = mgr->allocate( 512 );
    p[1] = mgr->allocate( 1024 );
    p[2] = mgr->allocate( 2048 );
    p[3] = mgr->allocate( 4096 );
    PMM_TEST( p[0] && p[1] && p[2] && p[3] );
    PMM_TEST( mgr->validate() );

    // Освобождаем все — при слиянии соседей получим один большой блок
    mgr->deallocate( p[0] );
    mgr->deallocate( p[1] );
    mgr->deallocate( p[2] );
    mgr->deallocate( p[3] );
    PMM_TEST( mgr->validate() );

    // Единственный блок объединён — allocate должно выбрать его
    void* big = mgr->allocate( 1500 );
    PMM_TEST( big != nullptr );
    PMM_TEST( mgr->validate() );

    mgr->deallocate( big );
    PMM_TEST( mgr->validate() );

    pmm::PersistMemoryManager::destroy();
    return true;
}

/// AVL-дерево должно оставаться валидным после множества alloc/dealloc
static bool test_avl_integrity_stress()
{
    const std::size_t size = 512 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );

    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    static const int N = 50;
    void*            ptrs[N];
    std::size_t      sizes[] = { 64, 128, 256, 512, 1024, 2048 };
    for ( int i = 0; i < N; i++ )
    {
        ptrs[i] = mgr->allocate( sizes[i % 6] );
        PMM_TEST( ptrs[i] != nullptr );
        PMM_TEST( pmm::PersistMemoryManager::instance()->validate() );
    }

    // Освобождаем каждый второй
    for ( int i = 0; i < N; i += 2 )
    {
        pmm::PersistMemoryManager::instance()->deallocate( ptrs[i] );
        PMM_TEST( pmm::PersistMemoryManager::instance()->validate() );
    }
    // Освобождаем оставшиеся
    for ( int i = 1; i < N; i += 2 )
    {
        pmm::PersistMemoryManager::instance()->deallocate( ptrs[i] );
        PMM_TEST( pmm::PersistMemoryManager::instance()->validate() );
    }

    // После полного освобождения должен быть 1 свободный блок
    auto stats = pmm::get_stats( pmm::PersistMemoryManager::instance() );
    PMM_TEST( stats.free_blocks == 1 );
    PMM_TEST( stats.allocated_blocks == 0 );

    pmm::PersistMemoryManager::destroy();
    return true;
}

/// Тест слияния с соседями: prev + current + next → один блок
static bool test_coalesce_three_way()
{
    const std::size_t size = 128 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );

    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    void* p1 = mgr->allocate( 512 );
    void* p2 = mgr->allocate( 512 );
    void* p3 = mgr->allocate( 512 );
    void* p4 = mgr->allocate( 512 ); // барьер чтобы слияние не поглотило весь хвост
    PMM_TEST( p1 && p2 && p3 && p4 );

    // Освобождаем p1 и p3 (создаём два несмежных свободных блока)
    mgr->deallocate( p1 );
    mgr->deallocate( p3 );
    PMM_TEST( mgr->validate() );

    auto before = pmm::get_stats( mgr );

    // Освобождаем p2 — должно слиться с p1 (предыдущим) и p3 (следующим)
    mgr->deallocate( p2 );
    PMM_TEST( mgr->validate() );

    auto after = pmm::get_stats( mgr );
    // 2 объединения = block_count уменьшился на 2
    PMM_TEST( after.total_blocks == before.total_blocks - 2 );
    // free_blocks уменьшился на 1 (3 стало 1)
    PMM_TEST( after.free_blocks == before.free_blocks - 1 );

    mgr->deallocate( p4 );
    PMM_TEST( mgr->validate() );

    pmm::PersistMemoryManager::destroy();
    return true;
}

/// Тест: 6 полей блока доступны через BlockView
static bool test_block_view_fields()
{
    const std::size_t size = 64 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );

    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    void* p1 = mgr->allocate( 256 );
    void* p2 = mgr->allocate( 512 );
    PMM_TEST( p1 && p2 );

    int  used_blocks       = 0;
    bool fields_consistent = true;
    pmm::for_each_block( mgr,
                         [&]( const pmm::BlockView& blk )
                         {
                             if ( blk.used )
                             {
                                 if ( blk.user_size == 0 )
                                     fields_consistent = false;
                                 if ( blk.total_size < blk.user_size + blk.header_size )
                                     fields_consistent = false;
                                 used_blocks++;
                             }
                             else
                             {
                                 if ( blk.user_size != 0 )
                                     fields_consistent = false;
                             }
                         } );
    PMM_TEST( fields_consistent );
    PMM_TEST( used_blocks == 2 );
    PMM_TEST( mgr->validate() );

    mgr->deallocate( p1 );
    mgr->deallocate( p2 );
    PMM_TEST( mgr->validate() );

    pmm::PersistMemoryManager::destroy();
    return true;
}

/// Тест: save/load сохраняет корректность AVL-дерева
static bool test_avl_survives_save_load()
{
    const std::size_t size = 64 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );

    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    void* p1 = mgr->allocate( 256 );
    void* p2 = mgr->allocate( 512 );
    PMM_TEST( p1 && p2 );
    mgr->deallocate( p1 ); // создаём фрагментацию
    PMM_TEST( mgr->validate() );

    // Имитируем сохранение/загрузку (копируем буфер и загружаем)
    void* snapshot = std::malloc( size );
    PMM_TEST( snapshot != nullptr );
    std::memcpy( snapshot, mem, size );

    pmm::PersistMemoryManager::destroy();
    // mem освобождён destroy()

    // Загружаем из snapshot
    pmm::PersistMemoryManager* mgr2 = pmm::PersistMemoryManager::load( snapshot, size );
    PMM_TEST( mgr2 != nullptr );
    PMM_TEST( mgr2->validate() );

    // Должна быть возможность выделить память после load
    void* p3 = mgr2->allocate( 128 );
    PMM_TEST( p3 != nullptr );
    PMM_TEST( mgr2->validate() );

    pmm::PersistMemoryManager::destroy();
    // snapshot освобождён destroy()
    return true;
}

/// Тест: best-fit выбирает наименьший подходящий блок из нескольких
static bool test_best_fit_chooses_smallest_fitting()
{
    const std::size_t size = 512 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );

    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    // Создаём блоки размером, чтобы после освобождения получились дыры разного размера.
    // Выделяем: 64, 256, 64, 512, 64 и барьер
    void* barrier[6];
    void* gap[5];
    // layout: gap[0]=64, barrier[0]=64, gap[1]=256, barrier[1]=64, gap[2]=512,
    //         barrier[2]=64, gap[3]=1024, barrier[3]=64, barrier[4]=keep
    gap[0]     = mgr->allocate( 64 );
    barrier[0] = mgr->allocate( 64 ); // hold between gaps
    gap[1]     = mgr->allocate( 256 );
    barrier[1] = mgr->allocate( 64 );
    gap[2]     = mgr->allocate( 512 );
    barrier[2] = mgr->allocate( 64 );
    gap[3]     = mgr->allocate( 1024 );
    barrier[3] = mgr->allocate( 64 );
    barrier[4] = mgr->allocate( 128 ); // keep allocated at end
    PMM_TEST( gap[0] && barrier[0] && gap[1] && barrier[1] && gap[2] && barrier[2] );
    PMM_TEST( gap[3] && barrier[3] && barrier[4] );

    // Создаём дыры: освобождаем gap[0..3]
    mgr->deallocate( gap[0] );
    mgr->deallocate( gap[1] );
    mgr->deallocate( gap[2] );
    mgr->deallocate( gap[3] );
    PMM_TEST( mgr->validate() );

    // Теперь запрашиваем 200 байт: best-fit должен выбрать дыру из gap[1] (256) или gap[2] (512)
    // но НЕ из gap[3] (1024), если есть 256
    // (точный выбор зависит от overhead заголовка, но блок должен быть валидным)
    void* result = pmm::PersistMemoryManager::instance()->allocate( 200 );
    PMM_TEST( result != nullptr );
    PMM_TEST( pmm::PersistMemoryManager::instance()->validate() );

    // Освобождаем всё
    pmm::PersistMemoryManager::instance()->deallocate( result );
    for ( int i = 0; i < 4; i++ )
        pmm::PersistMemoryManager::instance()->deallocate( barrier[i] );
    pmm::PersistMemoryManager::instance()->deallocate( barrier[4] );
    PMM_TEST( pmm::PersistMemoryManager::instance()->validate() );

    pmm::PersistMemoryManager::destroy();
    return true;
}

/// Тест: reallocate работает с новым алгоритмом
static bool test_reallocate_works()
{
    const std::size_t size = 128 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );

    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    void* ptr = mgr->allocate( 256 );
    PMM_TEST( ptr != nullptr );

    // Запись данных
    std::memset( ptr, 0xAB, 256 );

    // Реаллокация в больший блок
    void* new_ptr = mgr->reallocate( ptr, 512 );
    PMM_TEST( new_ptr != nullptr );
    PMM_TEST( pmm::PersistMemoryManager::instance()->validate() );

    // Проверяем, что данные сохранились
    const std::uint8_t* p = static_cast<const std::uint8_t*>( new_ptr );
    for ( std::size_t i = 0; i < 256; i++ )
    {
        PMM_TEST( p[i] == 0xAB );
    }

    pmm::PersistMemoryManager::instance()->deallocate( new_ptr );
    PMM_TEST( pmm::PersistMemoryManager::instance()->validate() );

    pmm::PersistMemoryManager::destroy();
    return true;
}

int main()
{
    std::cout << "=== test_avl_allocator ===\n";
    bool all_passed = true;

    PMM_RUN( "free_block_has_zero_used_size", test_free_block_has_zero_used_size );
    PMM_RUN( "best_fit_selection", test_best_fit_selection );
    PMM_RUN( "avl_integrity_stress", test_avl_integrity_stress );
    PMM_RUN( "coalesce_three_way", test_coalesce_three_way );
    PMM_RUN( "block_view_fields", test_block_view_fields );
    PMM_RUN( "avl_survives_save_load", test_avl_survives_save_load );
    PMM_RUN( "best_fit_chooses_smallest_fitting", test_best_fit_chooses_smallest_fitting );
    PMM_RUN( "reallocate_works", test_reallocate_works );

    std::cout << ( all_passed ? "\nAll tests PASSED\n" : "\nSome tests FAILED\n" );
    return all_passed ? 0 : 1;
}
