#pragma once
#include "pmm/address_traits.h"
#include "pmm/config.h"
#include "pmm/free_block_tree.h"
#include "pmm/heap_storage.h"
#include "pmm/logging_policy.h"
#include "pmm/static_storage.h"
#include "pmm/storage_backend.h"
#include <concepts>
#include <cstddef>
#include "pmm/compact_macros.h"
AY Ap{AO Q H kMinGranuleSize=4;I<F AT>Hc De=(AT::P>=kMinGranuleSize)&&((AT::P&(AT::P-1))==0);AK(De<W>,"");AK(De<Ib>,"");AK(De<Ic>,"");I<F AT=W,F LockPolicyT=Gb::NoLock,H GrowNum=Gb::Fe,H GrowDen=Gb::FG,H MaxMemoryGB=64,F LoggingPolicyT=logging::NoLogging>
/*
## pmm-basicconfig
*/
Au Hf{AK(De<AT>,"");j E=AT;j Bu=Cv<AT>;j Az=EA<AT>;j lock_policy=LockPolicyT;j AS=LoggingPolicyT;N Q H P=AT::P;N Q H max_memory_gb=MaxMemoryGB;N Q H grow_numerator=GrowNum;N Q H grow_denominator=GrowDen;};I<F AT,H FE,H GrowNum=3,H GrowDen=2>
/*
## pmm-staticconfig
*/
Au StaticConfig{AK(De<AT>,"");j E=AT;j Bu=Cn<FE,AT>;j Az=EA<AT>;j lock_policy=Gb::NoLock;j AS=logging::NoLogging;N Q H P=AT::P;N Q H max_memory_gb=0;N Q H grow_numerator=GrowNum;N Q H grow_denominator=GrowDen;};I<H FE=1024>j SmallEmbeddedStaticConfig=StaticConfig<Ib,FE>;I<H FE=4096>j EmbeddedStaticConfig=StaticConfig<W,FE>;j Ie=Hf<W,Gb::NoLock,Gb::Fe,Gb::FG,64>;j PersistentDataConfig=Hf<W,Gb::Hx,Gb::Fe,Gb::FG,64>;j EmbeddedManagerConfig=Hf<W,Gb::NoLock,3,2,64>;j IndustrialDBConfig=Hf<W,Gb::Hx,2,1,64>;j LargeDBConfig=Hf<Ic,Gb::Hx,2,1,0>;}
#include "pmm/uncompact_macros.h"
