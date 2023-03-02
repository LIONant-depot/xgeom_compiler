
#include "xgeom_compiler.h"
#include <filesystem>

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

    //
    // Example, please turn off when no needed...
    // Use the following command line: -OPTIMIZATION "O1" -DEBUG "D0" -TARGET "WINDOWS" -EDITOR ".\x64\test.lion_editor" -PROJECT ".\x64\test.lion_project" -ASSETS "./../../dependencies/xraw3D/dependencies/assimp/test/models/FBX" -INPUT "FF.FF/FF" -OUTPUT "./x64/test.lion_rcdbase"
    //
    if constexpr (true)
    {
        std::filesystem::create_directories("x64/test.lion_project/Config");
        std::filesystem::create_directories("x64/test.lion_project/Resources/xgeom/FF");
        std::filesystem::create_directories("x64/test.lion_rcdbase/0.0-0/WINDOWS.platform/Data");

        //
        // drescriptor file
        //
        xgeom_compiler::descriptor Option;
        Option.m_Main.m_MeshAsset = xcore::string::Fmt("spider.fbx");
        (void)xgeom_compiler::descriptor::Serialize(Option, "./x64/test.lion_project/Resources/xgeom/FF/ResourceDesc.txt", false);

        //
        // Create config file
        //
        xresource_pipeline::config::info Info
        { .m_RootAssetsPath = "x64/Assets"
        };
        Info.m_ResourceTypes.push_back
        ( xresource_pipeline::config::resource_type
        { .m_FullGuid                   = geom_pipeline_compiler::full_guid_v
        , .m_ResourceTypeName           = "xgeom"
        , .m_bDefaultSettingInEditor    = true
        });

        (void)xresource_pipeline::config::Serialize( Info, "./x64/test.lion_project/Config/ResourcePipeline.config", false );
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

