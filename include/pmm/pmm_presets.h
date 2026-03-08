/**
 * @file pmm/pmm_presets.h
 * @brief pmm_presets — готовые инстанции PersistMemoryManager (Issue #87 Phase 8, #110, #123).
 *
 * Предоставляет набор готовых псевдонимов для наиболее распространённых
 * конфигураций менеджера персистентной памяти:
 *
 *   - `SingleThreadedHeap`  — 32-bit, NoLock, HeapStorage (однопоточные приложения)
 *   - `MultiThreadedHeap`   — 32-bit, SharedMutexLock, HeapStorage (многопоточные)
 *   - `EmbeddedHeap`        — 32-bit, NoLock, HeapStorage, рост 50% (встраиваемые системы)
 *   - `IndustrialDBHeap`    — 32-bit, SharedMutexLock, HeapStorage, рост 100% (промышленные БД)
 *
 * Использует унифицированный `PersistMemoryManager<ConfigT, InstanceId>` (Issue #110).
 *
 * @see persist_memory_manager.h — PersistMemoryManager (Issue #110)
 * @see manager_configs.h — готовые конфигурации менеджеров
 * @version 0.3 (Issue #123 — добавлены EmbeddedHeap и IndustrialDBHeap)
 */

#pragma once

#include "pmm/manager_configs.h"
#include "pmm/persist_memory_manager.h"

namespace pmm
{
namespace presets
{

/**
 * @brief Стандартный однопоточный динамический менеджер.
 *
 * - 32-bit адресация, 16-байтная гранула
 * - Динамическая память через HeapStorage (std::malloc)
 * - Без блокировок (для однопоточных приложений)
 * - Коэффициент роста 5/4 (25%)
 * - InstanceId=0 (по умолчанию)
 *
 * Использование:
 * @code
 *   pmm::presets::SingleThreadedHeap::create(64 * 1024); // 64 KiB начальный размер
 *   void* ptr = pmm::presets::SingleThreadedHeap::allocate(256);
 *   pmm::presets::SingleThreadedHeap::deallocate(ptr);
 * @endcode
 */
using SingleThreadedHeap = PersistMemoryManager<CacheManagerConfig, 0>;

/**
 * @brief Стандартный многопоточный динамический менеджер.
 *
 * - 32-bit адресация, 16-байтная гранула
 * - Динамическая память через HeapStorage
 * - Блокировки через std::shared_mutex
 * - Коэффициент роста 5/4 (25%)
 * - InstanceId=0 (по умолчанию)
 *
 * Использование:
 * @code
 *   pmm::presets::MultiThreadedHeap::create(64 * 1024);
 *   void* ptr = pmm::presets::MultiThreadedHeap::allocate(256);
 *   pmm::presets::MultiThreadedHeap::deallocate(ptr);
 * @endcode
 */
using MultiThreadedHeap = PersistMemoryManager<PersistentDataConfig, 0>;

/**
 * @brief Менеджер для встраиваемых систем с ограниченными ресурсами.
 *
 * - 32-bit адресация, 16-байтная гранула
 * - Динамическая память через HeapStorage
 * - Без блокировок (однопоточный доступ)
 * - Консервативный коэффициент роста 3/2 (50%) для экономии памяти
 * - InstanceId=0 (по умолчанию)
 *
 * Использование:
 * @code
 *   pmm::presets::EmbeddedHeap::create(4 * 1024); // 4 KiB начальный размер
 *   void* ptr = pmm::presets::EmbeddedHeap::allocate(64);
 *   pmm::presets::EmbeddedHeap::deallocate(ptr);
 * @endcode
 */
using EmbeddedHeap = PersistMemoryManager<EmbeddedManagerConfig, 0>;

/**
 * @brief Менеджер для промышленных баз данных с высокой нагрузкой.
 *
 * - 32-bit адресация, 16-байтная гранула
 * - Динамическая память через HeapStorage
 * - Блокировки через std::shared_mutex
 * - Агрессивный коэффициент роста 2/1 (100%) для минимизации перевыделений
 * - InstanceId=0 (по умолчанию)
 *
 * Использование:
 * @code
 *   pmm::presets::IndustrialDBHeap::create(256 * 1024 * 1024); // 256 MiB начальный размер
 *   void* ptr = pmm::presets::IndustrialDBHeap::allocate(4096);
 *   pmm::presets::IndustrialDBHeap::deallocate(ptr);
 * @endcode
 */
using IndustrialDBHeap = PersistMemoryManager<IndustrialDBConfig, 0>;

} // namespace presets
} // namespace pmm
