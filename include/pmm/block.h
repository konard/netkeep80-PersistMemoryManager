#pragma once
#include "pmm/address_traits.h"
#include "pmm/tree_node.h"
#include <cstdint>
#include <type_traits>
namespace pmm {
/*
## pmm-block
*/
template <typename AddressTraitsT> struct Block : TreeNode<AddressTraitsT> {
  using address_traits = AddressTraitsT;
  using index_type = typename AddressTraitsT::index_type;
protected:
  index_type prev_offset;
  index_type next_offset;
};
static_assert(sizeof(pmm::Block<pmm::DefaultAddressTraits>) == 32,
              "Block<DefaultAddressTraits> must be 32 bytes ");
}
