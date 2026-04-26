#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace pmm {

/*
## pmm::AddressTraits
*/
template <typename IndexT, std::size_t GranuleSz> struct AddressTraits {
  static_assert(std::is_unsigned<IndexT>::value,
                "AddressTraits: IndexT must be an unsigned integer type");
  static_assert(
      GranuleSz >= 4,
      "AddressTraits: GranuleSz must be >= 4 (minimum architecture word size)");
  static_assert((GranuleSz & (GranuleSz - 1)) == 0,
                "AddressTraits: GranuleSz must be a power of 2");

  using index_type = IndexT;

  static constexpr std::size_t granule_size = GranuleSz;

  static constexpr index_type no_block = std::numeric_limits<IndexT>::max();

  /*
  ### pmm::AddressTraits::bytes_to_granules
  */
  static constexpr index_type bytes_to_granules(std::size_t bytes) noexcept {
    if (bytes == 0)
      return static_cast<index_type>(0);

    if (bytes > std::numeric_limits<std::size_t>::max() - (granule_size - 1))
      return static_cast<index_type>(0);

    std::size_t granules = (bytes + granule_size - 1) / granule_size;
    if (granules > static_cast<std::size_t>(std::numeric_limits<IndexT>::max()))
      return static_cast<index_type>(0);
    return static_cast<index_type>(granules);
  }

  /*
  ### pmm::AddressTraits::granules_to_bytes
  */
  static constexpr std::size_t granules_to_bytes(index_type granules) noexcept {
    return static_cast<std::size_t>(granules) * granule_size;
  }

  /*
  ### pmm::AddressTraits::idx_to_byte_off
  */
  static constexpr std::size_t idx_to_byte_off(index_type idx) noexcept {
    return static_cast<std::size_t>(idx) * granule_size;
  }

  /*
  ### pmm::AddressTraits::byte_off_to_idx
  */
  static index_type byte_off_to_idx(std::size_t byte_off) noexcept {

    assert(byte_off % granule_size == 0);
    assert(byte_off / granule_size <=
           static_cast<std::size_t>(std::numeric_limits<IndexT>::max()));
    return static_cast<index_type>(byte_off / granule_size);
  }
};

using SmallAddressTraits = AddressTraits<std::uint16_t, 16>;

using DefaultAddressTraits = AddressTraits<std::uint32_t, 16>;

using LargeAddressTraits = AddressTraits<std::uint64_t, 64>;

}
