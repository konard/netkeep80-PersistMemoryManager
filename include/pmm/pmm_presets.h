#pragma once
#include "pmm/manager_configs.h"
#include "pmm/persist_memory_manager.h"
namespace pmm{namespace presets{template<size_t BufferSize=1024>using SmallEmbeddedStaticHeap=PersistMemoryManager<SmallEmbeddedStaticConfig<BufferSize>,0>;template<size_t BufferSize=4096>using EmbeddedStaticHeap=PersistMemoryManager<EmbeddedStaticConfig<BufferSize>,0>;using EmbeddedHeap=PersistMemoryManager<EmbeddedManagerConfig,0>;using SingleThreadedHeap=PersistMemoryManager<CacheManagerConfig,0>;using MultiThreadedHeap=PersistMemoryManager<PersistentDataConfig,0>;using IndustrialDBHeap=PersistMemoryManager<IndustrialDBConfig,0>;using LargeDBHeap=PersistMemoryManager<LargeDBConfig,0>;}}
