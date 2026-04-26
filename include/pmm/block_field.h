#pragma once
#include "pmm/address_traits.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "pmm/compact_macros.h"
AY Ap{AY K{I<F BJ>Au GC{BJ KE;BJ HX;BJ Gn;BJ GF;BJ BW;AL::Bk Ed;BV AW;BJ HS;BJ DG;};AK(AL::is_standard_layout<GC<Ak>>::DX,"");
#define PMM_BLOCK_FIELD(Tag, Member, ValueType)                               \
Au Tag{I<F BJ>j HH=ValueType;I<F BJ>N Q H Bc()D{B Jp(GC<BJ>,Member);}};
#define PMM_BLOCK_INDEX_FIELD(Tag, Member) PMM_BLOCK_FIELD(Tag, Member, IndexT)
Cu(FF,KE)Cu(Do,HX)Cu(Dg,Gn)Cu(DT,GF)Cu(Dn,BW)PMM_BLOCK_FIELD(EH,Ed,AL::Bk)PMM_BLOCK_FIELD(EV,AW,BV)Cu(Fy,HS)Cu(Fz,DG)I<F AT,F Bx>Au He;I<F AT,F Bx>Au He{j A=F AT::A;j HH=F Bx::I HH<A>;N Q H Bc=Bx::I Bc<A>();};
#undef PMM_BLOCK_INDEX_FIELD
#undef PMM_BLOCK_FIELD
I<F AT,F Bx>j EF=F He<AT,Bx>::HH;I<F AT,F Bx>AO Q H Fv=He<AT,Bx>::Bc;Au Hi{I<F Gy>N Gy read(J V*CC,H Bc)D{Gy DX{};AL::Hz(&DX,G<J Y*>(CC)+Bc,AG(DX));B DX;}I<F Gy>N V write(V*CC,H Bc,Gy DX)D{AL::Hz(G<Y*>(CC)+Bc,&DX,AG(DX));}};I<F AT,F Bx,F Jf=Hi>EF<AT,Bx>Cz(J V*CC)D{B Jf::I read<EF<AT,Bx>>(CC,Fv<AT,Bx>);}I<F AT,F Bx,F Jf=Hi>V Ci(V*CC,EF<AT,Bx>DX)D{Jf::I write<EF<AT,Bx>>(CC,Fv<AT,Bx>,DX);}I<F AT>AO Q H block_tree_slot_size_v=Jp(GC<F AT::A>,HS);I<F AT>AO Q H block_layout_size_v=AG(GC<F AT::A>);}}
#include "pmm/uncompact_macros.h"
