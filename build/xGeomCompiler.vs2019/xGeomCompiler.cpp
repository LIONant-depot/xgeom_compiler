
#include "xgeom_compiler.h"

//---------------------------------------------------------------------------------------

struct geom_pipeline_compiler : xresource_pipeline::compiler::base
{
    static constexpr xcore::guid::rcfull<> full_guid_v
    { .m_Type       = xcore::guid::rctype<>         { "resource.pipeline", "plugin" }
    , .m_Instance   = xcore::guid::rcinstance<>     { "geom" }
    };

    virtual xcore::guid::rcfull<> getResourcePipelineFullGuid() const noexcept override
    {
        return full_guid_v;
    }

    virtual xcore::err onCompile( void ) noexcept override
    {
        if( auto Err = xgeom_compiler::descriptor::Serialize( m_CompilerOptions, m_ResourceDescriptorPathFile.data(), true ); Err )
            return Err;

        m_Compiler->LoadRaw( xcore::string::Fmt( "%s/%s", m_AssetsRootPath.data(), m_CompilerOptions.m_Main.m_MeshAsset.data() ).data() );
        m_Compiler->Compile(m_CompilerOptions);
        for( auto& T : m_Target )
        {
            if( T.m_bValid) m_Compiler->Serialize(T.m_DataPath.data());
        }
        return {};
    }

    xgeom_compiler::descriptor                  m_CompilerOptions   {};
    std::unique_ptr<xgeom_compiler::instance>   m_Compiler          = xgeom_compiler::MakeInstance();
};


//---------------------------------------------------------------------------------------

int main( int argc, const char* argv[] )
{
    xcore::Init("xgeom_compiler");

    auto GeomCompilerPipeline = std::make_unique<geom_pipeline_compiler>();

    if constexpr (true)
    {
        xgeom_compiler::descriptor Option;
        Option.m_Main.m_MeshAsset = xcore::string::Fmt("spider.fbx");
        (void)xgeom_compiler::descriptor::Serialize(Option, "ResourceDesc.txt", false);
    }

    if constexpr (true)
    {
        xresource_pipeline::config::info Info
        { .m_RootAssetsPath = "x64/Assets"
        };
        Info.m_ResourceTypes.push_back
        ( xresource_pipeline::config::resource_type
        { .m_FullGuid                   = geom_pipeline_compiler::full_guid_v
        , .m_ResourceTypeName           = "xgeom"
        , .m_bDefaultSettingInEditor    = true
        });

        (void)xresource_pipeline::config::Serialize( Info, "ResourcePipeline.config", false );
    }

    //
    // Parse parameters
    //
    if( auto Err = GeomCompilerPipeline->Parse( argc, argv ); Err )
    {
        printf( "%s\nERROR: Fail to compile\n", Err.getCode().m_pString );
        return -1;
    }

    //
    // Start compilation
    //
    if( auto Err = GeomCompilerPipeline->Compile(); Err )
    {
        printf("%s\nERROR: Fail to compile(2)\n", Err.getCode().m_pString);
        return -1;
    }

    return 0;
}

