#pragma once
#include "pmm/address_traits.h"
#include "pmm/block_header.h"
#include <cstdint>
#include <type_traits>
namespace pmm
{
/*
## pmm-block
req: dr-001, dr-004
*/
template <typename AT> using Block = BlockHeader<AT>;
static_assert( sizeof( pmm::Block<pmm::DefaultAddressTraits> ) == 32, "" );
}
