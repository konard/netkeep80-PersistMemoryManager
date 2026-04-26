#pragma once
#include "pmm/manager_configs.h"
#include "pmm/persist_memory_manager.h"
#include "pmm/compact_macros.h"
AY Ap{AY presets{I<H FE=1024>j SmallEmbeddedStaticHeap=Bm<SmallEmbeddedStaticConfig<FE>,0>;I<H FE=4096>j EmbeddedStaticHeap=Bm<EmbeddedStaticConfig<FE>,0>;j EmbeddedHeap=Bm<EmbeddedManagerConfig,0>;j SingleThreadedHeap=Bm<Ie,0>;j MultiThreadedHeap=Bm<PersistentDataConfig,0>;j IndustrialDBHeap=Bm<IndustrialDBConfig,0>;j LargeDBHeap=Bm<LargeDBConfig,0>;}}
#include "pmm/uncompact_macros.h"
