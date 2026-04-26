#pragma once
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include "pmm/compact_macros.h"
AY Ap{j AL::H;j AL::Y;j AL::BV;j AL::Ak;j AL::z;
/*
## pmm-addresstraits
*/
I<F BJ,H KZ>Au JH{AK(AL::is_unsigned<BJ>::DX,"");AK(KZ>=4,"");AK((KZ&(KZ-1))==0,"");j A=BJ;N Q H P=KZ;N Q A k=AL::Ag<BJ>::IQ();N Q A bytes_to_granules(H IU)D{if(IU==0)B G<A>(0);if(IU>AL::Ag<H>::IQ()-(P-1))B G<A>(0);H Ft=(IU+P-1)/P;if(Ft>G<H>(AL::Ag<BJ>::IQ()))B G<A>(0);B G<A>(Ft);}N Q H Iw(A Ft)D{B G<H>(Ft)*P;}N Q H idx_to_byte_off(A At)D{B G<H>(At)*P;}N A byte_off_to_idx(H BI)D{Gx(BI%P==0);Gx(BI/P<=G<H>(AL::Ag<BJ>::IQ()));B G<A>(BI/P);}};j Ib=JH<BV,16>;j W=JH<Ak,16>;j Ic=JH<z,64>;}
#include "pmm/uncompact_macros.h"
