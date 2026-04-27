#pragma once
#include <type_traits>
#include <utility>
namespace pmm
{
template <typename T>
concept HasFreeData = requires( T& t ) {
    { t.free_data() } noexcept;
};
template <typename T>
concept HasFreeAll = requires( T& t ) {
    { t.free_all() } noexcept;
};
template <typename T>
concept HasPersistentCleanup = HasFreeData<T> || HasFreeAll<T>;
template <typename T, typename ManagerT> class typed_guard
{
  public:
    using pptr_type = typename ManagerT::template pptr<T>;
    explicit typed_guard( pptr_type p ) noexcept : _ptr( p ) {}
    typed_guard() noexcept                       = default;
    typed_guard( const typed_guard& )            = delete;
    typed_guard& operator=( const typed_guard& ) = delete;
    typed_guard( typed_guard&& other ) noexcept : _ptr( other._ptr ) { other._ptr = pptr_type(); }
    typed_guard& operator=( typed_guard&& other ) noexcept
    {
        if ( this != &other )
        {
            reset();
            _ptr       = other._ptr;
            other._ptr = pptr_type();
        }
        return *this;
    }
    ~typed_guard() { reset(); }
    void reset() noexcept
    {
        if ( !_ptr.is_null() )
        {
            cleanup( *_ptr );
            ManagerT::destroy_typed( _ptr );
            _ptr = pptr_type();
        }
    }
    pptr_type release() noexcept
    {
        pptr_type p = _ptr;
        _ptr        = pptr_type();
        return p;
    }
    T*        operator->() const noexcept { return &( *_ptr ); }
    T&        operator*() const noexcept { return *_ptr; }
    pptr_type get() const noexcept { return _ptr; }
    explicit  operator bool() const noexcept { return !_ptr.is_null(); }

  private:
    pptr_type   _ptr;
    static void cleanup( T& obj ) noexcept
    {
        if constexpr ( HasFreeData<T> )
            obj.free_data();
        else if constexpr ( HasFreeAll<T> )
            obj.free_all();
    }
};
}
