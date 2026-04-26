#pragma once
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>
namespace pmm{using std::size_t;using std::uint8_t;template<typename Backend>concept StorageBackendConcept=requires(Backend&b,const Backend&cb,size_t n){{b.base_ptr()}->std::convertible_to<uint8_t*>;{cb.total_size()}->std::convertible_to<size_t>;{b.expand(n)}->std::convertible_to<bool>;{cb.owns_memory()}->std::convertible_to<bool>;};template<typename Backend>inline constexpr bool is_storage_backend_v=StorageBackendConcept<Backend>;}
