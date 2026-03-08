/**
 * @file pmm/manager_configs.h
 * @brief Готовые конфигурационные структуры для менеджеров ПАП (Issue #100, #110, #146).
 *
 * Предоставляет набор предопределённых конфигурационных структур для использования
 * с `PersistMemoryManager<ConfigT, InstanceId>`. Каждая конфигурация описывает
 * типичный сценарий использования менеджера персистентной памяти.
 *
 * Конфигурация включает:
 *   - `address_traits`   — тип адресного пространства (размер индекса, гранулы)
 *   - `storage_backend`  — бэкенд хранилища (HeapStorage, StaticStorage, MMapStorage)
 *   - `free_block_tree`  — политика дерева свободных блоков (AvlFreeTree)
 *   - `lock_policy`      — политика многопоточности (NoLock, SharedMutexLock)
 *   - `granule_size`     — размер гранулы в байтах
 *   - `max_memory_gb`    — максимальный объём памяти в ГБ
 *   - `grow_numerator` / `grow_denominator` — коэффициент роста хранилища
 *
 * Правила выбора конфигурации (Issue #146):
 *
 *   Поддерживаемый размер гранулы: 16 байт (DefaultAddressTraits::granule_size).
 *   Поддерживаемый размер индекса: uint32_t — текущий уровень поддержки PersistMemoryManager.
 *
 *   Ключевые ограничения (задокументированы static_assert-ами):
 *     1. granule_size >= kMinGranuleSize (4 байта — минимум размер слова архитектуры).
 *     2. granule_size — степень двойки.
 *
 *   Архитектурные сценарии:
 *     - Embedded (без heap, статический пул):
 *         StaticStorage<N, DefaultAddressTraits> + NoLock, гранула 16B.
 *         Нет динамического расширения, только однопоточный доступ.
 *     - Desktop/server (до 64 ГБ):
 *         HeapStorage<DefaultAddressTraits> + NoLock/SharedMutexLock, гранула 16B.
 *     - Industrial DB (высоконагруженный):
 *         HeapStorage<DefaultAddressTraits> + SharedMutexLock + агрессивный рост, гранула 16B.
 *
 * Доступные конфигурации:
 *   --- Embedded (статическое хранилище, однопоточный) ---
 *   - `EmbeddedStaticConfig<N>` — StaticStorage<N>, NoLock, 16B гранула (фиксированный пул)
 *
 *   --- Desktop (динамическое хранилище) ---
 *   - `CacheManagerConfig`      — однопоточный, NoLock, HeapStorage, 16B, рост 25%
 *   - `PersistentDataConfig`    — многопоточный, SharedMutexLock, HeapStorage, 16B, рост 25%
 *   - `EmbeddedManagerConfig`   — однопоточный, NoLock, HeapStorage, 16B, рост 50%
 *
 *   --- Industrial DB (высокая нагрузка) ---
 *   - `IndustrialDBConfig`      — многопоточный, SharedMutexLock, HeapStorage, 16B, рост 100%
 *
 * Пример использования:
 * @code
 *   // Кеш-менеджер (однопоточный, 64 МБ)
 *   using AppCache = pmm::PersistMemoryManager<pmm::CacheManagerConfig>;
 *   AppCache::create(64 * 1024 * 1024);
 *   AppCache::pptr<int> ptr = AppCache::allocate_typed<int>();
 *   *ptr = 42;
 *
 *   // Embedded-менеджер с фиксированным пулом 8 КБ (без heap)
 *   using EmbMgr = pmm::PersistMemoryManager<pmm::EmbeddedStaticConfig<8192>>;
 *   EmbMgr::create(8192);
 *   void* p = EmbMgr::allocate(64);
 * @endcode
 *
 * @see persist_memory_manager.h — PersistMemoryManager (Issue #110)
 * @see config.h — базовые политики блокировок (NoLock, SharedMutexLock)
 * @version 0.3 (Issue #146 — переосмысление конфигов: новые архитектурные пресеты и правила)
 */

#pragma once

#include "pmm/address_traits.h"
#include "pmm/config.h"
#include "pmm/free_block_tree.h"
#include "pmm/heap_storage.h"
#include "pmm/static_storage.h"
#include "pmm/storage_backend.h"

namespace pmm
{

// ─── Правила для гранул (Issue #146) ─────────────────────────────────────────

/// @brief Минимальный допустимый размер гранулы (размер слова архитектуры = 4 байта).
inline constexpr std::size_t kMinGranuleSize = 4;

// ─── Embedded / статические конфигурации ─────────────────────────────────────

/**
 * @brief Конфигурация embedded-менеджера со статическим фиксированным буфером.
 *
 * Предназначена для систем без heap (микроконтроллеры, RTOS, bare-metal):
 *   - uint32_t индекс (DefaultAddressTraits), 16-байтная гранула
 *   - StaticStorage<BufferSize> — фиксированный буфер в BSS/глобальной области, нет malloc
 *   - Нет блокировок (NoLock) — только однопоточный контекст
 *   - Не расширяется (StaticStorage::expand() всегда false)
 *
 * Статические проверки (Issue #146):
 *   - granule_size >= kMinGranuleSize (16 >= 4) ✓
 *   - granule_size — степень двойки ✓
 *   - BufferSize кратно granule_size ✓ (проверяется в StaticStorage)
 *
 * Типичный сценарий: встраиваемые системы без heap, Linux bare-metal, фиксированный пул.
 *
 * @tparam BufferSize Размер статического буфера в байтах (кратно 16).
 *                    По умолчанию 4096 байт (4 КБ).
 *
 * @code
 *   using EmbMgr = pmm::PersistMemoryManager<pmm::EmbeddedStaticConfig<8192>>;
 *   EmbMgr::create(8192);
 *   void* ptr = EmbMgr::allocate(64);
 * @endcode
 */
template <std::size_t BufferSize = 4096> struct EmbeddedStaticConfig
{
    static_assert( DefaultAddressTraits::granule_size >= kMinGranuleSize,
                   "EmbeddedStaticConfig: granule_size must be >= kMinGranuleSize (4 bytes)" );
    static_assert( ( DefaultAddressTraits::granule_size & ( DefaultAddressTraits::granule_size - 1 ) ) == 0,
                   "EmbeddedStaticConfig: granule_size must be a power of 2" );

    using address_traits                          = DefaultAddressTraits;
    using storage_backend                         = StaticStorage<BufferSize, DefaultAddressTraits>;
    using free_block_tree                         = AvlFreeTree<DefaultAddressTraits>;
    using lock_policy                             = config::NoLock;
    static constexpr std::size_t granule_size     = DefaultAddressTraits::granule_size;
    static constexpr std::size_t max_memory_gb    = 0; // Нет расширения — StaticStorage
    static constexpr std::size_t grow_numerator   = 3;
    static constexpr std::size_t grow_denominator = 2;
};

// ─── Desktop / динамические конфигурации ─────────────────────────────────────

/**
 * @brief Конфигурация кеш-менеджера (однопоточный, heap, 16B гранула).
 *
 * Оптимизирован для временного кеша с однопоточным доступом:
 *   - Нет блокировок (NoLock) — максимальная производительность
 *   - 16-байтная гранула (DefaultAddressTraits), поддержка до 64 ГБ
 *   - HeapStorage — динамическая память с авторасширением
 *   - Коэффициент роста 5/4 (25%)
 *
 * Статические проверки (Issue #146):
 *   - granule_size >= kMinGranuleSize (16 >= 4) ✓
 *   - granule_size — степень двойки ✓
 *
 * Типичный сценарий: кеш вычислений, временные буферы в однопоточном коде.
 */
struct CacheManagerConfig
{
    static_assert( DefaultAddressTraits::granule_size >= kMinGranuleSize,
                   "CacheManagerConfig: granule_size must be >= kMinGranuleSize (4 bytes)" );
    static_assert( ( DefaultAddressTraits::granule_size & ( DefaultAddressTraits::granule_size - 1 ) ) == 0,
                   "CacheManagerConfig: granule_size must be a power of 2" );

    using address_traits                          = DefaultAddressTraits;
    using storage_backend                         = HeapStorage<DefaultAddressTraits>;
    using free_block_tree                         = AvlFreeTree<DefaultAddressTraits>;
    using lock_policy                             = config::NoLock;
    static constexpr std::size_t granule_size     = 16;
    static constexpr std::size_t max_memory_gb    = 64;
    static constexpr std::size_t grow_numerator   = config::kDefaultGrowNumerator;
    static constexpr std::size_t grow_denominator = config::kDefaultGrowDenominator;
};

/**
 * @brief Конфигурация менеджера персистентных данных (многопоточный, heap, 16B гранула).
 *
 * Оптимизирован для хранения персистентных данных с многопоточным доступом:
 *   - SharedMutexLock — потокобезопасность
 *   - 16-байтная гранула (DefaultAddressTraits), поддержка до 64 ГБ
 *   - HeapStorage — динамическая память
 *   - Коэффициент роста 5/4 (25%)
 *
 * Статические проверки (Issue #146):
 *   - granule_size >= kMinGranuleSize (16 >= 4) ✓
 *   - granule_size — степень двойки ✓
 *
 * Типичный сценарий: долговременное хранение данных, файловые менеджеры.
 */
struct PersistentDataConfig
{
    static_assert( DefaultAddressTraits::granule_size >= kMinGranuleSize,
                   "PersistentDataConfig: granule_size must be >= kMinGranuleSize (4 bytes)" );
    static_assert( ( DefaultAddressTraits::granule_size & ( DefaultAddressTraits::granule_size - 1 ) ) == 0,
                   "PersistentDataConfig: granule_size must be a power of 2" );

    using address_traits                          = DefaultAddressTraits;
    using storage_backend                         = HeapStorage<DefaultAddressTraits>;
    using free_block_tree                         = AvlFreeTree<DefaultAddressTraits>;
    using lock_policy                             = config::SharedMutexLock;
    static constexpr std::size_t granule_size     = 16;
    static constexpr std::size_t max_memory_gb    = 64;
    static constexpr std::size_t grow_numerator   = config::kDefaultGrowNumerator;
    static constexpr std::size_t grow_denominator = config::kDefaultGrowDenominator;
};

/**
 * @brief Конфигурация embedded-менеджера с динамическим хранилищем.
 *
 * Оптимизирован для встраиваемых/ресурсоограниченных систем с heap:
 *   - Нет блокировок (NoLock) — минимальные накладные расходы
 *   - 16-байтная гранула (DefaultAddressTraits), поддержка до 64 ГБ
 *   - HeapStorage — динамическая память
 *   - Консервативный коэффициент роста 3/2 (50%) для экономии памяти
 *
 * Статические проверки (Issue #146):
 *   - granule_size >= kMinGranuleSize (16 >= 4) ✓
 *   - granule_size — степень двойки ✓
 *
 * Типичный сценарий: Linux embedded (RPi, etc.), системы с ограниченной памятью.
 */
struct EmbeddedManagerConfig
{
    static_assert( DefaultAddressTraits::granule_size >= kMinGranuleSize,
                   "EmbeddedManagerConfig: granule_size must be >= kMinGranuleSize (4 bytes)" );
    static_assert( ( DefaultAddressTraits::granule_size & ( DefaultAddressTraits::granule_size - 1 ) ) == 0,
                   "EmbeddedManagerConfig: granule_size must be a power of 2" );

    using address_traits                          = DefaultAddressTraits;
    using storage_backend                         = HeapStorage<DefaultAddressTraits>;
    using free_block_tree                         = AvlFreeTree<DefaultAddressTraits>;
    using lock_policy                             = config::NoLock;
    static constexpr std::size_t granule_size     = 16;
    static constexpr std::size_t max_memory_gb    = 64;
    static constexpr std::size_t grow_numerator   = 3;
    static constexpr std::size_t grow_denominator = 2;
};

// ─── Industrial DB конфигурации ───────────────────────────────────────────────

/**
 * @brief Конфигурация промышленной базы данных (многопоточный, heap, 16B гранула).
 *
 * Оптимизирован для высоконагруженных промышленных систем:
 *   - SharedMutexLock — потокобезопасность с поддержкой конкурентного чтения
 *   - 16-байтная гранула (DefaultAddressTraits), поддержка до 64 ГБ
 *   - HeapStorage — динамическая память
 *   - Агрессивный коэффициент роста 2/1 (100%) для минимизации перевыделений
 *
 * Статические проверки (Issue #146):
 *   - granule_size >= kMinGranuleSize (16 >= 4) ✓
 *   - granule_size — степень двойки ✓
 *
 * Типичный сценарий: промышленные базы данных, time-series хранилища.
 */
struct IndustrialDBConfig
{
    static_assert( DefaultAddressTraits::granule_size >= kMinGranuleSize,
                   "IndustrialDBConfig: granule_size must be >= kMinGranuleSize (4 bytes)" );
    static_assert( ( DefaultAddressTraits::granule_size & ( DefaultAddressTraits::granule_size - 1 ) ) == 0,
                   "IndustrialDBConfig: granule_size must be a power of 2" );

    using address_traits                          = DefaultAddressTraits;
    using storage_backend                         = HeapStorage<DefaultAddressTraits>;
    using free_block_tree                         = AvlFreeTree<DefaultAddressTraits>;
    using lock_policy                             = config::SharedMutexLock;
    static constexpr std::size_t granule_size     = 16;
    static constexpr std::size_t max_memory_gb    = 64;
    static constexpr std::size_t grow_numerator   = 2;
    static constexpr std::size_t grow_denominator = 1;
};

} // namespace pmm
