#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace pmm
{

namespace detail
{

/*
## pmm::detail::manager_index_type
*/
template <typename ManagerT> struct manager_index_type
{

    using type = std::uint32_t;
};

template <typename ManagerT>
    requires requires { typename ManagerT::address_traits::index_type; }
struct manager_index_type<ManagerT>
{

    using type = typename ManagerT::address_traits::index_type;
};

}

template <class T, class ManagerT>
    requires( !std::is_void_v<ManagerT> )

/*
## pmm::pptr
*/
class pptr
{

  public:

    using element_type = T;

    using manager_type = ManagerT;

    using index_type = typename detail::manager_index_type<ManagerT>::type;

  private:

    index_type _idx;

  public:

/*
### pmm::pptr::pptr
*/
    constexpr pptr() noexcept : _idx( 0 ) {}
    constexpr explicit pptr( index_type idx ) noexcept : _idx( idx ) {}
    constexpr pptr( const pptr& ) noexcept            = default;

    constexpr pptr& operator=( const pptr& ) noexcept = default;
    ~pptr() noexcept                                  = default;

/*
### pmm::pptr::operator_increment
*/
    pptr& operator++()      = delete;
    pptr  operator++( int ) = delete;
/*
### pmm::pptr::operator_decrement
*/
    pptr& operator--()      = delete;
    pptr  operator--( int ) = delete;

/*
### pmm::pptr::is_null
*/
    constexpr bool is_null() const noexcept { return _idx == 0; }

/*
### pmm::pptr::operator_bool
*/
    constexpr explicit operator bool() const noexcept { return _idx != 0; }

/*
### pmm::pptr::offset
*/
    constexpr index_type offset() const noexcept { return _idx; }

/*
### pmm::pptr::byte_offset
*/
    constexpr std::size_t byte_offset() const noexcept
    {
        return static_cast<std::size_t>( _idx ) * ManagerT::address_traits::granule_size;
    }

    constexpr bool operator==( const pptr& other ) const noexcept { return _idx == other._idx; }

    constexpr bool operator!=( const pptr& other ) const noexcept { return _idx != other._idx; }

/*
### pmm::pptr::operator_less
*/
    bool operator<( const pptr& other ) const noexcept
    {

        static_assert(
            requires( const T& a, const T& b ) {
                { a < b } -> std::convertible_to<bool>;
            }, "pptr<T>::operator< requires T to support operator<. "

               "Provide bool operator<(const T&, const T&) or use pptr::offset() for index-based ordering." );

        if ( is_null() && !other.is_null() )
            return true;
        if ( !is_null() && other.is_null() )
            return false;
        if ( is_null() && other.is_null() )
            return false;

        return **this < *other;
    }

/*
### pmm::pptr::operator_deref
*/
    T& operator*() const noexcept { return *ManagerT::template resolve_checked<T>( *this ); }

/*
### pmm::pptr::operator_arrow
*/
    T* operator->() const noexcept { return ManagerT::template resolve_checked<T>( *this ); }

/*
### pmm::pptr::resolve
*/
    T* resolve() const noexcept { return ManagerT::template resolve_checked<T>( *this ); }

/*
### pmm::pptr::resolve_unchecked
*/
    T* resolve_unchecked() const noexcept { return ManagerT::template resolve_unchecked<T>( *this ); }

/*
### pmm::pptr::tree_node
*/
    auto& tree_node() const noexcept { return ManagerT::tree_node( *this ); }
};

}
