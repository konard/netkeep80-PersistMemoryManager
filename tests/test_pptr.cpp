/**
 * @file test_pptr.cpp
 * @brief Тесты персистного типизированного указателя pptr<T> (Фаза 5)
 *
 * Проверяет корректность работы pptr<T>:
 * - sizeof(pptr<T>) == sizeof(void*);
 * - нулевой указатель по умолчанию;
 * - allocate_typed<T>() возвращает ненулевой pptr<T>;
 * - resolve() возвращает корректный абсолютный указатель;
 * - запись и чтение данных через pptr<T>;
 * - deallocate_typed() освобождает память;
 * - resolve() нулевого pptr<T> возвращает nullptr;
 * - allocate_typed(count) выделяет массив;
 * - resolve_at() обеспечивает доступ к элементам массива;
 * - персистентность: pptr<T> корректен после save/load;
 * - operator bool() для проверки нулевого указателя;
 * - операторы сравнения pptr<T>;
 * - несколько pptr<T> разных типов;
 * - нехватка памяти — allocate_typed возвращает нулевой pptr<T>.
 */

#include "persist_memory_manager.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>

// ─── Вспомогательные макросы ──────────────────────────────────────────────────

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

// ─── Тестовые функции ─────────────────────────────────────────────────────────

/**
 * @brief sizeof(pptr<T>) == sizeof(void*) для разных типов.
 */
static bool test_pptr_sizeof()
{
    PMM_TEST( sizeof( pmm::pptr<int> ) == sizeof( void* ) );
    PMM_TEST( sizeof( pmm::pptr<double> ) == sizeof( void* ) );
    PMM_TEST( sizeof( pmm::pptr<char> ) == sizeof( void* ) );
    PMM_TEST( sizeof( pmm::pptr<std::uint64_t> ) == sizeof( void* ) );
    return true;
}

/**
 * @brief Конструктор по умолчанию создаёт нулевой указатель.
 */
static bool test_pptr_default_null()
{
    pmm::pptr<int> p;
    PMM_TEST( p.is_null() );
    PMM_TEST( !p ); // operator bool() == false для нулевого указателя
    PMM_TEST( p.offset() == 0 );
    return true;
}

/**
 * @brief allocate_typed<int>() возвращает ненулевой pptr<int>.
 */
static bool test_pptr_allocate_typed_int()
{
    const std::size_t size = 64 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );
    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    pmm::pptr<int> p = mgr->allocate_typed<int>();
    PMM_TEST( !p.is_null() );
    PMM_TEST( static_cast<bool>( p ) ); // operator bool() == true для ненулевого
    PMM_TEST( p.offset() > 0 );
    PMM_TEST( mgr->validate() );

    mgr->deallocate_typed( p );
    PMM_TEST( mgr->validate() );

    mgr->destroy();
    std::free( mem );
    return true;
}

/**
 * @brief resolve() возвращает корректный абсолютный указатель.
 */
static bool test_pptr_resolve()
{
    const std::size_t size = 64 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );
    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    pmm::pptr<int> p = mgr->allocate_typed<int>();
    PMM_TEST( !p.is_null() );

    int* ptr = p.resolve( mgr );
    PMM_TEST( ptr != nullptr );
    // Указатель должен находиться внутри управляемой области
    PMM_TEST( ptr >= reinterpret_cast<int*>( mem ) );
    PMM_TEST( ptr < reinterpret_cast<int*>( static_cast<std::uint8_t*>( mem ) + size ) );

    mgr->deallocate_typed( p );
    mgr->destroy();
    std::free( mem );
    return true;
}

/**
 * @brief Запись и чтение данных через pptr<int>.
 */
static bool test_pptr_write_read()
{
    const std::size_t size = 64 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );
    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    pmm::pptr<int> p = mgr->allocate_typed<int>();
    PMM_TEST( !p.is_null() );

    int* ptr = p.resolve( mgr );
    PMM_TEST( ptr != nullptr );

    *ptr = 42;
    PMM_TEST( *p.resolve( mgr ) == 42 );

    *p.resolve( mgr ) = 100;
    PMM_TEST( *ptr == 100 );

    mgr->deallocate_typed( p );
    mgr->destroy();
    std::free( mem );
    return true;
}

/**
 * @brief deallocate_typed() корректно освобождает память, validate() проходит.
 */
static bool test_pptr_deallocate()
{
    const std::size_t size = 64 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );
    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    std::size_t free_before = mgr->free_size();

    pmm::pptr<double> p = mgr->allocate_typed<double>();
    PMM_TEST( !p.is_null() );
    PMM_TEST( mgr->validate() );

    mgr->deallocate_typed( p );
    PMM_TEST( mgr->validate() );

    // После освобождения свободного места должно стать не меньше, чем было
    PMM_TEST( mgr->free_size() >= free_before );

    mgr->destroy();
    std::free( mem );
    return true;
}

/**
 * @brief resolve() нулевого pptr<T> возвращает nullptr.
 */
static bool test_pptr_resolve_null()
{
    const std::size_t size = 64 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );
    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    pmm::pptr<int> p; // нулевой по умолчанию
    int*           ptr = p.resolve( mgr );
    PMM_TEST( ptr == nullptr );

    // resolve() с nullptr менеджером тоже возвращает nullptr
    pmm::pptr<int>             p2       = mgr->allocate_typed<int>();
    pmm::PersistMemoryManager* null_mgr = nullptr;
    PMM_TEST( p2.resolve( null_mgr ) == nullptr );

    mgr->deallocate_typed( p2 );
    mgr->destroy();
    std::free( mem );
    return true;
}

/**
 * @brief allocate_typed(count) выделяет массив из count элементов.
 */
static bool test_pptr_allocate_array()
{
    const std::size_t size  = 256 * 1024;
    const std::size_t count = 10;
    void*             mem   = std::malloc( size );
    PMM_TEST( mem != nullptr );
    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    pmm::pptr<int> p = mgr->allocate_typed<int>( count );
    PMM_TEST( !p.is_null() );
    PMM_TEST( mgr->validate() );

    // Записываем значения в каждый элемент через resolve_at
    for ( std::size_t i = 0; i < count; i++ )
    {
        int* elem = p.resolve_at( mgr, i );
        PMM_TEST( elem != nullptr );
        *elem = static_cast<int>( i * 10 );
    }

    // Читаем и проверяем
    for ( std::size_t i = 0; i < count; i++ )
    {
        PMM_TEST( *p.resolve_at( mgr, i ) == static_cast<int>( i * 10 ) );
    }

    mgr->deallocate_typed( p );
    PMM_TEST( mgr->validate() );

    mgr->destroy();
    std::free( mem );
    return true;
}

/**
 * @brief resolve_at() обеспечивает доступ к элементам массива.
 */
static bool test_pptr_resolve_at()
{
    const std::size_t size  = 256 * 1024;
    const std::size_t count = 5;
    void*             mem   = std::malloc( size );
    PMM_TEST( mem != nullptr );
    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    pmm::pptr<double> p = mgr->allocate_typed<double>( count );
    PMM_TEST( !p.is_null() );

    // Записываем float-значения
    for ( std::size_t i = 0; i < count; i++ )
    {
        *p.resolve_at( mgr, i ) = static_cast<double>( i ) * 1.5;
    }

    // Проверяем последовательный доступ через resolve() (начало массива)
    double* base_elem = p.resolve( mgr );
    PMM_TEST( base_elem != nullptr );
    for ( std::size_t i = 0; i < count; i++ )
    {
        PMM_TEST( base_elem[i] == static_cast<double>( i ) * 1.5 );
    }

    mgr->deallocate_typed( p );
    mgr->destroy();
    std::free( mem );
    return true;
}

/**
 * @brief Персистентность: pptr<T> корректен после save/load образа.
 *
 * Сохраняем образ с выделенным объектом. Загружаем в новый буфер.
 * Смещение pptr<T> должно остаться тем же, данные доступны через resolve().
 */
static bool test_pptr_persistence()
{
    const std::size_t size     = 64 * 1024;
    const char*       filename = "pptr_test.dat";

    // Шаг 1: создаём менеджер, выделяем объект, записываем значение, сохраняем
    void* mem1 = std::malloc( size );
    PMM_TEST( mem1 != nullptr );
    pmm::PersistMemoryManager* mgr1 = pmm::PersistMemoryManager::create( mem1, size );
    PMM_TEST( mgr1 != nullptr );

    pmm::pptr<int> p1 = mgr1->allocate_typed<int>();
    PMM_TEST( !p1.is_null() );
    *p1.resolve( mgr1 ) = 12345;

    std::ptrdiff_t saved_offset = p1.offset();
    PMM_TEST( mgr1->save( filename ) );

    mgr1->destroy();
    std::free( mem1 );

    // Шаг 2: загружаем образ в новый буфер по другому адресу
    void* mem2 = std::malloc( size );
    PMM_TEST( mem2 != nullptr );
    pmm::PersistMemoryManager* mgr2 = pmm::load_from_file( filename, mem2, size );
    PMM_TEST( mgr2 != nullptr );
    PMM_TEST( mgr2->validate() );

    // Восстанавливаем pptr<int> по тому же смещению
    pmm::pptr<int> p2( saved_offset );
    PMM_TEST( !p2.is_null() );

    // Данные должны быть те же
    PMM_TEST( *p2.resolve( mgr2 ) == 12345 );

    mgr2->deallocate_typed( p2 );
    mgr2->destroy();
    std::free( mem2 );
    std::remove( filename );
    return true;
}

/**
 * @brief Операторы сравнения pptr<T>.
 */
static bool test_pptr_comparison()
{
    const std::size_t size = 64 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );
    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    pmm::pptr<int> p1 = mgr->allocate_typed<int>();
    pmm::pptr<int> p2 = mgr->allocate_typed<int>();
    pmm::pptr<int> p3 = p1; // копия

    PMM_TEST( p1 == p3 ); // равны (одно смещение)
    PMM_TEST( p1 != p2 ); // разные смещения
    PMM_TEST( !( p1 == p2 ) );

    mgr->deallocate_typed( p1 );
    mgr->deallocate_typed( p2 );
    mgr->destroy();
    std::free( mem );
    return true;
}

/**
 * @brief Несколько pptr<T> разных типов в одном менеджере.
 */
static bool test_pptr_multiple_types()
{
    const std::size_t size = 256 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );
    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    pmm::pptr<int>    pi = mgr->allocate_typed<int>();
    pmm::pptr<double> pd = mgr->allocate_typed<double>();
    pmm::pptr<char>   pc = mgr->allocate_typed<char>( 16 ); // строка 16 символов

    PMM_TEST( !pi.is_null() );
    PMM_TEST( !pd.is_null() );
    PMM_TEST( !pc.is_null() );
    PMM_TEST( mgr->validate() );

    *pi.resolve( mgr ) = 7;
    *pd.resolve( mgr ) = 3.14;
    std::memcpy( pc.resolve( mgr ), "hello", 6 );

    PMM_TEST( *pi.resolve( mgr ) == 7 );
    PMM_TEST( *pd.resolve( mgr ) == 3.14 );
    PMM_TEST( std::memcmp( pc.resolve( mgr ), "hello", 6 ) == 0 );

    mgr->deallocate_typed( pi );
    mgr->deallocate_typed( pd );
    mgr->deallocate_typed( pc );
    PMM_TEST( mgr->validate() );

    mgr->destroy();
    std::free( mem );
    return true;
}

/**
 * @brief Нехватка памяти — allocate_typed возвращает нулевой pptr<T>.
 */
static bool test_pptr_allocate_oom()
{
    const std::size_t size = 4096; // минимальный размер
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );
    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    // Запрашиваем много памяти — не влезет
    pmm::pptr<std::uint8_t> p = mgr->allocate_typed<std::uint8_t>( 1024 * 1024 );
    PMM_TEST( p.is_null() );
    PMM_TEST( mgr->validate() );

    mgr->destroy();
    std::free( mem );
    return true;
}

/**
 * @brief deallocate_typed нулевого pptr<T> — безопасная операция (нет crash).
 */
static bool test_pptr_deallocate_null()
{
    const std::size_t size = 64 * 1024;
    void*             mem  = std::malloc( size );
    PMM_TEST( mem != nullptr );
    pmm::PersistMemoryManager* mgr = pmm::PersistMemoryManager::create( mem, size );
    PMM_TEST( mgr != nullptr );

    pmm::pptr<int> p;           // нулевой
    mgr->deallocate_typed( p ); // не должно падать
    PMM_TEST( mgr->validate() );

    mgr->destroy();
    std::free( mem );
    return true;
}

// ─── main ─────────────────────────────────────────────────────────────────────

int main()
{
    std::cout << "=== test_pptr ===\n";
    bool all_passed = true;

    PMM_RUN( "pptr_sizeof", test_pptr_sizeof );
    PMM_RUN( "pptr_default_null", test_pptr_default_null );
    PMM_RUN( "pptr_allocate_typed_int", test_pptr_allocate_typed_int );
    PMM_RUN( "pptr_resolve", test_pptr_resolve );
    PMM_RUN( "pptr_write_read", test_pptr_write_read );
    PMM_RUN( "pptr_deallocate", test_pptr_deallocate );
    PMM_RUN( "pptr_resolve_null", test_pptr_resolve_null );
    PMM_RUN( "pptr_allocate_array", test_pptr_allocate_array );
    PMM_RUN( "pptr_resolve_at", test_pptr_resolve_at );
    PMM_RUN( "pptr_persistence", test_pptr_persistence );
    PMM_RUN( "pptr_comparison", test_pptr_comparison );
    PMM_RUN( "pptr_multiple_types", test_pptr_multiple_types );
    PMM_RUN( "pptr_allocate_oom", test_pptr_allocate_oom );
    PMM_RUN( "pptr_deallocate_null", test_pptr_deallocate_null );

    std::cout << ( all_passed ? "\nAll tests PASSED\n" : "\nSome tests FAILED\n" );
    return all_passed ? 0 : 1;
}
