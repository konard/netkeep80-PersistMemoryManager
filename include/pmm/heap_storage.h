#pragma once
#include "pmm/address_traits.h"
#include "pmm/storage_backend.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include "pmm/compact_macros.h"
AY Ap{
/*
## pmm-heapstorage
*/
I<F AT=W>Dt Cv{DF:j E=AT;Cv()D=GL;Eh Cv(H Eb)D{if(Eb==0)B;H II=((Eb+AT::P-1)/AT::P)*AT::P;CW=G<Y*>(AL::malloc(II));if(CW!=M){Al=II;CR=Bi;}}Cv(J Cv&)=EO;Cv&Af=(J Cv&)=EO;Cv(Cv&&An)D:CW(An.CW),Al(An.Al),CR(An.CR){An.CW=M;An.Al=0;An.CR=X;}Cv&Af=(Cv&&An)D{if(BN!=&An){if(CR&&CW!=M)AL::free(CW);CW=An.CW;Al=An.Al;CR=An.CR;An.CW=M;An.Al=0;An.CR=X;}B*BN;}~Cv(){if(CR&&CW!=M)AL::free(CW);}V attach(V*memory,H IZ)D{if(CR&&CW!=M)AL::free(CW);CW=G<Y*>(memory);Al=IZ;CR=X;}Y*AR()D{B CW;}J Y*AR()J D{B CW;}H S()J D{B Al;}AN expand(H Dd)D{if(Dd==0)B Al>0;N Q H kMinInitialSize=4096;H KR=(Al>0)?(Al/4+Dd):AL::IQ(Dd,kMinInitialSize);H BM=Al+KR;BM=((BM+AT::P-1)/AT::P)*AT::P;if(BM<=Al)B X;V*new_buf=AL::malloc(BM);if(new_buf==M)B X;if(CW!=M)AL::Hz(new_buf,CW,Al);if(CR&&CW!=M)AL::free(CW);CW=G<Y*>(new_buf);Al=BM;CR=Bi;B Bi;}AN HT()J D{B CR;}Ew:Y*CW=M;H Al=0;AN CR=X;};AK(Fu<Cv<>>,"");}
#include "pmm/uncompact_macros.h"
