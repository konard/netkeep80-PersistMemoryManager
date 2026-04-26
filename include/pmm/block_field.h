#pragma once
#include "pmm/address_traits.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
namespace pmm {
namespace detail {
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
static_assert(std::is_standard_layout<BlockFieldLayout<std::uint32_t>>::value, "BlockFieldLayout must remain standard-layout");
#define PMM_BLOCK_FIELD(Tag, Member, ValueType)                               \
  struct Tag {                                                                \
    template <typename IndexT> using value_type = ValueType;                  \
    template <typename IndexT>                                                \
    static constexpr std::size_t offset() noexcept {                          \
      return offsetof(BlockFieldLayout<IndexT>, Member);                      \
    }                                                                         \
  };
#define PMM_BLOCK_INDEX_FIELD(Tag, Member) PMM_BLOCK_FIELD(Tag, Member, IndexT)
PMM_BLOCK_INDEX_FIELD(BlockWeightField, weight) PMM_BLOCK_INDEX_FIELD(BlockLeftOffsetField, left_offset) PMM_BLOCK_INDEX_FIELD(BlockRightOffsetField, right_offset) PMM_BLOCK_INDEX_FIELD(BlockParentOffsetField, parent_offset) PMM_BLOCK_INDEX_FIELD(BlockRootOffsetField, root_offset) PMM_BLOCK_FIELD(BlockAvlHeightField, avl_height, std::int16_t) PMM_BLOCK_FIELD(BlockNodeTypeField, node_type, std::uint16_t) PMM_BLOCK_INDEX_FIELD(BlockPrevOffsetField, prev_offset) PMM_BLOCK_INDEX_FIELD(BlockNextOffsetField, next_offset) template <typename AddressTraitsT, typename FieldTag> struct BlockFieldTraits;
template <typename AddressTraitsT, typename FieldTag> struct BlockFieldTraits {
using index_type = typename AddressTraitsT::index_type;
using value_type = typename FieldTag::template value_type<index_type>;
static constexpr std::size_t offset = FieldTag::template offset<index_type>();
};
#undef PMM_BLOCK_INDEX_FIELD
#undef PMM_BLOCK_FIELD
template <typename AddressTraitsT, typename FieldTag> using block_field_value_t = typename BlockFieldTraits<AddressTraitsT, FieldTag>::value_type;
template <typename AddressTraitsT, typename FieldTag> inline constexpr std::size_t block_field_offset_v = BlockFieldTraits<AddressTraitsT, FieldTag>::offset;
struct BlockFieldByteAccess {
template <typename ValueT> static ValueT read(const void *raw, std::size_t offset) noexcept {
ValueT value{
};
std::memcpy(&value, static_cast<const std::uint8_t *>(raw) + offset, sizeof(value));
return value;
}
template <typename ValueT> static void write(void *raw, std::size_t offset, ValueT value) noexcept {
std::memcpy(static_cast<std::uint8_t *>(raw) + offset, &value, sizeof(value));
}
};
template <typename AddressTraitsT, typename FieldTag, typename AccessPolicy = BlockFieldByteAccess> block_field_value_t<AddressTraitsT, FieldTag> read_block_field(const void *raw) noexcept {
return AccessPolicy::template read< block_field_value_t<AddressTraitsT, FieldTag>>( raw, block_field_offset_v<AddressTraitsT, FieldTag>);
}
template <typename AddressTraitsT, typename FieldTag, typename AccessPolicy = BlockFieldByteAccess> void write_block_field( void *raw, block_field_value_t<AddressTraitsT, FieldTag> value) noexcept {
AccessPolicy::template write<block_field_value_t<AddressTraitsT, FieldTag>>( raw, block_field_offset_v<AddressTraitsT, FieldTag>, value);
}
template <typename AddressTraitsT> inline constexpr std::size_t block_tree_slot_size_v = offsetof( BlockFieldLayout<typename AddressTraitsT::index_type>, prev_offset);
template <typename AddressTraitsT> inline constexpr std::size_t block_layout_size_v = sizeof(BlockFieldLayout<typename AddressTraitsT::index_type>);
}
}
