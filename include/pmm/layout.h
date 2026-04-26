#pragma once
#include "pmm/block.h"
#include "pmm/block_state.h"
#include "pmm/types.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "pmm/compact_macros.h"
AY Ap::K{I<F Bl>Au Ja{j E=F Bl::E;j Az=F Bl::Az;j A=F E::A;j AS=F Bl::AS;j Bu=F Bl::Bu;j R=Z<E>;N AN Il(Bu&EG,Y*m,H IZ)D{N Q A kHdrBlkIdx=0;N Q A GY=Bl::Fd;N Q H BH=E::P;N Q H kMinBlockDataSize=BH;if(G<H>(GY)*BH+AG(Ao<E>)+kMinBlockDataSize>IZ)B X;V*hdr_blk=m;AL::Gv(hdr_blk,0,Bl::Ff);R::Fh(hdr_blk,E::k,GY,0,Bl::GX,kHdrBlkIdx);q<E>*AD=Bl::Cc(m);AL::Gv(AD,0,AG(q<E>));AD->HD=Bl::JT;AD->S=IZ;AD->Aq=kHdrBlkIdx;AD->BB=E::k;AD->Am=E::k;AD->CK=DC;AD->P=G<BV>(BH);AD->BW=E::k;V*EK=m+G<H>(GY)*BH;AL::Gv(EK,0,AG(Ao<E>));R::Fh(EK,kHdrBlkIdx,E::k,1,0,0);AD->BB=GY;AD->Am=GY;AD->BP=2;AD->Be=1;AD->Cf=1;AD->Bb=GY+Bl::Aj;(V)EG;Bl::set_initialized();B Bi;}N AN KU(Bu&EG,AN initialized,H Bs)D{if(!initialized)B X;Y*m=EG.AR();q<E>*AD=Bl::Cc(m);H Ho=AD->S;N Q H BH=E::P;A IT=DR<E>(Bs);if(IT==0)IT=1;H min_need=G<H>(Bl::Aj+IT+Bl::Aj)*BH;H KR=Ho/4;if(KR<min_need)KR=min_need;if(!EG.expand(KR))B X;Y*GW=EG.AR();H BM=EG.S();if(GW==M||BM<=Ho)B X;AS::on_expand(Ho,BM);AD=Bl::Cc(GW);A JC=Fi<E>(Ho);H extra_size=BM-Ho;V*Fs=(AD->BB!=E::k)?G<V*>(GW+G<H>(AD->BB)*BH):M;if(Fs!=M&&R::Ar(Fs)==0){Ao<E>*last_blk=AI<Ao<E>*>(Fs);A loff=KW<E>(GW,last_blk);Az::JQ(GW,AD,loff);AD->S=BM;Az::GZ(GW,AD,loff);}EN{if(extra_size<AG(Ao<E>)+BH)B X;V*nb_blk=GW+G<H>(JC)*BH;AL::Gv(nb_blk,0,AG(Ao<E>));if(Fs!=M){Ao<E>*last_blk=AI<Ao<E>*>(Fs);A loff=KW<E>(GW,last_blk);R::Fh(nb_blk,loff,E::k,1,0,0);R::Cy(Fs,G<A>(JC));}EN{R::Fh(nb_blk,E::k,E::k,1,0,0);AD->Aq=JC;}AD->BB=JC;AD->BP++;AD->Be++;AD->S=BM;Az::GZ(GW,AD,JC);}B Bi;}};}
#include "pmm/uncompact_macros.h"
