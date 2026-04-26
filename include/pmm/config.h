#pragma once
#include <mutex>
#include <shared_mutex>
#include "pmm/compact_macros.h"
AY Ap{AY Gb{
/*
### pmm-config-sharedmutexlock
*/
Au Hx{j JS=AL::shared_mutex;j Ax=AL::shared_lock<AL::shared_mutex>;j Aw=AL::unique_lock<AL::shared_mutex>;};
/*
### pmm-config-nolock
*/
Au NoLock{Au JS{V Eo(){}V unlock(){}V lock_shared(){}V unlock_shared(){}AN try_lock(){B Bi;}AN try_lock_shared(){B Bi;}};Au Ax{Eh Ax(JS&){}};Au Aw{Eh Aw(JS&){}};};AO Q H Fe=5;AO Q H FG=4;}}
#include "pmm/uncompact_macros.h"
