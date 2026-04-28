#pragma once
#include "pmm/address_traits.h"
#include "pmm/block_header.h"
#include <cstdint>
#include <type_traits>
namespace pmm
{
/*
## pmm-block
*/
template <typename AT> using Block = BlockHeader<AT>;
static_assert( sizeof( pmm::Block<pmm::DefaultAddressTraits> ) == 32, "" );
}
