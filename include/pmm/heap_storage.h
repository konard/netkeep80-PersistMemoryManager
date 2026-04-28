#pragma once
#include "pmm/address_traits.h"
#include "pmm/storage_backend.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#if defined( _WIN32 )
#include <malloc.h>
#endif
namespace pmm
{
namespace detail
{
/*
### pmm-detail-alignedalloc
*/
inline void* aligned_alloc_for_arena( size_t a, size_t s ) noexcept
{
    if ( a == 0 || ( a & ( a - 1 ) ) != 0 || s == 0 )
        return nullptr;
    size_t r = ( ( s + a - 1 ) / a ) * a;
#if defined( _WIN32 )
    return _aligned_malloc( r, a );
#else
    void*  p = nullptr;
    size_t e = a < sizeof( void* ) ? sizeof( void* ) : a;
    return ( posix_memalign( &p, e, r ) == 0 ) ? p : nullptr;
#endif
}
inline void aligned_free_for_arena( void* p ) noexcept
{
    if ( !p )
        return;
#if defined( _WIN32 )
    _aligned_free( p );
#else
    std::free( p );
#endif
}
}
/*
## pmm-heapstorage
*/
template <typename AT = DefaultAddressTraits> class HeapStorage
{
  public:
    using address_traits   = AT;
    HeapStorage() noexcept = default;
    explicit HeapStorage( size_t initial_size ) noexcept
    {
        if ( initial_size == 0 )
            return;
        size_t aligned = ( ( initial_size + AT::granule_size - 1 ) / AT::granule_size ) * AT::granule_size;
        _buffer        = static_cast<uint8_t*>( detail::aligned_alloc_for_arena( AT::granule_size, aligned ) );
        if ( _buffer )
        {
            _size        = aligned;
            _owns_memory = true;
        }
        assert( reinterpret_cast<std::uintptr_t>( _buffer ) % AT::granule_size == 0 );
    }
    HeapStorage( const HeapStorage& )            = delete;
    HeapStorage& operator=( const HeapStorage& ) = delete;
    HeapStorage( HeapStorage&& o ) noexcept : _buffer( o._buffer ), _size( o._size ), _owns_memory( o._owns_memory )
    {
        o._buffer      = nullptr;
        o._size        = 0;
        o._owns_memory = false;
    }
    HeapStorage& operator=( HeapStorage&& o ) noexcept
    {
        if ( this != &o )
        {
            if ( _owns_memory && _buffer )
                detail::aligned_free_for_arena( _buffer );
            _buffer        = o._buffer;
            _size          = o._size;
            _owns_memory   = o._owns_memory;
            o._buffer      = nullptr;
            o._size        = 0;
            o._owns_memory = false;
        }
        return *this;
    }
    ~HeapStorage()
    {
        if ( _owns_memory && _buffer )
            detail::aligned_free_for_arena( _buffer );
    }
    void attach( void* m, size_t s ) noexcept
    {
        if ( _owns_memory && _buffer )
            detail::aligned_free_for_arena( _buffer );
        _buffer      = static_cast<uint8_t*>( m );
        _size        = s;
        _owns_memory = false;
    }
    uint8_t*       base_ptr() noexcept { return _buffer; }
    const uint8_t* base_ptr() const noexcept { return _buffer; }
    size_t         total_size() const noexcept { return _size; }
    bool           expand( size_t additional_bytes ) noexcept
    {
        if ( additional_bytes == 0 )
            return _size > 0;
        static constexpr size_t kMinInitialSize = 4096;
        size_t                  growth =
            ( _size > 0 ) ? ( _size / 4 + additional_bytes ) : std::max( additional_bytes, kMinInitialSize );
        size_t new_size = _size + growth;
        new_size        = ( ( new_size + AT::granule_size - 1 ) / AT::granule_size ) * AT::granule_size;
        if ( new_size <= _size )
            return false;
        void* new_buf = detail::aligned_alloc_for_arena( AT::granule_size, new_size );
        if ( !new_buf )
            return false;
        assert( reinterpret_cast<std::uintptr_t>( new_buf ) % AT::granule_size == 0 );
        if ( _buffer )
            std::memcpy( new_buf, _buffer, _size );
        if ( _owns_memory && _buffer )
            detail::aligned_free_for_arena( _buffer );
        _buffer      = static_cast<uint8_t*>( new_buf );
        _size        = new_size;
        _owns_memory = true;
        return true;
    }
    bool owns_memory() const noexcept { return _owns_memory; }

  private:
    uint8_t* _buffer      = nullptr;
    size_t   _size        = 0;
    bool     _owns_memory = false;
};
static_assert( is_storage_backend_v<HeapStorage<>>, "" );
}
