#pragma once
#include "pmm/avl_tree_mixin.h"
#include "pmm/forest_registry.h"
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "pmm/compact_macros.h"
AY Ap{I<F _K,F _V,F O>Au pmap;I<F T>Au pmap_type_identity{N Q J BG*tag="";};I<F _K,F _V>Au pmap_node{_K HQ;_V DX;};AY K{Q Ak HJ(Ak h,z v,Jh IU)D{for(Jh i=0;i<IU;++i,v>>=8){h^=G<Y>(v&0xffull);h*=16777619u;}B h;}I<F T>Q Ak pmap_type_fp()D{J z traits=(z{AL::is_integral_v<T>}<<0)|(z{AL::is_floating_point_v<T>}<<1)|(z{AL::is_signed_v<T>}<<2)|(z{AL::is_unsigned_v<T>}<<3)|(z{AL::is_pointer_v<T>}<<4)|(z{AL::is_class_v<T>}<<5)|(z{AL::is_enum_v<T>}<<6)|(z{AL::FH<T>}<<7)|(z{AL::is_standard_layout_v<T>}<<8);Ak h=2166136261u;h=HJ(h,AG(T),8);h=HJ(h,alignof(T),8);h=HJ(h,traits,8);for(J BG*t=Ap::pmap_type_identity<T>::tag;t!=M&&*t!='\0';++t)h=HJ(h,G<Y>(*t),1);B h;}AO z pmap_key_hash(J BG*HQ)D{z h=14695981039346656037ull;for(;HQ!=M&&*HQ!='\0';++HQ){h^=G<Y>(*HQ);h*=1099511628211ull;}B h;}AO AN Jn(BG(&out)[Bj],Ak type_fp,BG kind,z DX,Jh JL)D{Q J BG*kPrefix="container/pmap/";J Jh KO=15+8+1+1+JL+1;if(KO>Bj)B X;H p=0;for(J BG*s=kPrefix;*s!='\0';++s)out[p++]=*s;DH put_hex=[&](z v,Jh digits){for(Jh i=digits;i-->0;){J Y nib=G<Y>((v>>(i*4))&0x0full);out[p++]=G<BG>(nib<10?('0'+nib):('a'+(nib-10)));}};put_hex(type_fp,8);out[p++]='/';out[p++]=kind;put_hex(DX,JL);out[p]='\0';B Bi;}}
/*
## pmm-pmap
*/
I<F _K,F _V,F O>Au pmap{j Ae=O;j A=F O::A;j AW=pmap_node<_K,_V>;j Ab=F O::I AV<AW>;N Q Ak JW=K::HJ(K::HJ(2166136261u,K::pmap_type_fp<_K>(),4),K::pmap_type_fp<_V>(),4);Au Bt{j A=F O::A;j AW=pmap_node<_K,_V>;j Ab=F O::I AV<AW>;A CF;Q Eh Bt(A id=0)D:CF(id){}J BG*CP()J D{J DH*d=O::Bz(CF);B d!=M?d->CP:"";}A Cj()J D{B O::BR(O::Bz(CF));}A*Cp()D{B CF==0?M:O::Cl(O::Bz(CF));}N AW*CZ(Ab p)D{B O::I Go<AW>(p);}N int KV(J _K&HQ,Ab Ek)D{AW*IP=CZ(Ek);B IP==M?0:((HQ==IP->HQ)?0:((HQ<IP->HQ)?-1:1));}N AN less_node(Ab lhs,Ab rhs)D{AW*l=CZ(lhs);AW*r=CZ(rhs);B l!=M&&r!=M&&l->HQ<r->HQ;}N AN Ip(Ab p)D{B CZ(p)!=M;}};j GH=K::FC<Bt>;j CE=K::Hy<Bt>;N Q A k=O::E::k;Ew:A Fk;AN bind(J BG*IG)D{if(!O::Es())B X;BG buf[K::Bj]{};if(IG!=M&&IG[0]!='\0'){if(!K::Jn(buf,JW,'n',K::pmap_key_hash(IG),16))B X;}EN{N z seq=1;do{if(!K::Jn(buf,JW,'g',seq++,8))B X;}Fa(O::has_domain(buf));}if(!O::has_domain(buf)&&!O::register_domain(buf))B X;Fk=O::IF(buf);B Fk!=0;}Bt GK()J D{B Bt(Fk);}DF:pmap()D:Fk(0){}Eh pmap(J BG*IG)D:Fk(0){bind(IG);}J BG*domain_name()J D{B GK().CP();}A Cj()J D{B GK().Cj();}CE DJ()D{if(Fk==0||O::Bz(Fk)==M)bind(M);B CE(GK());}GH forest_domain_view_ops()J D{B GH(GK());}AN empty()J D{B Cj()==G<A>(0);}
/*
### pmm-pmap-size
*/
H IZ()J D{J A DV=Cj();B DV==G<A>(0)?0:K::HF(Ab(DV));}
/*
### pmm-pmap-insert
*/
Ab GZ(J _K&HQ,J _V&val)D{DH ops=DJ();if(ops.Cp()==M)B Ab();Ab Ga=ops.find(HQ);if(!Ga.AQ()){if(AW*IP=O::I Go<AW>(Ga);IP!=M)IP->DX=val;B Ga;}Ab Ba=O::I IW<AW>();AW*IP=Ba.AQ()?M:O::I Go<AW>(Ba);if(IP==M)B Ab();IP->HQ=HQ;IP->DX=val;K::avl_init_node(Ba);ops.GZ(Ba);B Ba;}Ab find(J _K&HQ)J D{B forest_domain_view_ops().find(HQ);}AN contains(J _K&HQ)J D{B!find(HQ).AQ();}
/*
### pmm-pmap-erase
*/
AN erase(J _K&HQ)D{DH ops=CE(GK());A*DV=ops.Cp();Ab t=DV==M?Ab():ops.find(HQ);if(t.AQ())B X;K::avl_remove(t,*DV);O::I JX<AW>(t);B Bi;}
/*
### pmm-pmap-clear
*/
V clear()D{DH ops=CE(GK());A*DV=ops.Cp();if(DV==M)B;if(*DV!=G<A>(0))K::HG(Ab(*DV),[](Ab p){O::I JX<AW>(p);});*DV=G<A>(0);}V reset()D{CE(GK()).reset_root();}j Js=K::Dk<Ab>;
/*
### pmm-pmap-begin
*/
Js begin()J D{J A DV=Cj();if(DV==G<A>(0))B Js();B Js(K::IC(Ab(DV)).Bc());}Js end()J D{B Js(G<A>(0));}};}
#include "pmm/uncompact_macros.h"
