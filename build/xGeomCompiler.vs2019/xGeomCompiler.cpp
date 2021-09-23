
#include "xgeom_compiler.h"
#include "../../xresource_pipeline/src/xresource_pipeline.h"
#include "../../dependencies/xresource_pipeline/src/xresource_pipeline.h"

//---------------------------------------------------------------------------------------

struct geom_pipeline_compiler : xresource_pipeline::compiler::base
{
    virtual xcore::guid::rcfull<> getResourcePipelineFullGuid() const noexcept override
    {
        static constexpr xcore::guid::rcfull<> full_guid_v{ 0xffull, 0xffull };
        return full_guid_v;
    }

    virtual xcore::err onCompile( void ) noexcept override
    {


        return {};
    }

    xgeom_compiler::compiler_options    m_CompilerOptions;
    xgeom_compiler::compiler            m_Compiler;
};


//---------------------------------------------------------------------------------------

int main( int argc, const char* argv[] )
{
    xcore::Init("xgeom_compiler");

    auto GeomCompilerPipeline = std::make_unique<geom_pipeline_compiler>();

    //
    // Start compilation
    //
    if( auto Err = GeomCompilerPipeline->Parse( argc, argv ); Err )
    {
        printf( "%s\nERROR: Fail to compile\n", Err.getCode().m_pString );
        return -1;
    }

    return 0;
}

