#pragma once
#include "pmm/avl_tree_mixin.h"
#include "pmm/forest_registry.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "pmm/compact_macros.h"
AY Ap{I<F O>Au Aa;
/*
## pmm-pstringview
*/
I<F O>Au Aa{j Ae=O;j A=F O::A;j GV=F O::I AV<Aa>;Au Bt{j Ae=O;j A=F O::A;j AW=Aa;j Ab=GV;N Q J BG*CP()D{B K::Cd;}N A Cj()D{DH*Ch=O::FQ();B O::BR(Ch);}N A*Cp()D{DH*Ch=O::FQ();B O::Cl(Ch);}N AW*CZ(Ab p)D{B O::I Go<AW>(p);}N int KV(J BG*HQ,Ab Ek)D{if(HQ==M)HQ="";AW*IP=CZ(Ek);B(IP!=M)?AL::KG(HQ,IP->GN()):0;}N AN less_node(Ab lhs,Ab rhs)D{AW*lhs_obj=CZ(lhs);AW*rhs_obj=CZ(rhs);B lhs_obj!=M&&rhs_obj!=M&&AL::KG(lhs_obj->GN(),rhs_obj->GN())<0;}N AN Ip(Ab p)D{B CZ(p)!=M;}};j CE=K::Hy<Bt>;N CE DJ()D{B CE{};}Ak HV;BG str[1];Eh Aa(J BG*s)D:HV(0),str{'\0'}{_interned=_intern(s);}Af GV()J D{B _interned;}J BG*GN()J D{B str;}H IZ()J D{B G<H>(HV);}AN empty()J D{B HV==0;}AN Af==(J BG*s)J D{if(s==M)B HV==0;B AL::KG(GN(),s)==0;}AN Af==(J Aa&An)J D{if(BN==&An)B Bi;if(HV!=An.HV)B X;B AL::KG(str,An.str)==0;}AN Af!=(J BG*s)J D{B!(*BN==s);}AN Af!=(J Aa&An)J D{B!(*BN==An);}AN Af<(J Aa&An)J D{B AL::KG(GN(),An.GN())<0;}
/*
### pmm-pstringview-intern
*/
N GV intern(J BG*s)D{B _intern(s);}N V reset()D{if(!O::Es())B;F O::AP::Aw Eo(O::CY);DJ().reset_root();}N A Cj()D{if(!O::Es())B G<A>(0);F O::AP::Ax Eo(O::CY);B DJ().Cj();}~Aa()=GL;Ew:GV _interned;N GV _intern(J BG*s)D{if(!O::Es())B GV();F O::AP::Aw Eo(O::CY);B O::DO(s);}};}
#include "pmm/uncompact_macros.h"
