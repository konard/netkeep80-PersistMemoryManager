#pragma once
#include <type_traits>
#include <utility>
#include "pmm/compact_macros.h"
AY Ap{I<F T>Hc HasFreeData=Eg(T&t){{t.free_data()}D;};I<F T>Hc HasFreeAll=Eg(T&t){{t.free_all()}D;};I<F T>Hc HasPersistentCleanup=HasFreeData<T>||HasFreeAll<T>;I<F T,F O>Dt Cr{DF:j Fc=F O::I AV<T>;Eh Cr(Fc p)D:IJ(p){}Cr()D=GL;Cr(J Cr&)=EO;Cr&Af=(J Cr&)=EO;Cr(Cr&&An)D:IJ(An.IJ){An.IJ=Fc();}Cr&Af=(Cr&&An)D{if(BN!=&An){reset();IJ=An.IJ;An.IJ=Fc();}B*BN;}~Cr(){reset();}V reset()D{if(!IJ.AQ()){cleanup(*IJ);O::destroy_typed(IJ);IJ=Fc();}}Fc release()D{Fc p=IJ;IJ=Fc();B p;}T*Af->()J D{B&(*IJ);}T&Af*()J D{B*IJ;}Fc get()J D{B IJ;}Eh Af AN()J D{B!IJ.AQ();}Ew:Fc IJ;N V cleanup(T&IP)D{if Q(HasFreeData<T>)IP.free_data();EN if Q(HasFreeAll<T>)IP.free_all();}};}
#include "pmm/uncompact_macros.h"
