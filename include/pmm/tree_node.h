#pragma once

#include "pmm/address_traits.h"
#include "pmm/block_field.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace pmm {

enum : std::uint16_t {
  kNodeReadWrite = 0,
  kNodeReadOnly = 1,
};

/*
## pmm::TreeNode
*/
template <typename AddressTraitsT> struct TreeNode {

  using address_traits = AddressTraitsT;

  using index_type = typename AddressTraitsT::index_type;

  /*
  ### pmm::TreeNode::get_left
  */
  index_type get_left() const noexcept {
    return detail::read_block_field<AddressTraitsT,
                                    detail::BlockLeftOffsetField>(this);
  }

  /*
  ### pmm::TreeNode::get_right
  */
  index_type get_right() const noexcept {
    return detail::read_block_field<AddressTraitsT,
                                    detail::BlockRightOffsetField>(this);
  }

  /*
  ### pmm::TreeNode::get_parent
  */
  index_type get_parent() const noexcept {
    return detail::read_block_field<AddressTraitsT,
                                    detail::BlockParentOffsetField>(this);
  }

  /*
  ### pmm::TreeNode::get_root
  */
  index_type get_root() const noexcept {
    return detail::read_block_field<AddressTraitsT,
                                    detail::BlockRootOffsetField>(this);
  }

  /*
  ### pmm::TreeNode::get_weight
  */
  index_type get_weight() const noexcept {
    return detail::read_block_field<AddressTraitsT, detail::BlockWeightField>(
        this);
  }

  /*
  ### pmm::TreeNode::get_height
  */
  std::int16_t get_height() const noexcept {
    return detail::read_block_field<AddressTraitsT,
                                    detail::BlockAvlHeightField>(this);
  }

  /*
  ### pmm::TreeNode::get_node_type
  */
  std::uint16_t get_node_type() const noexcept {
    return detail::read_block_field<AddressTraitsT, detail::BlockNodeTypeField>(
        this);
  }

  /*
  ### pmm::TreeNode::set_left
  */
  void set_left(index_type v) noexcept {

    detail::write_block_field<AddressTraitsT, detail::BlockLeftOffsetField>(
        this, v);
  }

  /*
  ### pmm::TreeNode::set_right
  */
  void set_right(index_type v) noexcept {

    detail::write_block_field<AddressTraitsT, detail::BlockRightOffsetField>(
        this, v);
  }

  /*
  ### pmm::TreeNode::set_parent
  */
  void set_parent(index_type v) noexcept {

    detail::write_block_field<AddressTraitsT, detail::BlockParentOffsetField>(
        this, v);
  }

  /*
  ### pmm::TreeNode::set_root
  */
  void set_root(index_type v) noexcept {

    detail::write_block_field<AddressTraitsT, detail::BlockRootOffsetField>(
        this, v);
  }

  /*
  ### pmm::TreeNode::set_weight
  */
  void set_weight(index_type v) noexcept {

    detail::write_block_field<AddressTraitsT, detail::BlockWeightField>(this,
                                                                        v);
  }

  /*
  ### pmm::TreeNode::set_height
  */
  void set_height(std::int16_t v) noexcept {

    detail::write_block_field<AddressTraitsT, detail::BlockAvlHeightField>(this,
                                                                           v);
  }

  /*
  ### pmm::TreeNode::set_node_type
  */
  void set_node_type(std::uint16_t v) noexcept {

    detail::write_block_field<AddressTraitsT, detail::BlockNodeTypeField>(this,
                                                                          v);
  }

protected:
  index_type weight;

  index_type left_offset;

  index_type right_offset;

  index_type parent_offset;

  index_type root_offset;

  std::int16_t avl_height;

  std::uint16_t node_type;
};

static_assert(
    std::is_standard_layout<pmm::TreeNode<pmm::DefaultAddressTraits>>::value,
    "TreeNode must be standard-layout ");

}
