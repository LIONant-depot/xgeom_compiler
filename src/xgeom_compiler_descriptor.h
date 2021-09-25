namespace xgeom_compiler
{
    constexpr int version_major_v    = 1;
    constexpr int version_minor_v    = 1;

    struct descriptor : xresource_pipeline::descriptor::base
    {
        using parent = xresource_pipeline::descriptor::base;

        struct main
        {
            string                  m_MeshAsset             {};                             // File name of the mesh to load
            string                  m_UseSkeletonFile       {};                             // Use a different skeleton file from the one found in the mesh data
//            xcore::guid::rcfull<>   m_UseSkeletonResource{};
        };

        struct cleanup
        {
            bool                    m_bMergeMeshes          = true;                         
            string                  m_RenameMesh            = "Master Mesh";
            bool                    m_bForceAddColorIfNone  = true;
            bool                    m_bRemoveColor          = false;
            std::array<bool,4>      m_bRemoveUVs            {};
            bool                    m_bRemoveBTN            = true;
            bool                    m_bRemoveBones          = true;
        };

        struct lod
        {
            bool                    m_GenerateLODs          = false;
            float                   m_LODReduction          = 0.7f;
            int                     m_MaxLODs               = 5;
        };

        struct streams
        {
            bool                    m_UseElementStreams     = false;
            bool                    m_SeparatePosition      = false;
            bool                    m_bCompressPosition     = false;
            bool                    m_bCompressBTN          = true;
            std::array<bool, 4>     m_bCompressUV           {};
            bool                    m_bCompressWeights      = true;
        };

        descriptor() : xresource_pipeline::descriptor::base
        { .m_Version
            { .m_Major = version_major_v
            , .m_Minor = version_minor_v
            }
        }
        {}

        inline
        static xcore::err Serialize(descriptor& Options, std::string_view FilePath, bool isRead) noexcept;

        main        m_Main;
        cleanup     m_Cleanup;
        lod         m_LOD;
        streams     m_Streams;
    };

    //-------------------------------------------------------------------------------------------------------

    xcore::err descriptor::Serialize( descriptor& Options, std::string_view FilePath, bool isRead ) noexcept
    {
        xcore::textfile::stream Stream;
        xcore::err              Error;

        // Open file
        if( auto Err = Stream.Open(isRead, FilePath, xcore::textfile::file_type::TEXT ); Err )
            return Err;

        if( auto Err = Options.parent::Serialize( Stream, isRead ); Err )
            return Err;

        if (Stream.Record(Error, "MainOptions"
            , [&](std::size_t, xcore::err& Err)
            {   
                0
                || (Err = Stream.Field("MeshAsset",             Options.m_Main.m_MeshAsset))
                || (Err = Stream.Field("UseSkeletonFile",       Options.m_Main.m_UseSkeletonFile))
                ;
            })) return Error;

        if (Stream.Record(Error, "CleanupOptions"
            , [&](std::size_t, xcore::err& Err)
            {
                0
                || (Err = Stream.Field("bMergeMeshes",          Options.m_Cleanup.m_bMergeMeshes))
                || (Err = Stream.Field("RenameMesh",            Options.m_Cleanup.m_RenameMesh))
                || (Err = Stream.Field("bForceAddColorIfNone",  Options.m_Cleanup.m_bForceAddColorIfNone))
                || (Err = Stream.Field("bRemoveColor",          Options.m_Cleanup.m_bRemoveColor))
                || (Err = Stream.Field("m_bRemoveBTN",          Options.m_Cleanup.m_bRemoveBTN))
                || (Err = Stream.Field("m_bRemoveBones",        Options.m_Cleanup.m_bRemoveBones))
                ;
            })) return Error;

        if( Stream.Record( Error, "CleanupOptionsUVs"
        , [&]( std::size_t& Count, xcore::err& Err )
        {
            if( isRead ) xassert( Count == Options.m_Cleanup.m_bRemoveUVs.size());
            else         Count = Options.m_Cleanup.m_bRemoveUVs.size();
        }
        , [&]( std::size_t I, xcore::err& Err )
        {
            Err = Stream.Field( "bRemoveUVs", Options.m_Cleanup.m_bRemoveUVs[I] );
        }) ) return Error;

        if (Stream.Record(Error, "LODOptions"
            , [&](std::size_t, xcore::err& Err)
            {
                0
                || (Err = Stream.Field("GenerateLODs",          Options.m_LOD.m_GenerateLODs))
                || (Err = Stream.Field("LODReduction",          Options.m_LOD.m_LODReduction))
                || (Err = Stream.Field("MaxLODs",               Options.m_LOD.m_MaxLODs))
                ;
            })) return Error;

        if (Stream.Record(Error, "StreamOptions"
            , [&](std::size_t, xcore::err& Err)
            {
                0
                || (Err = Stream.Field("UseElementStreams",     Options.m_Streams.m_UseElementStreams))
                || (Err = Stream.Field("SeparatePosition",      Options.m_Streams.m_SeparatePosition))
                || (Err = Stream.Field("m_bCompressPosition",   Options.m_Streams.m_bCompressPosition))
                || (Err = Stream.Field("m_bCompressBTN",        Options.m_Streams.m_bCompressBTN))
                || (Err = Stream.Field("m_bCompressWeights",    Options.m_Streams.m_bCompressWeights))
                ;
            })) return Error;

        if( Stream.Record( Error, "StreamOptionsUVs"
        , [&]( std::size_t& Count, xcore::err& Err )
        {
            if( isRead ) xassert( Count == Options.m_Streams.m_bCompressUV.size());
            else         Count = Options.m_Streams.m_bCompressUV.size();
        }
        , [&]( std::size_t I, xcore::err& Err )
        {
            Err = Stream.Field( "bCompressUV", Options.m_Streams.m_bCompressUV[I] );
        }) ) return Error;

        return {};
    }
}