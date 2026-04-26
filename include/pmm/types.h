#pragma once
#include "pmm/address_traits.h"
#include "pmm/block.h"
#include "pmm/block_state.h"
#include "pmm/tree_node.h"
#include "pmm/validation.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
namespace pmm {
/*
## pmm-pmmerror
*/
enum class PmmError : std::uint8_t {
  Ok = 0,
  NotInitialized = 1,
  InvalidSize = 2,
  Overflow = 3,
  OutOfMemory = 4,
  ExpandFailed = 5,
  InvalidMagic = 6,
  CrcMismatch = 7,
  SizeMismatch = 8,
  GranuleMismatch = 9,
  BackendError = 10,
  InvalidPointer = 11,
  BlockLocked = 12,
  UnsupportedImageVersion = 13,
};
inline constexpr std::size_t kGranuleSize = 16;
static_assert((kGranuleSize & (kGranuleSize - 1)) == 0,
              "kGranuleSize must be a power of 2 ");
static_assert(kGranuleSize == pmm::DefaultAddressTraits::granule_size,
              "kGranuleSize must match DefaultAddressTraits::granule_size ");
inline constexpr std::uint64_t kMagic = 0x504D4D5F56303938ULL;
/*
## pmm-memorystats
*/
struct MemoryStats {
  std::size_t total_blocks;
  std::size_t free_blocks;
  std::size_t allocated_blocks;
  std::size_t largest_free;
  std::size_t smallest_free;
  std::size_t total_fragmentation;
};
/*
## pmm-managerinfo
*/
struct ManagerInfo {
  std::uint64_t magic;
  std::size_t total_size;
  std::size_t used_size;
  std::size_t block_count;
  std::size_t free_count;
  std::size_t alloc_count;
  std::ptrdiff_t first_block_offset;
  std::ptrdiff_t first_free_offset;
  std::size_t manager_header_size;
};
/*
## pmm-blockview
*/
struct BlockView {
  std::size_t index;
  std::ptrdiff_t offset;
  std::size_t total_size;
  std::size_t header_size;
  std::size_t user_size;
  std::size_t alignment;
  bool used;
};
/*
## pmm-freeblockview
*/
struct FreeBlockView {
  std::ptrdiff_t offset;
  std::size_t total_size;
  std::size_t free_size;
  std::ptrdiff_t left_offset;
  std::ptrdiff_t right_offset;
  std::ptrdiff_t parent_offset;
  int avl_height;
  int avl_depth;
};
namespace detail {
inline constexpr std::uint8_t kLegacyUnversionedImageVersion = 0;
inline constexpr std::uint8_t kCurrentImageVersion = 1;
inline constexpr bool
is_supported_image_version(std::uint8_t image_version) noexcept {
  return image_version == kLegacyUnversionedImageVersion ||
         image_version == kCurrentImageVersion;
}
inline constexpr bool
image_version_requires_migration(std::uint8_t image_version) noexcept {
  return image_version == kLegacyUnversionedImageVersion;
}
inline std::uint32_t crc32_accumulate_byte(std::uint32_t crc,
                                           std::uint8_t byte) noexcept {
  crc ^= byte;
  for (int bit = 0; bit < 8; ++bit)
    crc = (crc >> 1) ^ (0xEDB88320U & (~(crc & 1U) + 1U));
  return crc;
}
inline std::uint32_t compute_crc32(const std::uint8_t *data,
                                   std::size_t length) noexcept {
  std::uint32_t crc = 0xFFFFFFFFU;
  for (std::size_t i = 0; i < length; ++i)
    crc = crc32_accumulate_byte(crc, data[i]);
  return crc ^ 0xFFFFFFFFU;
}
static_assert(sizeof(pmm::Block<pmm::DefaultAddressTraits>) == 32,
              "Block<DefaultAddressTraits> must be 32 bytes ");
static_assert(sizeof(pmm::Block<pmm::DefaultAddressTraits>) % kGranuleSize == 0,
              "Block<DefaultAddressTraits> must be granule-aligned ");
static_assert(sizeof(pmm::Block<pmm::DefaultAddressTraits>) ==
                  sizeof(pmm::TreeNode<pmm::DefaultAddressTraits>) +
                      2 * sizeof(std::uint32_t),
              "Block<DefaultAddressTraits> must have TreeNode + 2 index_type "
              "list fields ");
static_assert(sizeof(pmm::TreeNode<pmm::DefaultAddressTraits>) ==
                  5 * sizeof(std::uint32_t) + 4,
              "TreeNode<DefaultAddressTraits> must be 24 bytes ");
inline constexpr std::uint32_t kNoBlock = 0xFFFFFFFFU;
static_assert(kNoBlock == pmm::DefaultAddressTraits::no_block,
              "kNoBlock must match DefaultAddressTraits::no_block ");
template <typename AddressTraitsT>
inline constexpr typename AddressTraitsT::index_type kNoBlock_v =
    AddressTraitsT::no_block;
template <typename AddressTraitsT>
inline constexpr typename AddressTraitsT::index_type kNullIdx_v =
    static_cast<typename AddressTraitsT::index_type>(0);
/*
### pmm-detail-managerheader
*/
template <typename AddressTraitsT = DefaultAddressTraits> struct ManagerHeader {
  using index_type = typename AddressTraitsT::index_type;
  std::uint64_t magic;
  std::uint64_t total_size;
  index_type used_size;
  index_type block_count;
  index_type free_count;
  index_type alloc_count;
  index_type first_block_offset;
  index_type last_block_offset;
  index_type free_tree_root;
  bool owns_memory;
  std::uint8_t image_version;
  std::uint16_t granule_size;
  std::uint64_t prev_total_size;
  std::uint32_t crc32;
  index_type root_offset;
};
static_assert(sizeof(ManagerHeader<DefaultAddressTraits>) == 64,
              "ManagerHeader<DefaultAddressTraits> must be exactly 64 bytes ");
static_assert(sizeof(ManagerHeader<DefaultAddressTraits>) % kGranuleSize == 0,
              "ManagerHeader<DefaultAddressTraits> must be granule-aligned ");
template <typename AddressTraitsT>
inline constexpr typename AddressTraitsT::index_type kBlockHeaderGranules_t =
    static_cast<typename AddressTraitsT::index_type>(
        (sizeof(pmm::Block<AddressTraitsT>) + AddressTraitsT::granule_size -
         1) /
        AddressTraitsT::granule_size);
template <typename AddressTraitsT>
inline constexpr std::size_t manager_header_offset_bytes_v =
    static_cast<std::size_t>(kBlockHeaderGranules_t<AddressTraitsT>) *
    AddressTraitsT::granule_size;
template <typename AddressTraitsT>
inline ManagerHeader<AddressTraitsT> *manager_header_at(std::uint8_t *base) noexcept { return reinterpret_cast<ManagerHeader<AddressTraitsT> *>(base + manager_header_offset_bytes_v<AddressTraitsT>); }
template <typename AddressTraitsT>
inline const ManagerHeader<AddressTraitsT> *manager_header_at(const std::uint8_t *base) noexcept { return reinterpret_cast<const ManagerHeader<AddressTraitsT> *>(base + manager_header_offset_bytes_v<AddressTraitsT>); }
template <typename AddressTraitsT>
inline std::uint32_t compute_image_crc32(const std::uint8_t *data,
                                         std::size_t length) noexcept {
  constexpr std::size_t kHdrOffset =
      manager_header_offset_bytes_v<AddressTraitsT>;
  constexpr std::size_t kCrcOffset =
      kHdrOffset + offsetof(ManagerHeader<AddressTraitsT>, crc32);
  constexpr std::size_t kCrcSize = sizeof(std::uint32_t);
  constexpr std::size_t kAfterCrc = kCrcOffset + kCrcSize;
  std::uint32_t crc = 0xFFFFFFFFU;
  for (std::size_t i = 0; i < kCrcOffset && i < length; ++i)
    crc = crc32_accumulate_byte(crc, data[i]);
  for (std::size_t i = 0; i < kCrcSize; ++i)
    crc = crc32_accumulate_byte(crc, 0x00U);
  for (std::size_t i = kAfterCrc; i < length; ++i)
    crc = crc32_accumulate_byte(crc, data[i]);
  return crc ^ 0xFFFFFFFFU;
}
inline constexpr std::uint32_t kManagerHeaderGranules =
    sizeof(ManagerHeader<DefaultAddressTraits>) / kGranuleSize;
inline constexpr std::size_t kMinBlockSize =
    sizeof(pmm::Block<pmm::DefaultAddressTraits>) + kGranuleSize;
inline constexpr std::size_t kMinMemorySize =
    sizeof(pmm::Block<pmm::DefaultAddressTraits>) +
    sizeof(ManagerHeader<pmm::DefaultAddressTraits>) +
    sizeof(pmm::Block<pmm::DefaultAddressTraits>) + kMinBlockSize;
template <typename AddressTraitsT>
inline typename AddressTraitsT::index_type
bytes_to_granules_t(std::size_t bytes) {
  using IndexT = typename AddressTraitsT::index_type;
  static constexpr std::size_t kGranSz = AddressTraitsT::granule_size;
  if (bytes > std::numeric_limits<std::size_t>::max() - (kGranSz - 1))
    return static_cast<IndexT>(0);
  std::size_t granules = (bytes + kGranSz - 1) / kGranSz;
  if (granules > static_cast<std::size_t>(std::numeric_limits<IndexT>::max()))
    return static_cast<IndexT>(0);
  return static_cast<IndexT>(granules);
}
template <typename AddressTraitsT>
inline typename AddressTraitsT::index_type bytes_to_idx_t(std::size_t bytes) {
  static constexpr std::size_t kGranSz = AddressTraitsT::granule_size;
  using IndexT = typename AddressTraitsT::index_type;
  if (bytes == 0)
    return static_cast<IndexT>(0);
  if (bytes > std::numeric_limits<std::size_t>::max() - (kGranSz - 1))
    return AddressTraitsT::no_block;
  std::size_t granules = (bytes + kGranSz - 1) / kGranSz;
  if (granules > static_cast<std::size_t>(std::numeric_limits<IndexT>::max()))
    return AddressTraitsT::no_block;
  return static_cast<IndexT>(granules);
}
template <typename AddressTraitsT>
inline std::size_t idx_to_byte_off_t(typename AddressTraitsT::index_type idx) { return static_cast<std::size_t>(idx) * AddressTraitsT::granule_size; }
template <typename AddressTraitsT>
inline typename AddressTraitsT::index_type
byte_off_to_idx_t(std::size_t byte_off) {
  using IndexT = typename AddressTraitsT::index_type;
  assert(byte_off % AddressTraitsT::granule_size == 0);
  assert(byte_off / AddressTraitsT::granule_size <=
         static_cast<std::size_t>(std::numeric_limits<IndexT>::max()));
  return static_cast<IndexT>(byte_off / AddressTraitsT::granule_size);
}
inline bool is_valid_alignment(std::size_t align) { return align == kGranuleSize; }
template <typename AddressTraitsT = pmm::DefaultAddressTraits>
inline pmm::Block<AddressTraitsT> *block_at(std::uint8_t *base, typename AddressTraitsT::index_type idx) { assert(idx != kNoBlock_v<AddressTraitsT>); return reinterpret_cast<pmm::Block<AddressTraitsT> *>(base + static_cast<std::size_t>(idx) * AddressTraitsT::granule_size); }
template <typename AddressTraitsT = pmm::DefaultAddressTraits>
inline const pmm::Block<AddressTraitsT> *block_at(const std::uint8_t *base, typename AddressTraitsT::index_type idx) { assert(idx != kNoBlock_v<AddressTraitsT>); return reinterpret_cast<const pmm::Block<AddressTraitsT> *>(base + static_cast<std::size_t>(idx) * AddressTraitsT::granule_size); }
template <typename AddressTraitsT = pmm::DefaultAddressTraits>
inline pmm::Block<AddressTraitsT> *
block_at_checked(std::uint8_t *base, std::size_t total_size,
                 typename AddressTraitsT::index_type idx) noexcept {
  if (!validate_block_index<AddressTraitsT>(total_size, idx))
    return nullptr;
  return reinterpret_cast<pmm::Block<AddressTraitsT> *>(base + static_cast<std::size_t>(idx) * AddressTraitsT::granule_size);
}
template <typename AddressTraitsT = pmm::DefaultAddressTraits>
inline const pmm::Block<AddressTraitsT> *
block_at_checked(const std::uint8_t *base, std::size_t total_size,
                 typename AddressTraitsT::index_type idx) noexcept {
  if (!validate_block_index<AddressTraitsT>(total_size, idx))
    return nullptr;
  return reinterpret_cast<const pmm::Block<AddressTraitsT> *>(base + static_cast<std::size_t>(idx) * AddressTraitsT::granule_size);
}
template <typename AddressTraitsT>
inline typename AddressTraitsT::index_type
block_idx_t(const std::uint8_t *base, const pmm::Block<AddressTraitsT> *block) {
  std::size_t byte_off = reinterpret_cast<const std::uint8_t *>(block) - base;
  assert(byte_off % AddressTraitsT::granule_size == 0);
  return static_cast<typename AddressTraitsT::index_type>(byte_off / AddressTraitsT::granule_size);
}
template <typename AddressTraitsT>
inline constexpr typename AddressTraitsT::index_type kManagerHeaderGranules_t =
    static_cast<typename AddressTraitsT::index_type>(
        (sizeof(ManagerHeader<AddressTraitsT>) + AddressTraitsT::granule_size -
         1) /
        AddressTraitsT::granule_size);
template <typename AddressTraitsT>
inline typename AddressTraitsT::index_type
block_total_granules(const std::uint8_t *base,
                     const ManagerHeader<AddressTraitsT> *hdr,
                     const pmm::Block<AddressTraitsT> *blk) {
  using BlockState = pmm::BlockStateBase<AddressTraitsT>;
  static constexpr std::size_t kGranSz = AddressTraitsT::granule_size;
  using IndexT = typename AddressTraitsT::index_type;
  static constexpr IndexT kNoBlk = AddressTraitsT::no_block;
  std::size_t byte_off = reinterpret_cast<const std::uint8_t *>(blk) - base;
  IndexT this_idx = static_cast<IndexT>(byte_off / kGranSz);
  IndexT next_off = BlockState::get_next_offset(blk);
  IndexT total_gran = static_cast<IndexT>(hdr->total_size / kGranSz);
  if (next_off != kNoBlk)
    return static_cast<IndexT>(next_off - this_idx);
  return static_cast<IndexT>(total_gran - this_idx);
}
template <typename AddressTraitsT>
inline void *resolve_granule_ptr(std::uint8_t *base, typename AddressTraitsT::index_type idx) noexcept { return (idx == static_cast<typename AddressTraitsT::index_type>(0)) ? nullptr : base + static_cast<std::size_t>(idx) * AddressTraitsT::granule_size; }
template <typename AddressTraitsT>
inline void *
resolve_granule_ptr_checked(std::uint8_t *base, std::size_t total_size,
                            typename AddressTraitsT::index_type idx) noexcept {
  if (idx == static_cast<typename AddressTraitsT::index_type>(0))
    return nullptr;
  std::size_t byte_off =
      static_cast<std::size_t>(idx) * AddressTraitsT::granule_size;
  if (byte_off >= total_size)
    return nullptr;
  return base + byte_off;
}
template <typename AddressTraitsT>
inline typename AddressTraitsT::index_type ptr_to_granule_idx(const std::uint8_t *base, const void *ptr) noexcept { return static_cast<typename AddressTraitsT::index_type>((static_cast<const std::uint8_t *>(ptr) - base) / AddressTraitsT::granule_size); }
template <typename AddressTraitsT>
inline typename AddressTraitsT::index_type
ptr_to_granule_idx_checked(const std::uint8_t *base, std::size_t total_size,
                           const void *ptr) noexcept {
  using IndexT = typename AddressTraitsT::index_type;
  if (ptr == nullptr || base == nullptr)
    return AddressTraitsT::no_block;
  const auto *raw = static_cast<const std::uint8_t *>(ptr);
  if (raw < base || raw >= base + total_size)
    return AddressTraitsT::no_block;
  std::size_t byte_off = static_cast<std::size_t>(raw - base);
  if (byte_off % AddressTraitsT::granule_size != 0)
    return AddressTraitsT::no_block;
  std::size_t idx = byte_off / AddressTraitsT::granule_size;
  if (idx > static_cast<std::size_t>(std::numeric_limits<IndexT>::max()))
    return AddressTraitsT::no_block;
  return static_cast<IndexT>(idx);
}
template <typename AddressTraitsT = pmm::DefaultAddressTraits>
inline void *user_ptr(pmm::Block<AddressTraitsT> *block) { return reinterpret_cast<std::uint8_t *>(block) + sizeof(pmm::Block<AddressTraitsT>); }
template <typename AddressTraitsT>
inline bool is_block_header_linked_in_canonical_chain(
    const std::uint8_t *base, const ManagerHeader<AddressTraitsT> *hdr,
    std::size_t total_size,
    typename AddressTraitsT::index_type cand_idx) noexcept {
  using BlockState = pmm::BlockStateBase<AddressTraitsT>;
  using IndexT = typename AddressTraitsT::index_type;
  if (base == nullptr || hdr == nullptr)
    return false;
  if (hdr->block_count == 0 ||
      hdr->first_block_offset == AddressTraitsT::no_block)
    return false;
  if (!validate_block_index<AddressTraitsT>(total_size,
                                            hdr->first_block_offset) ||
      !validate_block_index<AddressTraitsT>(total_size, hdr->last_block_offset))
    return false;
  const void *cand =
      base + static_cast<std::size_t>(cand_idx) * AddressTraitsT::granule_size;
  const IndexT prev = BlockState::get_prev_offset(cand);
  const IndexT next = BlockState::get_next_offset(cand);
  if (prev == AddressTraitsT::no_block) {
    if (cand_idx != hdr->first_block_offset)
      return false;
  } else {
    if (!validate_block_index<AddressTraitsT>(total_size, prev) ||
        prev >= cand_idx)
      return false;
    const void *prev_block =
        base + static_cast<std::size_t>(prev) * AddressTraitsT::granule_size;
    if (BlockState::get_next_offset(prev_block) != cand_idx)
      return false;
  }
  if (next == AddressTraitsT::no_block) {
    if (cand_idx != hdr->last_block_offset)
      return false;
  } else {
    if (!validate_block_index<AddressTraitsT>(total_size, next) ||
        next <= cand_idx)
      return false;
    const void *next_block =
        base + static_cast<std::size_t>(next) * AddressTraitsT::granule_size;
    if (BlockState::get_prev_offset(next_block) != cand_idx)
      return false;
  }
  return true;
}
template <typename AddressTraitsT>
inline bool
is_canonical_allocated_block_header(const std::uint8_t *base,
                                    std::size_t total_size,
                                    const std::uint8_t *cand_addr) noexcept {
  using BlockState = pmm::BlockStateBase<AddressTraitsT>;
  using IndexT = typename AddressTraitsT::index_type;
  if (base == nullptr || cand_addr == nullptr)
    return false;
  const std::uintptr_t base_addr = reinterpret_cast<std::uintptr_t>(base);
  const std::uintptr_t cand_raw = reinterpret_cast<std::uintptr_t>(cand_addr);
  if (cand_raw < base_addr)
    return false;
  const std::size_t cand_off = static_cast<std::size_t>(cand_raw - base_addr);
  if (cand_off % AddressTraitsT::granule_size != 0)
    return false;
  if (cand_off / AddressTraitsT::granule_size >
      static_cast<std::size_t>(std::numeric_limits<IndexT>::max()))
    return false;
  const IndexT cand_idx =
      static_cast<IndexT>(cand_off / AddressTraitsT::granule_size);
  if (!validate_block_index<AddressTraitsT>(total_size, cand_idx))
    return false;
  const IndexT weight = BlockState::get_weight(cand_addr);
  if (weight == 0 || BlockState::get_root_offset(cand_addr) != cand_idx)
    return false;
  const std::uint16_t node_type = BlockState::get_node_type(cand_addr);
  if (node_type != pmm::kNodeReadWrite && node_type != pmm::kNodeReadOnly)
    return false;
  if (total_size < manager_header_offset_bytes_v<AddressTraitsT> +
                       sizeof(ManagerHeader<AddressTraitsT>))
    return false;
  const auto *hdr = manager_header_at<AddressTraitsT>(base);
  if (hdr->total_size != total_size)
    return false;
  if (!is_block_header_linked_in_canonical_chain<AddressTraitsT>(
          base, hdr, total_size, cand_idx))
    return false;
  constexpr std::size_t kBlockSize = sizeof(pmm::Block<AddressTraitsT>);
  if (cand_off > total_size - kBlockSize)
    return false;
  const std::size_t data_start = cand_off + kBlockSize;
  if (static_cast<std::size_t>(weight) >
      (std::numeric_limits<std::size_t>::max)() / AddressTraitsT::granule_size)
    return false;
  const std::size_t data_bytes =
      static_cast<std::size_t>(weight) * AddressTraitsT::granule_size;
  if (data_bytes > total_size - data_start)
    return false;
  return true;
}
template <typename AddressTraitsT>
inline bool is_canonical_user_ptr(const std::uint8_t *base,
                                  std::size_t total_size,
                                  const void *ptr) noexcept {
  constexpr std::size_t kBlockSize = sizeof(pmm::Block<AddressTraitsT>);
  const std::size_t min_user_offset =
      kBlockSize + sizeof(ManagerHeader<AddressTraitsT>) + kBlockSize;
  if (!validate_user_ptr<AddressTraitsT>(base, total_size, ptr,
                                         min_user_offset))
    return false;
  const auto *raw_ptr = static_cast<const std::uint8_t *>(ptr);
  const auto *cand_addr = raw_ptr - kBlockSize;
  if (cand_addr + kBlockSize != raw_ptr)
    return false;
  return is_canonical_allocated_block_header<AddressTraitsT>(base, total_size,
                                                             cand_addr);
}
template <typename AddressTraitsT>
inline pmm::Block<AddressTraitsT> *
header_from_ptr_t(std::uint8_t *base, void *ptr, std::size_t total_size) {
  static constexpr std::size_t kBlockSize = sizeof(pmm::Block<AddressTraitsT>);
  if (!is_canonical_user_ptr<AddressTraitsT>(base, total_size, ptr))
    return nullptr;
  std::uint8_t *cand_addr = static_cast<std::uint8_t *>(ptr) - kBlockSize;
  return reinterpret_cast<pmm::Block<AddressTraitsT> *>(cand_addr);
}
template <typename AddressTraitsT>
inline typename AddressTraitsT::index_type
required_block_granules_t(std::size_t user_bytes) {
  using index_type = typename AddressTraitsT::index_type;
  index_type data_granules = bytes_to_granules_t<AddressTraitsT>(user_bytes);
  if (data_granules == 0)
    data_granules = 1;
  return kBlockHeaderGranules_t<AddressTraitsT> + data_granules;
}
}
}
