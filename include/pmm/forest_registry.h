#pragma once

#include "pmm/address_traits.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace pmm::detail
{

inline constexpr std::size_t kForestDomainNameCapacity = 48;
inline constexpr std::size_t kMaxForestDomains         = 32;

inline constexpr const char*   kSystemDomainFreeTree         = "system/free_tree";
inline constexpr const char*   kSystemDomainSymbols          = "system/symbols";
inline constexpr const char*   kSystemDomainRegistry         = "system/domain_registry";
inline constexpr const char*   kSystemTypeForestRegistry     = "type/forest_registry";
inline constexpr const char*   kSystemTypeForestDomainRecord = "type/forest_domain_record";
inline constexpr const char*   kSystemTypePstringview        = "type/pstringview";
inline constexpr const char*   kServiceNameLegacyRoot        = "service/legacy_root";
inline constexpr const char*   kServiceNameDomainRoot        = "service/domain_root";
inline constexpr const char*   kServiceNameDomainSymbol      = "service/domain_symbol";
inline constexpr const char*   kContainerDomainPmap          = "container/pmap";
inline constexpr std::uint32_t kForestRegistryMagic          = 0x50465247U; // "PFRG"
inline constexpr std::uint16_t kForestRegistryVersion        = 1;
inline constexpr std::uint8_t  kForestBindingDirectRoot      = 0;
inline constexpr std::uint8_t  kForestBindingFreeTree        = 1;
inline constexpr std::uint8_t  kForestDomainFlagSystem       = 0x01;
inline constexpr char          kPmapGeneratedDomainKind      = 'g';
inline constexpr char          kPmapNamedDomainKind          = 'n';

inline bool forest_domain_name_fits( const char* name ) noexcept;

constexpr std::uint32_t pmap_fnv1a_32_append( std::uint32_t hash, std::string_view value ) noexcept
{
    for ( char ch : value )
    {
        hash ^= static_cast<std::uint8_t>( ch );
        hash *= 16777619u;
    }
    return hash;
}

template <typename T> constexpr std::string_view pmap_type_signature() noexcept
{
#if defined( __clang__ ) || defined( __GNUC__ )
    return __PRETTY_FUNCTION__;
#elif defined( _MSC_VER )
    return __FUNCSIG__;
#else
    return "pmm::detail::pmap_type_signature";
#endif
}

template <typename _K, typename _V> constexpr std::uint32_t pmap_domain_type_hash() noexcept
{
    std::uint32_t hash = 2166136261u;
    hash               = pmap_fnv1a_32_append( hash, pmap_type_signature<_K>() );
    hash               = pmap_fnv1a_32_append( hash, "|" );
    hash               = pmap_fnv1a_32_append( hash, pmap_type_signature<_V>() );
    return hash;
}

inline std::uint64_t pmap_hash_domain_key( const char* key ) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    if ( key == nullptr )
        key = "";
    while ( *key != '\0' )
    {
        hash ^= static_cast<std::uint8_t>( *key );
        hash *= 1099511628211ull;
        ++key;
    }
    return hash;
}

inline char pmap_hex_digit( std::uint8_t value ) noexcept
{
    return static_cast<char>( value < 10 ? ( '0' + value ) : ( 'a' + ( value - 10 ) ) );
}

inline bool pmap_append_char( char* out, std::size_t& pos, char ch ) noexcept
{
    if ( out == nullptr || pos + 1 >= kForestDomainNameCapacity )
        return false;
    out[pos++] = ch;
    out[pos]   = '\0';
    return true;
}

inline bool pmap_append_literal( char* out, std::size_t& pos, const char* value ) noexcept
{
    if ( value == nullptr )
        return false;
    while ( *value != '\0' )
    {
        if ( !pmap_append_char( out, pos, *value ) )
            return false;
        ++value;
    }
    return true;
}

inline bool pmap_append_hex( char* out, std::size_t& pos, std::uint64_t value, unsigned digits ) noexcept
{
    for ( unsigned i = digits; i > 0; --i )
    {
        const unsigned shift = ( i - 1 ) * 4;
        if ( !pmap_append_char( out, pos,
                                pmap_hex_digit( static_cast<std::uint8_t>( ( value >> shift ) & 0x0full ) ) ) )
            return false;
    }
    return true;
}

inline bool pmap_write_domain_name( char* out, std::uint32_t type_hash, char kind, std::uint64_t value,
                                    unsigned value_digits ) noexcept
{
    if ( out == nullptr )
        return false;
    std::memset( out, 0, kForestDomainNameCapacity );

    std::size_t pos = 0;
    if ( !pmap_append_literal( out, pos, kContainerDomainPmap ) )
        return false;
    if ( !pmap_append_char( out, pos, '/' ) )
        return false;
    if ( !pmap_append_hex( out, pos, type_hash, 8 ) )
        return false;
    if ( !pmap_append_char( out, pos, '/' ) )
        return false;
    if ( !pmap_append_char( out, pos, kind ) )
        return false;
    if ( !pmap_append_hex( out, pos, value, value_digits ) )
        return false;

    return forest_domain_name_fits( out );
}

template <typename AddressTraitsT> struct ForestDomainRecord
{
    using index_type = typename AddressTraitsT::index_type;

    index_type    binding_id;
    index_type    root_offset;
    index_type    symbol_offset;
    std::uint8_t  binding_kind;
    std::uint8_t  flags;
    std::uint16_t reserved;
    char          name[kForestDomainNameCapacity];

    constexpr ForestDomainRecord() noexcept
        : binding_id( 0 ), root_offset( 0 ), symbol_offset( 0 ), binding_kind( kForestBindingDirectRoot ), flags( 0 ),
          reserved( 0 ), name{}
    {
    }
};

template <typename AddressTraitsT> struct ForestDomainRegistry
{
    using index_type = typename AddressTraitsT::index_type;

    std::uint32_t                      magic;
    std::uint16_t                      version;
    std::uint16_t                      domain_count;
    index_type                         reserved_root_offset;
    index_type                         next_binding_id;
    ForestDomainRecord<AddressTraitsT> domains[kMaxForestDomains];

    constexpr ForestDomainRegistry() noexcept
        : magic( kForestRegistryMagic ), version( kForestRegistryVersion ), domain_count( 0 ),
          reserved_root_offset( 0 ), next_binding_id( 1 ), domains{}
    {
    }
};

template <typename AddressTraitsT>
inline bool forest_domain_name_equals( const ForestDomainRecord<AddressTraitsT>& rec, const char* name ) noexcept
{
    if ( name == nullptr )
        return false;
    return std::strncmp( rec.name, name, kForestDomainNameCapacity ) == 0;
}

inline bool forest_domain_name_fits( const char* name ) noexcept
{
    if ( name == nullptr || name[0] == '\0' )
        return false;
    return std::strlen( name ) < kForestDomainNameCapacity;
}

template <typename AddressTraitsT>
inline bool forest_domain_name_copy( ForestDomainRecord<AddressTraitsT>& rec, const char* name ) noexcept
{
    if ( !forest_domain_name_fits( name ) )
        return false;
    std::memset( rec.name, 0, sizeof( rec.name ) );
    std::memcpy( rec.name, name, std::strlen( name ) );
    return true;
}

static_assert( std::is_trivially_copyable_v<ForestDomainRecord<DefaultAddressTraits>>,
               "ForestDomainRecord must be trivially copyable" );
static_assert( std::is_nothrow_default_constructible_v<ForestDomainRegistry<DefaultAddressTraits>>,
               "ForestDomainRegistry must be nothrow-default-constructible" );

} // namespace pmm::detail
