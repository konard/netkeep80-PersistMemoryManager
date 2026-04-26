#pragma once

#include "pmm/address_traits.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace pmm {
namespace detail {

/*
## pmm::detail::BlockFieldLayout
*/
template <typename IndexT> struct BlockFieldLayout {

  IndexT weight;

  IndexT left_offset;

  IndexT right_offset;

  IndexT parent_offset;

  IndexT root_offset;

  std::int16_t avl_height;
  std::uint16_t node_type;

  IndexT prev_offset;

  IndexT next_offset;
};

static_assert(std::is_standard_layout<BlockFieldLayout<std::uint32_t>>::value,
              "BlockFieldLayout must remain standard-layout");

/*
## pmm::detail::BlockWeightField
*/
struct BlockWeightField {};

/*
## pmm::detail::BlockLeftOffsetField
*/
struct BlockLeftOffsetField {};

/*
## pmm::detail::BlockRightOffsetField
*/
struct BlockRightOffsetField {};

/*
## pmm::detail::BlockParentOffsetField
*/
struct BlockParentOffsetField {};

/*
## pmm::detail::BlockRootOffsetField
*/
struct BlockRootOffsetField {};

/*
## pmm::detail::BlockAvlHeightField
*/
struct BlockAvlHeightField {};

/*
## pmm::detail::BlockNodeTypeField
*/
struct BlockNodeTypeField {};

/*
## pmm::detail::BlockPrevOffsetField
*/
struct BlockPrevOffsetField {};

/*
## pmm::detail::BlockNextOffsetField
*/
struct BlockNextOffsetField {};

template <typename AddressTraitsT, typename FieldTag> struct BlockFieldTraits;

/*
## pmm::detail::BlockFieldTraits
*/
template <typename AddressTraitsT>
struct BlockFieldTraits<AddressTraitsT, BlockWeightField> {

  using value_type = typename AddressTraitsT::index_type;

  static constexpr std::size_t offset =
      offsetof(BlockFieldLayout<value_type>, weight);
};

template <typename AddressTraitsT>
struct BlockFieldTraits<AddressTraitsT, BlockLeftOffsetField> {

  using value_type = typename AddressTraitsT::index_type;

  static constexpr std::size_t offset =
      offsetof(BlockFieldLayout<value_type>, left_offset);
};

template <typename AddressTraitsT>
struct BlockFieldTraits<AddressTraitsT, BlockRightOffsetField> {

  using value_type = typename AddressTraitsT::index_type;

  static constexpr std::size_t offset =
      offsetof(BlockFieldLayout<value_type>, right_offset);
};

template <typename AddressTraitsT>
struct BlockFieldTraits<AddressTraitsT, BlockParentOffsetField> {

  using value_type = typename AddressTraitsT::index_type;

  static constexpr std::size_t offset =
      offsetof(BlockFieldLayout<value_type>, parent_offset);
};

template <typename AddressTraitsT>
struct BlockFieldTraits<AddressTraitsT, BlockRootOffsetField> {

  using value_type = typename AddressTraitsT::index_type;

  static constexpr std::size_t offset =
      offsetof(BlockFieldLayout<value_type>, root_offset);
};

template <typename AddressTraitsT>
struct BlockFieldTraits<AddressTraitsT, BlockAvlHeightField> {

  using value_type = std::int16_t;

  using index_type = typename AddressTraitsT::index_type;

  static constexpr std::size_t offset =
      offsetof(BlockFieldLayout<index_type>, avl_height);
};

template <typename AddressTraitsT>
struct BlockFieldTraits<AddressTraitsT, BlockNodeTypeField> {

  using value_type = std::uint16_t;

  using index_type = typename AddressTraitsT::index_type;

  static constexpr std::size_t offset =
      offsetof(BlockFieldLayout<index_type>, node_type);
};

template <typename AddressTraitsT>
struct BlockFieldTraits<AddressTraitsT, BlockPrevOffsetField> {

  using value_type = typename AddressTraitsT::index_type;

  static constexpr std::size_t offset =
      offsetof(BlockFieldLayout<value_type>, prev_offset);
};

template <typename AddressTraitsT>
struct BlockFieldTraits<AddressTraitsT, BlockNextOffsetField> {

  using value_type = typename AddressTraitsT::index_type;

  static constexpr std::size_t offset =
      offsetof(BlockFieldLayout<value_type>, next_offset);
};

template <typename AddressTraitsT, typename FieldTag>
using block_field_value_t =
    typename BlockFieldTraits<AddressTraitsT, FieldTag>::value_type;

template <typename AddressTraitsT, typename FieldTag>
inline constexpr std::size_t block_field_offset_v =
    BlockFieldTraits<AddressTraitsT, FieldTag>::offset;

/*
## pmm::detail::BlockFieldByteAccess
*/
struct BlockFieldByteAccess {
  template <typename ValueT>
  static ValueT read(const void *raw, std::size_t offset) noexcept {

    ValueT value{};

    std::memcpy(&value, static_cast<const std::uint8_t *>(raw) + offset,
                sizeof(value));
    return value;
  }

  template <typename ValueT>
  static void write(void *raw, std::size_t offset, ValueT value) noexcept {
    std::memcpy(static_cast<std::uint8_t *>(raw) + offset, &value,
                sizeof(value));
  }
};

template <typename AddressTraitsT, typename FieldTag,
          typename AccessPolicy = BlockFieldByteAccess>
block_field_value_t<AddressTraitsT, FieldTag>
read_block_field(const void *raw) noexcept {
  return AccessPolicy::template read<
      block_field_value_t<AddressTraitsT, FieldTag>>(
      raw, block_field_offset_v<AddressTraitsT, FieldTag>);
}

template <typename AddressTraitsT, typename FieldTag,
          typename AccessPolicy = BlockFieldByteAccess>
void write_block_field(
    void *raw, block_field_value_t<AddressTraitsT, FieldTag> value) noexcept {
  AccessPolicy::template write<block_field_value_t<AddressTraitsT, FieldTag>>(
      raw, block_field_offset_v<AddressTraitsT, FieldTag>, value);
}

template <typename AddressTraitsT>
inline constexpr std::size_t block_tree_slot_size_v = offsetof(
    BlockFieldLayout<typename AddressTraitsT::index_type>, prev_offset);

template <typename AddressTraitsT>
inline constexpr std::size_t block_layout_size_v =
    sizeof(BlockFieldLayout<typename AddressTraitsT::index_type>);

}
}
