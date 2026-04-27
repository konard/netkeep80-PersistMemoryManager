#pragma once
#include "pmm/address_traits.h"
#include "pmm/storage_backend.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
namespace pmm
{
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
        _buffer        = static_cast<uint8_t*>( std::malloc( aligned ) );
        if ( _buffer != nullptr )
        {
            _size        = aligned;
            _owns_memory = true;
        }
    }
    HeapStorage( const HeapStorage& )            = delete;
    HeapStorage& operator=( const HeapStorage& ) = delete;
    HeapStorage( HeapStorage&& other ) noexcept
        : _buffer( other._buffer ), _size( other._size ), _owns_memory( other._owns_memory )
    {
        other._buffer      = nullptr;
        other._size        = 0;
        other._owns_memory = false;
    }
    HeapStorage& operator=( HeapStorage&& other ) noexcept
    {
        if ( this != &other )
        {
            if ( _owns_memory && _buffer != nullptr )
                std::free( _buffer );
            _buffer            = other._buffer;
            _size              = other._size;
            _owns_memory       = other._owns_memory;
            other._buffer      = nullptr;
            other._size        = 0;
            other._owns_memory = false;
        }
        return *this;
    }
    ~HeapStorage()
    {
        if ( _owns_memory && _buffer != nullptr )
            std::free( _buffer );
    }
    void attach( void* memory, size_t size ) noexcept
    {
        if ( _owns_memory && _buffer != nullptr )
            std::free( _buffer );
        _buffer      = static_cast<uint8_t*>( memory );
        _size        = size;
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
        void* new_buf = std::malloc( new_size );
        if ( new_buf == nullptr )
            return false;
        if ( _buffer != nullptr )
            std::memcpy( new_buf, _buffer, _size );
        if ( _owns_memory && _buffer != nullptr )
            std::free( _buffer );
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
