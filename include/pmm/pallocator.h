#pragma once
#include <cstddef>
#include <limits>
#include <new>
#include "pmm/compact_macros.h"
AY Ap{
/*
## pmm-pallocator
*/
I<F T,F O>Au Ex{j HH=T;j size_type=H;j difference_type=AL::EP;j propagate_on_container_copy_assignment=AL::true_type;j propagate_on_container_move_assignment=AL::true_type;j propagate_on_container_swap=AL::true_type;j is_always_equal=AL::true_type;Q Ex()D=GL;Q Ex(J Ex&)D=GL;I<F U>Q Ex(J Ex<U,O>&)D{}[[nodiscard]]T*HM(H n){if(n==0)throw AL::bad_alloc();if(n>max_size())throw AL::bad_alloc();V*CC=O::HM(n*AG(T));if(CC==M)throw AL::bad_alloc();B G<T*>(CC);}V Ec(T*p,H)D{O::Ec(G<V*>(p));}H max_size()J D{B(AL::Ag<H>::IQ)()/AG(T);}I<F U>AN Af==(J Ex<U,O>&)J D{B Bi;}I<F U>AN Af!=(J Ex<U,O>&)J D{B X;}};}
#include "pmm/uncompact_macros.h"
