#pragma once

#include "pmm/address_traits.h"
#include "pmm/storage_backend.h"

#include <cstddef>
#include <cstdint>

namespace pmm {

/*
## pmm::StaticStorage
*/
template <std::size_t Size, typename AddressTraitsT = DefaultAddressTraits>
class StaticStorage {
  static_assert(Size > 0, "StaticStorage: Size must be > 0");
  static_assert(Size % AddressTraitsT::granule_size == 0,
                "StaticStorage: Size must be a multiple of granule_size");

public:
  using address_traits = AddressTraitsT;

  /*
### pmm::StaticStorage::StaticStorage
  */
  StaticStorage() noexcept = default;
  StaticStorage(const StaticStorage &) = delete;

  StaticStorage &operator=(const StaticStorage &) = delete;
  StaticStorage(StaticStorage &&) = delete;
  StaticStorage &operator=(StaticStorage &&) = delete;

  /*
### pmm::StaticStorage::base_ptr
  */
  std::uint8_t *base_ptr() noexcept { return _buffer; }
  const std::uint8_t *base_ptr() const noexcept { return _buffer; }

  /*
### pmm::StaticStorage::total_size
  */
  constexpr std::size_t total_size() const noexcept { return Size; }

  /*
### pmm::StaticStorage::expand
  */
  bool expand(std::size_t) noexcept { return false; }

  /*
### pmm::StaticStorage::owns_memory
  */
  constexpr bool owns_memory() const noexcept { return false; }

private:
  alignas(AddressTraitsT::granule_size) std::uint8_t _buffer[Size]{};
};

static_assert(is_storage_backend_v<StaticStorage<64>>,
              "StaticStorage must satisfy StorageBackendConcept");

}
