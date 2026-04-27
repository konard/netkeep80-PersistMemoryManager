static void verify_image_unlocked( VerifyResult& result ) noexcept
{
    result.mode                                       = RecoveryMode::Verify;
    result.ok                                         = true;
    const uint8_t*                               base = _backend.base_ptr();
    const detail::ManagerHeader<address_traits>* hdr  = get_header_c( base );
    if ( hdr->magic != kMagic )
    {
        result.add( ViolationType::HeaderCorruption, DiagnosticAction::Aborted, 0, static_cast<uint64_t>( kMagic ),
                    static_cast<uint64_t>( hdr->magic ) );
        return;
    }
    if ( !detail::is_supported_image_version( hdr->image_version ) )
    {
        result.add( ViolationType::HeaderCorruption, DiagnosticAction::Aborted, 0, detail::kCurrentImageVersion,
                    static_cast<uint64_t>( hdr->image_version ) );
        return;
    }
    if ( hdr->total_size != _backend.total_size() )
    {
        result.add( ViolationType::HeaderCorruption, DiagnosticAction::Aborted, 0, _backend.total_size(),
                    static_cast<uint64_t>( hdr->total_size ) );
        return;
    }
    if ( hdr->granule_size != static_cast<uint16_t>( address_traits::granule_size ) )
    {
        result.add( ViolationType::HeaderCorruption, DiagnosticAction::Aborted, 0, address_traits::granule_size,
                    static_cast<uint64_t>( hdr->granule_size ) );
        return;
    }
    {
        using BlockState = BlockStateBase<address_traits>;
        index_type idx   = hdr->first_block_offset;
        while ( idx != address_traits::no_block )
        {
            if ( !detail::validate_block_index<address_traits>( hdr->total_size, idx ) )
                break;
            detail::validate_block_header_full<address_traits>( base, hdr->total_size, idx, result );
            const void* blk_ptr = base + static_cast<size_t>( idx ) * address_traits::granule_size;
            idx                 = BlockState::get_next_offset( blk_ptr );
        }
    }
    allocator::verify_block_states( base, hdr, result );
    allocator::verify_linked_list( base, hdr, result );
    allocator::verify_counters( base, hdr, result );
    allocator::verify_free_tree( base, hdr, result );
    verify_forest_registry_unlocked( result );
}
static void verify_forest_registry_unlocked( VerifyResult& result ) noexcept
{
    const forest_registry* reg = forest_registry_root_unlocked();
    if ( reg == nullptr )
    {
        result.add( ViolationType::ForestRegistryMissing, DiagnosticAction::NoAction );
        return;
    }
    if ( reg->magic != detail::kForestRegistryMagic || reg->version != detail::kForestRegistryVersion )
    {
        result.add( ViolationType::ForestRegistryMissing, DiagnosticAction::NoAction, 0,
                    static_cast<uint64_t>( detail::kForestRegistryMagic ), static_cast<uint64_t>( reg->magic ) );
        return;
    }
    static constexpr const char* kRequired[] = { detail::kSystemDomainFreeTree, detail::kSystemDomainSymbols,
                                                 detail::kSystemDomainRegistry, detail::kServiceNameDomainRoot };
    for ( const char* name : kRequired )
    {
        const forest_domain* rec = find_domain_by_name_unlocked( name );
        if ( rec == nullptr )
        {
            result.add( ViolationType::ForestDomainMissing, DiagnosticAction::NoAction );
            continue;
        }
        if ( ( rec->flags & detail::kForestDomainFlagSystem ) == 0 )
        {
            result.add( ViolationType::ForestDomainFlagsMissing, DiagnosticAction::NoAction );
        }
    }
}
