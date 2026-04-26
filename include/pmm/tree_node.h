#pragma once
#include "pmm/address_traits.h"
#include "pmm/block_field.h"
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "pmm/compact_macros.h"
AY Ap{enum:BV{Gw=0,Db=1,};
/*
## pmm-treenode
*/
I<F AT>Au HP{j E=AT;j A=F AT::A;A get_left()J D{B K::Cz<AT,K::Do>(BN);}A get_right()J D{B K::Cz<AT,K::Dg>(BN);}A get_parent()J D{B K::Cz<AT,K::DT>(BN);}A get_root()J D{B K::Cz<AT,K::Dn>(BN);}A Ar()J D{B K::Cz<AT,K::FF>(BN);}AL::Bk get_height()J D{B K::Cz<AT,K::EH>(BN);}BV Dc()J D{B K::Cz<AT,K::EV>(BN);}V set_left(A v)D{K::Ci<AT,K::Do>(BN,v);}V set_right(A v)D{K::Ci<AT,K::Dg>(BN,v);}V set_parent(A v)D{K::Ci<AT,K::DT>(BN,v);}V set_root(A v)D{K::Ci<AT,K::Dn>(BN,v);}V JM(A v)D{K::Ci<AT,K::FF>(BN,v);}V JO(AL::Bk v)D{K::Ci<AT,K::EH>(BN,v);}V set_node_type(BV v)D{K::Ci<AT,K::EV>(BN,v);}protected:A KE;A HX;A Gn;A GF;A BW;AL::Bk Ed;BV AW;};AK(AL::is_standard_layout<Ap::HP<Ap::W>>::DX,"");}
#include "pmm/uncompact_macros.h"
