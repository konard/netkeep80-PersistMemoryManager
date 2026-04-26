#pragma once
#include "pmm/address_traits.h"
#include "pmm/tree_node.h"
#include <cstdint>
#include <type_traits>
namespace pmm{
/*
## pmm-block
*/
template<typename AT>struct Block:TreeNode<AT>{using address_traits=AT;using index_type=typename AT::index_type;protected:index_type prev_offset;index_type next_offset;};static_assert(sizeof(pmm::Block<pmm::DefaultAddressTraits>)==32,"");}
