#pragma once
#include "pmm/address_traits.h"
#include "pmm/storage_backend.h"
#include <cstddef>
#include <cstdint>
#include "pmm/compact_macros.h"
AY Ap{
/*
## pmm-staticstorage
*/
I<H Size,F AT=W>Dt Cn{AK(Size>0,"");AK(Size%AT::P==0,"");DF:j E=AT;Cn()D=GL;Cn(J Cn&)=EO;Cn&Af=(J Cn&)=EO;Cn(Cn&&)=EO;Cn&Af=(Cn&&)=EO;Y*AR()D{B CW;}J Y*AR()J D{B CW;}Q H S()J D{B Size;}
/*
### pmm-staticstorage-expand
*/
AN expand(H)D{B X;}Q AN HT()J D{B X;}Ew:alignas(AT::P)Y CW[Size]{};};AK(Fu<Cn<64>>,"");}
#include "pmm/uncompact_macros.h"
