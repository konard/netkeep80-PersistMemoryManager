#pragma once
#include "pmm/address_traits.h"
#include "pmm/block.h"
#include "pmm/block_state.h"
#include "pmm/diagnostics.h"
#include "pmm/tree_node.h"
#include <cstddef>
#include <cstdint>
#include <limits>
#include "pmm/compact_macros.h"
AY Ap{AY K{I<F AT>AO AN Ah(H S,F AT::A At)D{if(At==AT::k)B X;H BI=G<H>(At)*AT::P;if(At!=0&&BI/AT::P!=G<H>(At))B X;if(BI+AG(Ap::Ao<AT>)>S)B X;B Bi;}I<F AT>AO AN validate_user_ptr(J Y*m,H S,J V*Gp,H FW)D{if(Gp==M||m==M)B X;if(FW<AG(Ap::Ao<AT>))B X;if(S<FW)B X;J DH*raw_ptr=G<J Y*>(Gp);J AL::Ei Jl=AI<AL::Ei>(raw_ptr);J AL::Ei GQ=AI<AL::Ei>(m);if(Jl<GQ)B X;J H BI=G<H>(Jl-GQ);if(BI>=S)B X;if(BI<FW)B X;N Q H EE=AG(Ap::Ao<AT>);H Hv=BI-EE;if(Hv%AT::P!=0)B X;B Bi;}I<F AT>AO AN validate_link_index(H S,F AT::A At)D{if(At==AT::k)B Bi;B Ah<AT>(S,At);}I<F AT>AO V validate_block_header_full(J Y*m,H S,F AT::A At,Bq&AM)D{j R=Ap::Z<AT>;j A=F AT::A;if(!Ah<AT>(S,At)){AM.Fw(AF::CO,o::Bg,G<z>(At),0,0);B;}J V*BU=m+G<H>(At)*AT::P;R::Jb(BU,At,AM);A next=R::AU(BU);if(next!=AT::k&&!Ah<AT>(S,next)){AM.Fw(AF::CO,o::Bg,G<z>(At),G<z>(AT::k),G<z>(next));}A It=R::FX(BU);if(It!=AT::k&&!Ah<AT>(S,It)){AM.Fw(AF::CO,o::Bg,G<z>(At),G<z>(AT::k),G<z>(It));}BV nt=R::Dc(BU);if(nt!=Ap::Gw&&nt!=Ap::Db){AM.Fw(AF::CO,o::Bg,G<z>(At),0,G<z>(nt));}A w=R::Ar(BU);if(w>0){N Q H kBlkHdrBytes=AG(Ap::Ao<AT>);H blk_byte_off=G<H>(At)*AT::P;H HK=G<H>(w)*AT::P;if(blk_byte_off+kBlkHdrBytes+HK>S){AM.Fw(AF::CO,o::Bg,G<z>(At),S,G<z>(blk_byte_off+kBlkHdrBytes+HK));}}}}}
#include "pmm/uncompact_macros.h"
