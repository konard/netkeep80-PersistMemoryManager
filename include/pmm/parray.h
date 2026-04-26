#pragma once
#include "pmm/types.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "pmm/compact_macros.h"
AY Ap{
/*
## pmm-parray
*/
I<F T,F O>Au parray{AK(AL::FH<T>,"");j Ae=O;j A=F O::A;j HH=T;Ak Al;Ak ER;A CB;parray()D:Al(0),ER(0),CB(K::Dl<F O::E>){}~parray()D=GL;H IZ()J D{B G<H>(Al);}AN empty()J D{B Al==0;}H capacity()J D{B G<H>(ER);}T*at(H i)D{if(i>=G<H>(Al))B M;T*Dm=BO();B(Dm!=M)?(Dm+i):M;}J T*at(H i)J D{if(i>=G<H>(Al))B M;J T*Dm=BO();B(Dm!=M)?(Dm+i):M;}T Af[](H i)J D{J T*Dm=BO();B(Dm!=M)?Dm[i]:T{};}T*front()D{B at(0);}J T*front()J D{B at(0);}T*back()D{B(Al>0)?at(G<H>(Al)-1):M;}J T*back()J D{B(Al>0)?at(G<H>(Al)-1):M;}T*Dm()D{B BO();}J T*Dm()J D{B BO();}AN push_back(J T&DX)D{if(!Ds(Al+1))B X;T*d=BO();if(d==M)B X;d[Al]=DX;++Al;B Bi;}V pop_back()D{if(Al>0)--Al;}AN set(H i,J T&DX)D{if(i>=G<H>(Al))B X;T*d=BO();if(d==M)B X;d[i]=DX;B Bi;}AN reserve(H n)D{if(n>G<H>(AL::Ag<Ak>::IQ()))B X;B Ds(G<Ak>(n));}AN resize(H n)D{if(n>G<H>(AL::Ag<Ak>::IQ()))B X;DH BM=G<Ak>(n);if(BM>Al){if(!Ds(BM))B X;T*d=BO();if(d==M)B X;AL::Gv(d+Al,0,G<H>(BM-Al)*AG(T));}Al=BM;B Bi;}AN GZ(H Ha,J T&DX)D{if(Ha>G<H>(Al))B X;if(!Ds(Al+1))B X;T*d=BO();if(d==M)B X;if(Ha<G<H>(Al))AL::memmove(d+Ha+1,d+Ha,(G<H>(Al)-Ha)*AG(T));d[Ha]=DX;++Al;B Bi;}AN erase(H Ha)D{if(Ha>=G<H>(Al))B X;T*d=BO();if(d==M)B X;if(Ha+1<G<H>(Al))AL::memmove(d+Ha,d+Ha+1,(G<H>(Al)-Ha-1)*AG(T));--Al;B Bi;}V clear()D{Al=0;}V free_data()D{if(CB!=K::Dl<F O::E>){O::Ec(K::DM<F O::E>(O::EG().AR(),CB));CB=K::Dl<F O::E>;}Al=0;ER=0;}AN Af==(J parray&An)J D{if(BN==&An)B Bi;if(Al!=An.Al)B X;if(Al==0)B Bi;J T*a=BO();J T*b=An.BO();if(a==M||b==M)B(a==b);B AL::memcmp(a,b,G<H>(Al)*AG(T))==0;}AN Af!=(J parray&An)J D{B!(*BN==An);}Ew:T*BO()J D{B AI<T*>(K::DM<F O::E>(O::EG().AR(),CB));}AN Ds(Ak Hn)D{if(Hn<=ER)B Bi;Ak FA=ER*2;if(FA<Hn)FA=Hn;if(FA<4)FA=4;H HN=G<H>(FA)*AG(T);if(AG(T)>0&&HN/AG(T)!=G<H>(FA))B X;V*GG=O::HM(HN);if(GG==M)B X;Y*m=O::EG().AR();A KM=K::Gh<F O::E>(m,GG);if(Al>0&&CB!=K::Dl<F O::E>){T*Jo=BO();if(Jo!=M)AL::Hz(GG,Jo,G<H>(Al)*AG(T));}if(CB!=K::Dl<F O::E>)O::Ec(K::DM<F O::E>(m,CB));CB=KM;ER=FA;B Bi;}};}
#include "pmm/uncompact_macros.h"
