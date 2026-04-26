#pragma once
#if defined(_MSVC_LANG)
#if _MSVC_LANG < 202002L
#error "pmm.h requires C++20 or later. Please compile with /std:c++20 on MSVC."
#endif
#elif __cplusplus < 202002L
#error "pmm.h requires C++20 or later. Please compile with -std=c++20."
#endif
#include "pmm/allocator_policy.h"
#include "pmm/block.h"
#include "pmm/block_state.h"
#include "pmm/diagnostics.h"
#include "pmm/forest_registry.h"
#include "pmm/layout.h"
#include "pmm/logging_policy.h"
#include "pmm/manager_configs.h"
#include "pmm/pallocator.h"
#include "pmm/parray.h"
#include "pmm/pmap.h"
#include "pmm/pptr.h"
#include "pmm/pstring.h"
#include "pmm/pstringview.h"
#include "pmm/typed_manager_api.h"
#include "pmm/types.h"
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include "pmm/compact_macros.h"
AY Ap{AY K{I<F C,F=V>Au Hb{j Ji=logging::NoLogging;};I<F C>Au Hb<C,AL::void_t<F C::AS>>{j Ji=F C::AS;};}template<typename ConfigT=CacheManagerConfig,size_t InstanceId=0>
/*
## pmm-persistmemorymanager
*/
class PersistMemoryManager:public detail::PersistMemoryTypedApi<PersistMemoryManager<ConfigT, InstanceId>>{public:j E=F JG::E;j Bu=F JG::Bu;j Az=F JG::Az;j AP=F JG::lock_policy;j AS=F K::Hb<JG>::Ji;j CM=Du<Az,E>;j A=F E::A;j BY=K::Fx<E>;j BF=K::Dj<E>;j Ae=Bm<JG,InstanceId>;I<F>friend Au Aa;I<F,F,F>friend Au pmap;friend Dt K::Hd<Ae>;I<F>friend AN save_manager(J BG*);I<F T>j AV=Ap::AV<T,Ae>;j Aa=Ap::Aa<Ae>;j Is=Ap::Is<Ae>;I<F _K,F _V>j pmap=Ap::pmap<_K,_V,Ae>;I<F T>j parray=Ap::parray<T,Ae>;I<F T>j Ex=Ap::Ex<T,Ae>;N AE last_error()D{B AA;}N V clear_error()D{AA=AE::Ok;}N V set_last_error(AE err)D{AA=err;}N AN create(H Eb)D{F AP::Aw Eo(CY);if(Eb<K::Fm){AA=AE::Gd;B X;}N Q H HZ=E::P;if(Eb>AL::Ag<H>::IQ()-(HZ-1)){AA=AE::Io;B X;}H II=((Eb+HZ-1)/HZ)*HZ;if(AJ.AR()==M||AJ.S()<II){H additional=(AJ.S()<II)?(II-AJ.S()):II;if(!AJ.expand(additional)){AA=AE::ExpandFailed;B X;}}if(AJ.AR()==M||AJ.S()<II){AA=AE::Ee;B X;}AN ok=Il(AJ.AR(),AJ.S());if(ok)ok=DE();if(ok)ok=Br();if(ok){AA=AE::Ok;AS::on_create(AJ.S());}B ok;}N AN create()D{F AP::Aw Eo(CY);if(AJ.AR()==M||AJ.S()<K::Fm){AA=(AJ.AR()==M)?AE::Ee:AE::Gd;B X;}AN ok=Il(AJ.AR(),AJ.S());if(ok)ok=DE();if(ok)ok=Br();if(ok){AA=AE::Ok;AS::on_create(AJ.S());}B ok;}N AN load(Bq&AM)D{AM.mode=ID::Repair;AM.ok=Bi;F AP::Aw Eo(CY);if(AJ.AR()==M||AJ.S()<K::Fm){AA=(AJ.AR()==M)?AE::Ee:AE::Gd;AM.Fw(AF::CS,o::GT);B X;}Y*m=AJ.AR();K::q<E>*AD=Cc(m);if(AD->HD!=JT){AA=AE::InvalidMagic;AS::Cg(AE::InvalidMagic);AM.Fw(AF::CS,o::GT,0,G<z>(JT),G<z>(AD->HD));B X;}if(!K::GA(AD->CK)){AA=AE::Gt;AS::Cg(AE::Gt);AM.Fw(AF::CS,o::GT,0,K::DC,G<z>(AD->CK));B X;}if(AD->S!=AJ.S()){AA=AE::SizeMismatch;AS::Cg(AE::SizeMismatch);AM.Fw(AF::CS,o::GT,0,AJ.S(),G<z>(AD->S));B X;}if(AD->P!=G<BV>(E::P)){AA=AE::KB;AS::Cg(AE::KB);AM.Fw(AF::CS,o::GT,0,E::P,G<z>(AD->P));B X;}DH IA=[](Bq&r,H from,o act){for(H i=from;i<r.Ce;++i)r.GJ[i].action=act;};H pre=AM.Ce;CM::IE(m,AD,AM);IA(AM,pre,o::Repaired);pre=AM.Ce;CM::IY(m,AD,AM);IA(AM,pre,o::Repaired);pre=AM.Ce;CM::Jg(m,AD,AM);IA(AM,pre,o::Rebuilt);pre=AM.Ce;CM::JK(m,AD,AM);IA(AM,pre,o::Rebuilt);if(K::HR(AD->CK))AD->CK=K::DC;AD->HT=X;AD->prev_total_size=0;CM::repair_linked_list(m,AD);CM::recompute_counters(m,AD);CM::rebuild_free_tree(m,AD);AH=Bi;{Bq Ix;Eu(Ix);for(H i=0;i<Ix.Ce;++i){J DH&e=Ix.GJ[i];AM.Fw(e.Ji,o::Repaired,e.Im,e.expected,e.actual);}}if(!Ev()){for(H i=0;i<AM.Ce;++i){if(AM.GJ[i].Ji==AF::Fl||AM.GJ[i].Ji==AF::IK||AM.GJ[i].Ji==AF::Gf)AM.GJ[i].action=o::GT;}AH=X;B X;}if(!Br()){AH=X;B X;}AA=AE::Ok;AS::on_load();B Bi;}
/*
### pmm-persistmemorymanager-destroy
*/
N V destroy()D{F AP::Aw Eo(CY);if(!AH)B;AH=X;AS::on_destroy();}N V destroy_image()D{F AP::Aw Eo(CY);Y*m=AJ.AR();if(m!=M&&AJ.S()>=K::Fm)Cc(m)->HD=0;AH=X;AS::on_destroy();}N AN Es()D{B AH.load(AL::Hg);}
/*
### pmm-persistmemorymanager-allocate
*/
N V*HM(H Bs)D{F AP::Aw Eo(CY);B Fj(Bs);}N V Ec(V*Gp)D{F AP::Aw Eo(CY);GM(Gp);}N AN lock_block_permanent(V*Gp)D{F AP::Aw Eo(CY);B Dr(Gp);}N AN is_permanently_locked(J V*Gp)D{F AP::Ax Eo(CY);if(!AH||Gp==M)B X;J Ap::Ao<E>*EK=Cx(Gp);if(EK==M)B X;B Z<E>::Dc(EK)==Ap::Db;}I<F T>N V set_root(AV<T>p)D{F AP::Aw Eo(CY);if(!AH)B;ED(AX(K::Bn),p.AQ()?G<A>(0):p.Bc());}I<F T>N AV<T>get_root()D{F AP::Ax Eo(CY);if(!AH)B AV<T>();A DV=BR(AX(K::Bn));if(DV==G<A>(0))B AV<T>();B AV<T>(DV);}N A IF(J BG*CP)D{F AP::Ax Eo(CY);if(!AH)B 0;J BF*Ep=AX(CP);B(Ep!=M)?Ep->CF:G<A>(0);}N A find_domain_by_symbol(AV<Aa>EW)D{F AP::Ax Eo(CY);if(!AH)B 0;J BF*Ep=FN(EW);B(Ep!=M)?Ep->CF:G<A>(0);}N AN has_domain(J BG*CP)D{B IF(CP)!=0;}N AN validate_bootstrap_invariants()D{F AP::Ax Eo(CY);B Br();}N AN register_domain(J BG*CP)D{F AP::Aw Eo(CY);if(!AH)B X;B BC(CP,0,K::Ai,0);}N AN register_system_domain(J BG*CP)D{F AP::Aw Eo(CY);if(!AH)B X;B BC(CP,K::BA,K::Ai,0);}N A DQ(J BG*CP)D{F AP::Ax Eo(CY);if(!AH)B 0;J BF*Ep=AX(CP);B BR(Ep);}N A DQ(A CF)D{F AP::Ax Eo(CY);if(!AH)B 0;J BF*Ep=Bz(CF);B BR(Ep);}N A DQ(AV<Aa>EW)D{F AP::Ax Eo(CY);if(!AH)B 0;J BF*Ep=FN(EW);B BR(Ep);}I<F T>N AV<T>Jt(J BG*CP)D{A DV=DQ(CP);B(DV==0)?AV<T>():AV<T>(DV);}I<F T>N AV<T>Jt(A CF)D{A DV=DQ(CF);B(DV==0)?AV<T>():AV<T>(DV);}I<F T>N AV<T>Jt(AV<Aa>EW)D{A DV=DQ(EW);B(DV==0)?AV<T>():AV<T>(DV);}I<F T>N AN set_domain_root(J BG*CP,AV<T>DV)D{F AP::Aw Eo(CY);if(!AH)B X;BF*Ep=AX(CP);B ED(Ep,DV.AQ()?G<A>(0):DV.Bc());}Ew:I<F T,F Gy>N Gy IR(AV<T>p,Gy(*getter)(J V*))D{if(p.AQ()||!AH)B Gy{};J V*EK=Gs(p);if(EK==M){AA=AE::CX;B Gy{};}B getter(EK);}I<F T,F Gy>N V IN(AV<T>p,V(*setter)(V*,Gy),Gy DX)D{if(p.AQ()||!AH)B;V*EK=Fp(p);if(EK==M){AA=AE::CX;B;}setter(EK,DX);}I<F T>N A Gi(AV<T>p,A(*getter)(J V*))D{A v=IR(p,getter);B(v==E::k)?G<A>(0):v;}I<F T>N V Gg(AV<T>p,V(*setter)(V*,A),A val)D{IN(p,setter,(val==0)?E::k:val);}DF:I<F T>N A get_tree_left_offset(AV<T>p)D{B Gi(p,&Z<E>::Ca);}I<F T>N A get_tree_right_offset(AV<T>p)D{B Gi(p,&Z<E>::CQ);}I<F T>N A get_tree_parent_offset(AV<T>p)D{B Gi(p,&Z<E>::CA);}I<F T>N V set_tree_left_offset(AV<T>p,A v)D{Gg(p,&Z<E>::Bp,v);}I<F T>N V set_tree_right_offset(AV<T>p,A v)D{Gg(p,&Z<E>::Bd,v);}I<F T>N V set_tree_parent_offset(AV<T>p,A v)D{Gg(p,&Z<E>::Ay,v);}I<F T>N A get_tree_weight(AV<T>p)D{B IR(p,&Z<E>::Ar);}I<F T>N V set_tree_weight(AV<T>p,A w)D{IN(p,&Z<E>::FR,w);}I<F T>N AL::Bk get_tree_height(AV<T>p)D{B IR(p,&Z<E>::Fo);}I<F T>N V set_tree_height(AV<T>p,AL::Bk h)D{IN(p,&Z<E>::DI,h);}I<F T>N HP<E>&DY(AV<T>p)D{Gx(!p.AQ()&&"tree_node: pptr must not be null");Gx(AH&&"tree_node: manager must be initialized before calling tree_node");V*EK=Fp(p);if(EK==M){AA=AE::CX;N thread_local HP<E>sentinel{};sentinel={};B sentinel;}B*AI<HP<E>*>(EK);}Ew:I<F Fn>N H Ir(Fn fn)D{if(!AH.load(AL::Hg))B 0;F AP::Ax Eo(CY);if(!AH.load(AL::memory_order_relaxed))B 0;B fn(FI(AJ.AR()));}DF:N H S()D{if(!AH.load(AL::Hg))B 0;F AP::Ax Eo(CY);B AH.load(AL::memory_order_relaxed)?AJ.S():0;}N H Bb()D{B Ir([](J DH*h){B E::Iw(h->Bb);});}N H free_size()D{B Ir([](J DH*h){H used=E::Iw(h->Bb);B(h->S>used)?(h->S-used):H(0);});}N H BP()D{B Ir([](J DH*h){B G<H>(h->BP);});}N H free_block_count()D{B Ir([](J DH*h){B G<H>(h->Be);});}N H alloc_block_count()D{B Ir([](J DH*h){B G<H>(h->Cf);});}N Bq verify()D{Bq AM;F AP::Ax Eo(CY);if(!AH||AJ.AR()==M){AM.Fw(AF::CS,o::GT);B AM;}verify_image_unlocked(AM);B AM;}I<F KD>N AN for_each_block(KD&&Hw)D{F AP::Ax Eo(CY);if(!AH)B X;J Y*m=AJ.AR();j R=Z<E>;J K::q<E>*AD=FI(m);A At=AD->Aq;N Q H BH=E::P;Fa(At!=E::k){if(G<H>(At)*BH+AG(Ao<E>)>AD->S)IV;J V*BU=m+G<H>(At)*BH;J Ao<E>*EK=AI<J Ao<E>*>(BU);A EC=K::En(m,AD,EK);DH w=R::Ar(BU);AN is_used=(w>0);H hdr_bytes=AG(Ao<E>);H HK=is_used?G<H>(w)*BH:0;BlockView FP;FP.Ha=At;FP.Bc=G<AL::EP>(G<H>(At)*BH);FP.S=G<H>(EC)*BH;FP.header_size=hdr_bytes;FP.Bs=HK;FP.alignment=BH;FP.used=is_used;Hw(FP);At=R::AU(BU);}B Bi;}I<F KD>N AN for_each_free_block(KD&&Hw)D{F AP::Ax Eo(CY);if(!AH)B X;J Y*m=AJ.AR();J K::q<E>*AD=FI(m);EQ(m,AD,AD->Am,0,Hw);B Bi;}N Bu&EG()D{B AJ;}Ew:N AO Bu AJ{};N AO AL::atomic<AN>AH{X};N AO F AP::JS CY{};N AO thread_local AE AA{AE::Ok};N AN Hs(A off,H CD)D{if(off==0||AJ.AR()==M||AJ.S()==0)B X;H BI=G<H>(off)*E::P;B BI+CD<=AJ.S();}N V*Fj(H Bs)D{if(!AH){AA=AE::IX;AS::By(Bs,AE::IX);B M;}if(Bs==0){AA=AE::Gd;AS::By(Bs,AE::Gd);B M;}Y*m=AJ.AR();K::q<E>*AD=Cc(m);A FK=K::DR<E>(Bs);if(FK==0)FK=1;if(FK>AL::Ag<A>::IQ()-Aj){AA=AE::Io;AS::By(Bs,AE::Io);B M;}A KO=Aj+FK;A At=Az::GI(m,AD,KO);if(At!=E::k){AA=AE::Ok;B CM::GR(m,AD,At,Bs);}if(!KU(Bs)){AA=AE::Dy;AS::By(Bs,AE::Dy);B M;}m=AJ.AR();AD=Cc(m);At=Az::GI(m,AD,KO);if(At!=E::k){AA=AE::Ok;B CM::GR(m,AD,At,Bs);}AA=AE::Dy;AS::By(Bs,AE::Dy);B M;}N V GM(V*Gp)D{if(!AH||Gp==M)B;Ap::Ao<E>*EK=Cx(Gp);if(EK==M)B;A freed=Z<E>::Ar(EK);if(freed==0)B;if(Z<E>::Dc(EK)==Ap::Db)B;Y*m=AJ.AR();K::q<E>*AD=Cc(m);A As=K::KW<E>(m,EK);Bh<E>*alloc=Bh<E>::Cm(EK);alloc->mark_as_free();AD->Cf--;AD->Be++;if(AD->Bb>=freed)AD->Bb-=freed;CM::coalesce(m,AD,As);}N AN Dr(V*Gp)D{if(!AH||Gp==M)B X;Ap::Ao<E>*EK=Cx(Gp);if(EK==M)B X;A w=Z<E>::Ar(EK);if(w==0)B X;Z<E>::JN(EK,Ap::Db);B Bi;}I<F T,F...Args>N AV<T>create_typed_unlocked(Args&&...args)D{AK(AL::is_nothrow_constructible_v<T,Args...>,"");V*CC=Fj(AG(T));if(CC==M)B AV<T>();AV<T>p=Di<T>(CC);T*IP=Ae::I Ck<T>(p);if(IP==M){GM(CC);B AV<T>();}::new(IP)T(G<Args&&>(args)...);B p;}
#include "pmm/forest_domain_mixin.inc"
#include "pmm/verify_repair_mixin.inc"
N Q H Ff=K::Bf<E>;N Q A Aj=G<A>(Ff/E::P);N Q A GX=K::kManagerHeaderGranules_t<E>;N Q A Fd=Aj+GX;N K::q<E>*Cc(Y*m)D{B K::Dq<E>(m);}N J K::q<E>*FI(J Y*m)D{B K::Dq<E>(m);}Au layout_access{j E=Ae::E;j Az=Ae::Az;j AS=Ae::AS;j Bu=Ae::Bu;j A=Ae::A;N Q z JT=Ap::JT;N Q H Ff=Ae::Ff;N Q A Aj=Ae::Aj;N Q A GX=Ae::GX;N Q A Fd=Ae::Fd;N K::q<E>*Cc(Y*m)D{B Ae::Cc(m);}N V set_initialized()D{Ae::AH=Bi;}};N AN Il(Y*m,H IZ)D{B K::Ja<layout_access>::Il(AJ,m,IZ);}N AN KU(H Bs)D{B K::Ja<layout_access>::KU(AJ,AH,Bs);}};}
#include "pmm/uncompact_macros.h"
