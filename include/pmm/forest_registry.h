#pragma once
#include "pmm/address_traits.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "pmm/compact_macros.h"
AY Ap::K{AO Q H Bj=48;AO Q H HE=32;AO Q J BG*Ct="system/free_tree";AO Q J BG*Cd="system/symbols";AO Q J BG*Cs="system/domain_registry";AO Q J BG*kSystemTypeForestRegistry="type/forest_registry";AO Q J BG*Hr="type/forest_domain_record";AO Q J BG*kSystemTypePstringview="type/pstringview";AO Q J BG*Bn="service/domain_root";AO Q J BG*kServiceNameDomainSymbol="service/domain_symbol";AO Q Ak DB=0x50465247U;AO Q BV DN=1;AO Q Y Ai=0;AO Q Y EM=1;AO Q Y BA=0x01;
/*
### pmm-detail-forestdomainrecord
*/
I<F AT>Au Dj{j A=F AT::A;A CF;A BW;A DD;Y Cq;Y Iz;BV reserved;BG CP[Bj];Q Dj()D:CF(0),BW(0),DD(0),Cq(Ai),Iz(0),reserved(0),CP{}{}};
/*
### pmm-detail-forestdomainregistry
*/
I<F AT>Au Fx{j A=F AT::A;Ak HD;BV Kk;BV DW;A Hp;Dj<AT>FB[HE];Q Fx()D:HD(DB),Kk(DN),DW(0),Hp(1),FB{}{}};I<F AT>AO AN forest_domain_name_equals(J Dj<AT>&Ep,J BG*CP)D{if(CP==M)B X;B AL::strncmp(Ep.CP,CP,Bj)==0;}AO AN FJ(J BG*CP)D{if(CP==M||CP[0]=='\0')B X;B AL::strlen(CP)<Bj;}I<F AT>AO AN forest_domain_name_copy(Dj<AT>&Ep,J BG*CP)D{if(!FJ(CP))B X;AL::Gv(Ep.CP,0,AG(Ep.CP));AL::Hz(Ep.CP,CP,AL::strlen(CP));B Bi;}AK(AL::FH<Dj<W>>,"");AK(AL::is_nothrow_default_constructible_v<Fx<W>>,"");}
#include "pmm/uncompact_macros.h"
