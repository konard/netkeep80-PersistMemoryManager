#pragma once
#include "pmm/address_traits.h"
#include "pmm/block.h"
#include "pmm/block_field.h"
#include "pmm/diagnostics.h"
#include "pmm/tree_node.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "pmm/compact_macros.h"
AY Ap{I<F AT>Dt DL;I<F AT>Dt Bh;I<F AT>Dt DS;I<F AT>Dt DK;I<F AT>Dt Et;I<F AT>Dt Cw;
/*
## pmm-blockstatebase
*/
I<F AT>Dt Z:Ew Ao<AT>{Ew:j TNode=HP<AT>;DF:j E=AT;j A=F AT::A;j BaseBlock=Ao<AT>;I<F Bx>j JV=K::EF<AT,Bx>;I<F Bx>N Q H Dx=K::Fv<AT,Bx>;I<F Bx>N JV<Bx>Dw(J V*AZ)D{B K::Cz<AT,Bx>(AZ);}I<F Bx>N V Dv(V*AZ,JV<Bx>DX)D{K::Ci<AT,Bx>(AZ,DX);}N AN Ik(J V*AZ)D{B Ar(AZ)==0&&DP(AZ)==0;}N AN JU(J V*AZ,A CJ)D{B Ar(AZ)>0&&DP(AZ)==CJ;}N Q H kOffsetPrevOffset=Dx<K::Fy>;N Q H kOffsetNextOffset=Dx<K::Fz>;N Q H kOffsetWeight=Dx<K::FF>;N Q H kOffsetLeftOffset=Dx<K::Do>;N Q H kOffsetRightOffset=Dx<K::Dg>;N Q H kOffsetParentOffset=Dx<K::DT>;N Q H kOffsetRootOffset=Dx<K::Dn>;N Q H kOffsetAvlHeight=Dx<K::EH>;N Q H kOffsetNodeType=Dx<K::EV>;AK(K::block_tree_slot_size_v<AT> ==AG(TNode),"");AK(K::block_layout_size_v<AT> ==AG(BaseBlock),"");Z()=EO;A KE()J D{B Ar(BN);}A HS()J D{B FX(BN);}A DG()J D{B AU(BN);}A HX()J D{B Ca(BN);}A Gn()J D{B CQ(BN);}A GF()J D{B CA(BN);}AL::Bk Ed()J D{B Fo(BN);}A BW()J D{B DP(BN);}BV AW()J D{B Dc(BN);}
/*
### pmm-blockstatebase-is_free
*/
AN is_free()J D{B Ik(BN);}AN is_allocated(A CJ)J D{B JU(BN,CJ);}AN is_permanently_locked()J D{B AW()==Ap::Db;}
/*
### pmm-blockstatebase-recover_state
*/
N V recover_state(V*AZ,A CJ)D{J A GD=Ar(AZ);J A Hm=DP(AZ);if(GD>0&&Hm!=CJ)FO(AZ,CJ);if(GD==0&&Hm!=0)FO(AZ,0);}
/*
### pmm-blockstatebase-verify_state
*/
N V Jb(J V*AZ,A CJ,Bq&AM)D{J A GD=Ar(AZ);J A Hm=DP(AZ);if(GD>0&&Hm!=CJ){AM.Fw(AF::CO,o::Bg,G<z>(CJ),G<z>(CJ),G<z>(Hm));}if(GD==0&&Hm!=0){AM.Fw(AF::CO,o::Bg,G<z>(CJ),0,G<z>(Hm));}}N V reset_avl_fields_of(V*AZ)D{Bp(AZ,AT::k);Bd(AZ,AT::k);Ay(AZ,AT::k);DI(AZ,0);}N V repair_prev_offset(V*AZ,A Jm)D{CH(AZ,Jm);}N A FX(J V*AZ)D{B Dw<K::Fy>(AZ);}N A AU(J V*AZ)D{B Dw<K::Fz>(AZ);}N A Ar(J V*AZ)D{B Dw<K::FF>(AZ);}N V Fh(V*AZ,A Jm,A FU,AL::Bk avl_height_val,A GD,A root_offset_val)D{CH(AZ,Jm);Cy(AZ,FU);Bp(AZ,AT::k);Bd(AZ,AT::k);Ay(AZ,AT::k);DI(AZ,avl_height_val);FR(AZ,GD);FO(AZ,root_offset_val);}N V Cy(V*AZ,A FU)D{Dv<K::Fz>(AZ,FU);}N A Ca(J V*b)D{B Dw<K::Do>(b);}N A CQ(J V*b)D{B Dw<K::Dg>(b);}N A CA(J V*b)D{B Dw<K::DT>(b);}N A DP(J V*b)D{B Dw<K::Dn>(b);}N V Bp(V*b,A v)D{Dv<K::Do>(b,v);}N V Bd(V*b,A v)D{Dv<K::Dg>(b,v);}N V Ay(V*b,A v)D{Dv<K::DT>(b,v);}N V CH(V*b,A v)D{Dv<K::Fy>(b,v);}N V FR(V*b,A v)D{Dv<K::FF>(b,v);}N V FO(V*b,A v)D{Dv<K::Dn>(b,v);}N AL::Bk Fo(J V*AZ)D{B Dw<K::EH>(AZ);}N V DI(V*AZ,AL::Bk v)D{Dv<K::EH>(AZ,v);}N BV Dc(J V*AZ)D{B Dw<K::EV>(AZ);}N V JN(V*AZ,BV v)D{Dv<K::EV>(AZ,v);}protected:I<F JZ>N JZ*Co(V*CC)D{B AI<JZ*>(CC);}I<F JZ>N J JZ*Co(J V*CC)D{B AI<J JZ*>(CC);}I<F JZ>JZ*GU()D{B AI<JZ*>(BN);}V JM(A v)D{FR(BN,v);}V set_prev_offset(A v)D{CH(BN,v);}V Jk(A v)D{Cy(BN,v);}V set_left_offset(A v)D{Bp(BN,v);}V set_right_offset(A v)D{Bd(BN,v);}V set_parent_offset(A v)D{Ay(BN,v);}V IO(AL::Bk v)D{DI(BN,v);}V Hl(A v)D{FO(BN,v);}V set_node_type(BV v)D{JN(BN,v);}V JP()D{set_left_offset(AT::k);set_right_offset(AT::k);set_parent_offset(AT::k);IO(0);}};AK(AG(Z<W>)==AG(Ao<W>),"");AK(AG(Z<W>)==32,"");
/*
## pmm-freeblock
*/
I<F AT>Dt DL:DF Z<AT>{DF:j Dh=Z<AT>;j A=F AT::A;
/*
### pmm-freeblock-cast_from_raw
*/
N DL*Cm(V*CC)D{if(CC==M)B M;if(!Dh::Ik(CC)){Gx(X&&"cast_from_raw<FreeBlock>: block is not in FreeBlock state");B M;}B Dh::I Co<DL<AT>>(CC);}N J DL*Cm(J V*CC)D{if(CC==M)B M;if(!Dh::Ik(CC)){Gx(X&&"cast_from_raw<FreeBlock>: block is not in FreeBlock state");B M;}B Dh::I Co<DL<AT>>(CC);}
/*
### pmm-freeblock-verify_invariants
*/
AN verify_invariants()J D{B Dh::is_free();}DS<AT>*remove_from_avl()D{B BN->I GU<DS<AT>>();}};
/*
## pmm-freeblockremovedavl
*/
I<F AT>Dt DS:DF Z<AT>{DF:j Dh=Z<AT>;j A=F AT::A;N DS*Cm(V*CC)D{B Dh::I Co<DS<AT>>(CC);}Bh<AT>*mark_as_allocated(A Ej,A CJ)D{Dh::JM(Ej);Dh::Hl(CJ);Dh::JP();B BN->I GU<Bh<AT>>();}Et<AT>*begin_splitting()D{B BN->I GU<Et<AT>>();}DL<AT>*insert_to_avl()D{B BN->I GU<DL<AT>>();}};
/*
## pmm-splittingblock
*/
I<F AT>Dt Et:DF Z<AT>{DF:j Dh=Z<AT>;j A=F AT::A;N Et*Cm(V*CC)D{B Dh::I Co<Et<AT>>(CC);}V initialize_new_block(V*Ii,[[maybe_unused]]A Ey,A CJ)D{AL::Gv(Ii,0,AG(Ao<AT>));Dh::Fh(Ii,CJ,BN->DG(),1,0,0);}V link_new_block(V*Gq,A Ey)D{if(Gq!=M){Dh::CH(Gq,Ey);}Dh::Jk(Ey);}Bh<AT>*finalize_split(A Ej,A CJ)D{Dh::JM(Ej);Dh::Hl(CJ);Dh::JP();B BN->I GU<Bh<AT>>();}};
/*
## pmm-allocatedblock
*/
I<F AT>Dt Bh:DF Z<AT>{DF:j Dh=Z<AT>;j A=F AT::A;
/*
### pmm-allocatedblock-cast_from_raw
*/
N Bh*Cm(V*CC)D{if(CC==M)B M;if(Dh::Ar(CC)==0){Gx(X&&"cast_from_raw<AllocatedBlock>: block is not allocated (weight==0)");B M;}B Dh::I Co<Bh<AT>>(CC);}N J Bh*Cm(J V*CC)D{if(CC==M)B M;if(Dh::Ar(CC)==0){Gx(X&&"cast_from_raw<AllocatedBlock>: block is not allocated (weight==0)");B M;}B Dh::I Co<Bh<AT>>(CC);}
/*
### pmm-allocatedblock-verify_invariants
*/
AN verify_invariants(A CJ)J D{B Dh::is_allocated(CJ);}V*user_ptr()D{B AI<Y*>(BN)+AG(Ao<AT>);}J V*user_ptr()J D{B AI<J Y*>(BN)+AG(Ao<AT>);}DK<AT>*mark_as_free()D{Dh::JM(0);Dh::Hl(0);B BN->I GU<DK<AT>>();}};
/*
## pmm-freeblocknotinavl
*/
I<F AT>Dt DK:DF Z<AT>{DF:j Dh=Z<AT>;j A=F AT::A;N DK*Cm(V*CC)D{B Dh::I Co<DK<AT>>(CC);}Cw<AT>*begin_coalescing()D{B BN->I GU<Cw<AT>>();}DL<AT>*insert_to_avl()D{Dh::IO(1);B BN->I GU<DL<AT>>();}};
/*
## pmm-coalescingblock
*/
I<F AT>Dt Cw:DF Z<AT>{DF:j Dh=Z<AT>;j A=F AT::A;N Cw*Cm(V*CC)D{B Dh::I Co<Cw<AT>>(CC);}V coalesce_with_next(V*Er,V*next_next_blk,A CJ)D{Dh::Jk(Dh::AU(Er));if(next_next_blk!=M){Dh::CH(next_next_blk,CJ);}AL::Gv(Er,0,AG(Ao<AT>));}Cw<AT>*coalesce_with_prev(V*prev_blk,V*Er,A Jm)D{Dh::Cy(prev_blk,Dh::DG());if(Er!=M){Dh::CH(Er,Jm);}AL::Gv(BN,0,AG(Ao<AT>));B Dh::I Co<Cw<AT>>(prev_blk);}DL<AT>*JA()D{Dh::IO(1);B BN->I GU<DL<AT>>();}};I<F AT>int detect_block_state(J V*AZ,F AT::A CJ)D{j R=Z<AT>;if(R::Ik(AZ))B 0;if(R::JU(AZ,CJ))B 1;B-1;}I<F AT>AO V recover_block_state(V*AZ,F AT::A CJ)D{Z<AT>::recover_state(AZ,CJ);}I<F AT>AO V verify_block_state(J V*AZ,F AT::A CJ,Bq&AM)D{Z<AT>::Jb(AZ,CJ,AM);}}
#include "pmm/uncompact_macros.h"
