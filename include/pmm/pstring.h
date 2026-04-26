#pragma once
#include "pmm/types.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "pmm/compact_macros.h"
AY Ap{
/*
## pmm-pstring
*/
I<F O>Au Is{j Ae=O;j A=F O::A;Ak ET;Ak ER;A CB;Is()D:ET(0),ER(0),CB(K::Dl<F O::E>){}~Is()D=GL;J BG*GN()J D{if(CB==K::Dl<F O::E>)B "";BG*Dm=BO();B(Dm!=M)?Dm:"";}H IZ()J D{B G<H>(ET);}AN empty()J D{B ET==0;}BG Af[](H i)J D{BG*Dm=BO();B(Dm!=M)?Dm[i]:'\0';}AN assign(J BG*s)D{if(s==M)s="";DH len=G<Ak>(AL::strlen(s));if(!Ds(len))B X;BG*Dm=BO();if(Dm==M)B X;AL::Hz(Dm,s,G<H>(len)+1);ET=len;B Bi;}AN append(J BG*s)D{if(s==M)s="";DH add_len=G<Ak>(AL::strlen(s));if(add_len==0)B Bi;Ak new_len=ET+add_len;if(new_len<ET)B X;if(!Ds(new_len))B X;BG*Dm=BO();if(Dm==M)B X;AL::Hz(Dm+ET,s,G<H>(add_len)+1);ET=new_len;B Bi;}V clear()D{ET=0;if(CB!=K::Dl<F O::E>){BG*Dm=BO();if(Dm!=M)Dm[0]='\0';}}V free_data()D{if(CB!=K::Dl<F O::E>){O::Ec(K::DM<F O::E>(O::EG().AR(),CB));CB=K::Dl<F O::E>;}ET=0;ER=0;}AN Af==(J BG*s)J D{if(s==M)B ET==0;B AL::KG(GN(),s)==0;}AN Af!=(J BG*s)J D{B!(*BN==s);}AN Af==(J Is&An)J D{if(BN==&An)B Bi;if(ET!=An.ET)B X;if(ET==0)B Bi;B AL::KG(GN(),An.GN())==0;}AN Af!=(J Is&An)J D{B!(*BN==An);}AN Af<(J Is&An)J D{B AL::KG(GN(),An.GN())<0;}Ew:BG*BO()J D{B AI<BG*>(K::DM<F O::E>(O::EG().AR(),CB));}AN Ds(Ak Hn)D{if(Hn<=ER)B Bi;Ak FA=ER*2;if(FA<Hn)FA=Hn;if(FA<16)FA=16;H HN=G<H>(FA)+1;V*GG=O::HM(HN);if(GG==M)B X;Y*m=O::EG().AR();A KM=K::Gh<F O::E>(m,GG);if(ET>0&&CB!=K::Dl<F O::E>){BG*Jo=BO();if(Jo!=M)AL::Hz(GG,Jo,G<H>(ET)+1);}EN{G<BG*>(GG)[0]='\0';}if(CB!=K::Dl<F O::E>)O::Ec(K::DM<F O::E>(m,CB));CB=KM;ER=FA;B Bi;}};}
#include "pmm/uncompact_macros.h"
