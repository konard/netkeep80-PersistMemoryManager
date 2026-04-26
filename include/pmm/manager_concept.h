#pragma once
#include <concepts>
#include <cstddef>
#include <type_traits>
namespace pmm{using std::size_t;template<typename T>concept PersistMemoryManagerConcept=requires{typename T::manager_type;typename T::address_traits;typename T::storage_backend;{T::is_initialized()};{T::allocate(size_t{})}->std::convertible_to<void*>;{T::deallocate(static_cast<void*>(nullptr))};{T::total_size()}->std::convertible_to<size_t>;{T::destroy()};};template<typename T>struct is_persist_memory_manager:std::bool_constant<PersistMemoryManagerConcept<T>>{};template<typename T>inline constexpr bool is_persist_memory_manager_v=PersistMemoryManagerConcept<T>;}
