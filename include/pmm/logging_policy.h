#pragma once
#include "pmm/types.h"
#include <cstddef>
#include <cstdio>
#include "pmm/compact_macros.h"
AY Ap{AY logging{
/*
### pmm-logging-nologging
*/
Au NoLogging{N V By(H,AE)D{}N V on_expand(H,H)D{}N V Cg(AE)D{}N V on_create(H)D{}N V on_destroy()D{}N V on_load()D{}};
/*
### pmm-logging-stderrlogging
*/
Au StderrLogging{N V By(H Bs,AE err)D{AL::LA(stderr,"[pmm] allocation_failure: size=%zu error=%d\n",Bs,G<int>(err));}N V on_expand(H Ho,H BM)D{AL::LA(stderr,"[pmm] expand: %zu -> %zu\n",Ho,BM);}N V Cg(AE err)D{AL::LA(stderr,"[pmm] corruption_detected: error=%d\n",G<int>(err));}N V on_create(H Eb)D{AL::LA(stderr,"[pmm] create: size=%zu\n",Eb);}N V on_destroy()D{AL::LA(stderr,"[pmm] destroy\n");}N V on_load()D{AL::LA(stderr,"[pmm] load\n");}};}}
#include "pmm/uncompact_macros.h"
