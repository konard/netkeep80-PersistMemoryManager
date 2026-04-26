#pragma once
#include "pmm/address_traits.h"
#include "pmm/block.h"
#include "pmm/block_state.h"
#include "pmm/tree_node.h"
#include "pmm/validation.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include "pmm/compact_macros.h"
AY Ap{
/*
## pmm-pmmerror
*/
enum Dt AE:Y{Ok=0,IX=1,Gd=2,Io=3,Dy=4,ExpandFailed=5,InvalidMagic=6,CrcMismatch=7,SizeMismatch=8,KB=9,Ee=10,CX=11,BlockLocked=12,Gt=13,};AO Q H Ea=16;AK((Ea&(Ea-1))==0,"");AK(Ea==Ap::W::P,"");AO Q z JT=0x504D4D5F56303938ULL;
/*
## pmm-memorystats
*/
Au MemoryStats{H total_blocks;H free_blocks;H allocated_blocks;H largest_free;H smallest_free;H total_fragmentation;};Au ManagerInfo{z HD;H S;H Bb;H BP;H Be;H Cf;AL::EP Aq;AL::EP first_free_offset;H manager_header_size;};
/*
## pmm-blockview
*/
Au BlockView{H Ha;AL::EP Bc;H S;H header_size;H Bs;H alignment;AN used;};
/*
## pmm-freeblockview
*/
Au FreeBlockView{AL::EP Bc;H S;H free_size;AL::EP HX;AL::EP Gn;AL::EP GF;int Ed;int avl_depth;};AY K{AO Q Y FM=0;AO Q Y DC=1;AO Q AN GA(Y CK)D{B CK==FM||CK==DC;}AO Q AN HR(Y CK)D{B CK==FM;}AO Ak EX(Ak crc,Y byte)D{crc^=byte;for(int bit=0;bit<8;++bit)crc=(crc>>1)^(0xEDB88320U&(~(crc&1U)+1U));B crc;}AO Ak compute_crc32(J Y*Dm,H HV)D{Ak crc=0xFFFFFFFFU;for(H i=0;i<HV;++i)crc=EX(crc,Dm[i]);B crc^0xFFFFFFFFU;}AK(AG(Ap::Ao<Ap::W>)==32,"");AK(AG(Ap::Ao<Ap::W>)%Ea==0,"");AK(AG(Ap::Ao<Ap::W>)==AG(Ap::HP<Ap::W>)+2*AG(Ak),"");AK(AG(Ap::HP<Ap::W>)==5*AG(Ak)+4,"");AO Q Ak kNoBlock=0xFFFFFFFFU;AK(kNoBlock==Ap::W::k,"");I<F AT>AO Q F AT::A kNoBlock_v=AT::k;I<F AT>AO Q F AT::A Dl=G<F AT::A>(0);
/*
### pmm-detail-managerheader
*/
I<F AT=W>Au q{j A=F AT::A;z HD;z S;A Bb;A BP;A Be;A Cf;A Aq;A BB;A Am;AN HT;Y CK;BV P;z prev_total_size;Ak crc32;A BW;};AK(AG(q<W>)==64,"");AK(AG(q<W>)%Ea==0,"");I<F AT>AO Q F AT::A Bo=G<F AT::A>((AG(Ap::Ao<AT>)+AT::P-1)/AT::P);I<F AT>AO Q H Bf=G<H>(Bo<AT>)*AT::P;I<F AT>AO q<AT>*Dq(Y*m)D{B AI<q<AT>*>(m+Bf<AT>);}I<F AT>AO J q<AT>*Dq(J Y*m)D{B AI<J q<AT>*>(m+Bf<AT>);}I<F AT>AO Ak IH(J Y*Dm,H HV)D{Q H kHdrOffset=Bf<AT>;Q H kCrcOffset=kHdrOffset+Jp(q<AT>,crc32);Q H kCrcSize=AG(Ak);Q H kAfterCrc=kCrcOffset+kCrcSize;Ak crc=0xFFFFFFFFU;for(H i=0;i<kCrcOffset&&i<HV;++i)crc=EX(crc,Dm[i]);for(H i=0;i<kCrcSize;++i)crc=EX(crc,0x00U);for(H i=kAfterCrc;i<HV;++i)crc=EX(crc,Dm[i]);B crc^0xFFFFFFFFU;}AO Q Ak kManagerHeaderGranules=AG(q<W>)/Ea;AO Q H kMinBlockSize=AG(Ap::Ao<Ap::W>)+Ea;AO Q H Fm=AG(Ap::Ao<Ap::W>)+AG(q<Ap::W>)+AG(Ap::Ao<Ap::W>)+kMinBlockSize;I<F AT>AO F AT::A DR(H IU){j BJ=F AT::A;N Q H BH=AT::P;if(IU>AL::Ag<H>::IQ()-(BH-1))B G<BJ>(0);H Ft=(IU+BH-1)/BH;if(Ft>G<H>(AL::Ag<BJ>::IQ()))B G<BJ>(0);B G<BJ>(Ft);}I<F AT>AO F AT::A bytes_to_idx_t(H IU){N Q H BH=AT::P;j BJ=F AT::A;if(IU==0)B G<BJ>(0);if(IU>AL::Ag<H>::IQ()-(BH-1))B AT::k;H Ft=(IU+BH-1)/BH;if(Ft>G<H>(AL::Ag<BJ>::IQ()))B AT::k;B G<BJ>(Ft);}I<F AT>AO H idx_to_byte_off_t(F AT::A At){B G<H>(At)*AT::P;}I<F AT>AO F AT::A Fi(H BI){j BJ=F AT::A;Gx(BI%AT::P==0);Gx(BI/AT::P<=G<H>(AL::Ag<BJ>::IQ()));B G<BJ>(BI/AT::P);}AO AN is_valid_alignment(H align){B align==Ea;}I<F AT=Ap::W>AO Ap::Ao<AT>*Ac(Y*m,F AT::A At){Gx(At!=kNoBlock_v<AT>);B AI<Ap::Ao<AT>*>(m+G<H>(At)*AT::P);}I<F AT=Ap::W>AO J Ap::Ao<AT>*Ac(J Y*m,F AT::A At){Gx(At!=kNoBlock_v<AT>);B AI<J Ap::Ao<AT>*>(m+G<H>(At)*AT::P);}I<F AT=Ap::W>AO Ap::Ao<AT>*block_at_checked(Y*m,H S,F AT::A At)D{if(!Ah<AT>(S,At))B M;B AI<Ap::Ao<AT>*>(m+G<H>(At)*AT::P);}I<F AT=Ap::W>AO J Ap::Ao<AT>*block_at_checked(J Y*m,H S,F AT::A At)D{if(!Ah<AT>(S,At))B M;B AI<J Ap::Ao<AT>*>(m+G<H>(At)*AT::P);}I<F AT>AO F AT::A KW(J Y*m,J Ap::Ao<AT>*block){H BI=AI<J Y*>(block)-m;Gx(BI%AT::P==0);B G<F AT::A>(BI/AT::P);}I<F AT>AO Q F AT::A kManagerHeaderGranules_t=G<F AT::A>((AG(q<AT>)+AT::P-1)/AT::P);I<F AT>AO F AT::A En(J Y*m,J q<AT>*AD,J Ap::Ao<AT>*EK){j R=Ap::Z<AT>;N Q H BH=AT::P;j BJ=F AT::A;N Q BJ kNoBlk=AT::k;H BI=AI<J Y*>(EK)-m;BJ this_idx=G<BJ>(BI/BH);BJ next_off=R::AU(EK);BJ EC=G<BJ>(AD->S/BH);if(next_off!=kNoBlk)B G<BJ>(next_off-this_idx);B G<BJ>(EC-this_idx);}I<F AT>AO V*DM(Y*m,F AT::A At)D{B(At==G<F AT::A>(0))?M:m+G<H>(At)*AT::P;}I<F AT>AO V*resolve_granule_ptr_checked(Y*m,H S,F AT::A At)D{if(At==G<F AT::A>(0))B M;H BI=G<H>(At)*AT::P;if(BI>=S)B M;B m+BI;}I<F AT>AO F AT::A Gh(J Y*m,J V*Gp)D{B G<F AT::A>((G<J Y*>(Gp)-m)/AT::P);}I<F AT>AO F AT::A ptr_to_granule_idx_checked(J Y*m,H S,J V*Gp)D{j BJ=F AT::A;if(Gp==M||m==M)B AT::k;J DH*CC=G<J Y*>(Gp);if(CC<m||CC>=m+S)B AT::k;H BI=G<H>(CC-m);if(BI%AT::P!=0)B AT::k;H At=BI/AT::P;if(At>G<H>(AL::Ag<BJ>::IQ()))B AT::k;B G<BJ>(At);}I<F AT=Ap::W>AO V*user_ptr(Ap::Ao<AT>*block){B AI<Y*>(block)+AG(Ap::Ao<AT>);}I<F AT>AO AN Fg(J Y*m,J q<AT>*AD,H S,F AT::A FY)D{j R=Ap::Z<AT>;j BJ=F AT::A;if(m==M||AD==M)B X;if(AD->BP==0||AD->Aq==AT::k)B X;if(!Ah<AT>(S,AD->Aq)||!Ah<AT>(S,AD->BB))B X;J V*cand=m+G<H>(FY)*AT::P;J BJ It=R::FX(cand);J BJ next=R::AU(cand);if(It==AT::k){if(FY!=AD->Aq)B X;}EN{if(!Ah<AT>(S,It)||It>=FY)B X;J V*prev_block=m+G<H>(It)*AT::P;if(R::AU(prev_block)!=FY)B X;}if(next==AT::k){if(FY!=AD->BB)B X;}EN{if(!Ah<AT>(S,next)||next<=FY)B X;J V*next_block=m+G<H>(next)*AT::P;if(R::FX(next_block)!=FY)B X;}B Bi;}I<F AT>AO AN ES(J Y*m,H S,J Y*FL)D{j R=Ap::Z<AT>;j BJ=F AT::A;if(m==M||FL==M)B X;J AL::Ei GQ=AI<AL::Ei>(m);J AL::Ei cand_raw=AI<AL::Ei>(FL);if(cand_raw<GQ)B X;J H Hv=G<H>(cand_raw-GQ);if(Hv%AT::P!=0)B X;if(Hv/AT::P>G<H>(AL::Ag<BJ>::IQ()))B X;J BJ FY=G<BJ>(Hv/AT::P);if(!Ah<AT>(S,FY))B X;J BJ KE=R::Ar(FL);if(KE==0||R::DP(FL)!=FY)B X;J BV AW=R::Dc(FL);if(AW!=Ap::Gw&&AW!=Ap::Db)B X;if(S<Bf<AT>+AG(q<AT>))B X;J DH*AD=Dq<AT>(m);if(AD->S!=S)B X;if(!Fg<AT>(m,AD,S,FY))B X;Q H EE=AG(Ap::Ao<AT>);if(Hv>S-EE)B X;J H data_start=Hv+EE;if(G<H>(KE)>(AL::Ag<H>::IQ)()/AT::P)B X;J H HK=G<H>(KE)*AT::P;if(HK>S-data_start)B X;B Bi;}I<F AT>AO AN is_canonical_user_ptr(J Y*m,H S,J V*Gp)D{Q H EE=AG(Ap::Ao<AT>);J H FW=EE+AG(q<AT>)+EE;if(!validate_user_ptr<AT>(m,S,Gp,FW))B X;J DH*raw_ptr=G<J Y*>(Gp);J DH*FL=raw_ptr-EE;if(FL+EE!=raw_ptr)B X;B ES<AT>(m,S,FL);}I<F AT>AO Ap::Ao<AT>*header_from_ptr_t(Y*m,V*Gp,H S){N Q H EE=AG(Ap::Ao<AT>);if(!is_canonical_user_ptr<AT>(m,S,Gp))B M;Y*FL=G<Y*>(Gp)-EE;B AI<Ap::Ao<AT>*>(FL);}I<F AT>AO F AT::A required_block_granules_t(H user_bytes){j A=F AT::A;A Ej=DR<AT>(user_bytes);if(Ej==0)Ej=1;B Bo<AT>+Ej;}}}
#include "pmm/uncompact_macros.h"
