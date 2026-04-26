#pragma once
#include "pmm/avl_tree_mixin.h"
#include "pmm/block.h"
#include "pmm/block_state.h"
#include "pmm/types.h"
#include <concepts>
#include <cstdint>
#include <type_traits>
#include "pmm/compact_macros.h"
AY Ap{I<F Policy,F AT>Hc EU=Eg(Y*m,K::q<AT>*AD,F AT::A At){{Policy::GZ(m,AD,At)};{Policy::JQ(m,AD,At)};{Policy::GI(m,AD,At)}->AL::Bw<F AT::A>;};
/*
## pmm-avlfreetree
*/
I<F AT=W>Au EA{j E=AT;j A=F AT::A;j GB=Ao<AT>;j R=Z<AT>;j BPPtr=K::GS<AT>;N Q J BG*kForestDomainName="system/free_tree";EA()=EO;EA(J EA&)=EO;EA&Af=(J EA&)=EO;N V GZ(Y*m,K::q<AT>*AD,A As){V*EK=K::Ac<AT>(m,As);R::Bp(EK,AT::k);R::Bd(EK,AT::k);R::Ay(EK,AT::k);R::DI(EK,1);if(AD->Am==AT::k){AD->Am=As;B;}A EC=K::Fi<AT>(AD->S);A Jx=R::AU(EK);A blk_gran=(Jx!=AT::k)?(Jx-As):(EC-As);A Ek=AD->Am,BX=AT::k;AN go_left=X;Fa(Ek!=AT::k){BX=Ek;J V*n=K::Ac<AT>(m,Ek);A n_next=R::AU(n);A n_gran=(n_next!=AT::k)?(n_next-Ek):(EC-Ek);AN smaller=(blk_gran<n_gran)||(blk_gran==n_gran&&As<Ek);go_left=smaller;Ek=smaller?R::Ca(n):R::CQ(n);}R::Ay(EK,BX);if(go_left)R::Bp(K::Ac<AT>(m,BX),As);EN R::Bd(K::Ac<AT>(m,BX),As);K::DA(BPPtr(m,BX),AD->Am);}N V JQ(Y*m,K::q<AT>*AD,A As){V*EK=K::Ac<AT>(m,As);A BX=R::CA(EK);A Hh=R::Ca(EK);A GE=R::CQ(EK);A rebal=AT::k;if(Hh==AT::k&&GE==AT::k){KI(m,AD,BX,As,AT::k);rebal=BX;}EN if(Hh==AT::k||GE==AT::k){A child=(Hh!=AT::k)?Hh:GE;R::Ay(K::Ac<AT>(m,child),BX);KI(m,AD,BX,As,child);rebal=BX;}EN{BPPtr succ=K::IC(BPPtr(m,GE));A Hk=succ.Bc();V*Jj=K::Ac<AT>(m,Hk);A Ig=R::CA(Jj);A succ_right=R::CQ(Jj);if(Ig!=As){KI(m,AD,Ig,Hk,succ_right);if(succ_right!=AT::k)R::Ay(K::Ac<AT>(m,succ_right),Ig);R::Bd(Jj,GE);R::Ay(K::Ac<AT>(m,GE),Hk);rebal=Ig;}EN{rebal=Hk;}R::Bp(Jj,Hh);R::Ay(K::Ac<AT>(m,Hh),Hk);R::Ay(Jj,BX);KI(m,AD,BX,As,Hk);K::JD(BPPtr(m,Hk));}R::Bp(EK,AT::k);R::Bd(EK,AT::k);R::Ay(EK,AT::k);R::DI(EK,0);K::DA(BPPtr(m,rebal),AD->Am);}
/*
### pmm-avlfreetree-find_best_fit
*/
N A GI(Y*m,K::q<AT>*AD,A needed_granules){A EC=K::Fi<AT>(AD->S);A Ek=AD->Am,AM=AT::k;Fa(Ek!=AT::k){J V*Iu=K::Ac<AT>(m,Ek);A node_next=R::AU(Iu);A cur_gran=(node_next!=AT::k)?(node_next-Ek):(EC-Ek);if(cur_gran>=needed_granules){AM=Ek;Ek=R::Ca(Iu);}EN{Ek=R::CQ(Iu);}}B AM;}Ew:N V KI(Y*m,K::q<AT>*AD,A BX,A old_child,A HC){if(BX==AT::k){AD->Am=HC;B;}V*p=K::Ac<AT>(m,BX);if(R::Ca(p)==old_child)R::Bp(p,HC);EN R::Bd(p,HC);}};AK(EU<EA<W>,W>,"");}
#include "pmm/uncompact_macros.h"
