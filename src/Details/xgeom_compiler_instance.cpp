
#include "xraw3d.h"
#include "../../dependencies/meshoptimizer/src/meshoptimizer.h"
#include "../../src_runtime/xgeom.h"

namespace xgeom_compiler
{
    struct implementation : xgeom_compiler::instance
    {
        struct vertex
        { 
            struct weight
            {
                std::int32_t    m_iBone;
                float           m_Weight;
            };

            xcore::vector3d                 m_Position;
            std::array<xcore::vector2, 4>   m_UVs;
            xcore::icolor                   m_Color;
            xcore::vector3d                 m_Normal;
            xcore::vector3d                 m_Tangent;
            xcore::vector3d                 m_Binormal;
            std::array<weight, 4>           m_Weights;
        };

        struct lod
        {
            float                           m_ScreenArea;
            std::vector<std::uint32_t>      m_Indices;
        };

        struct sub_mesh
        {
            std::vector<vertex>             m_Vertex;
            std::vector<std::uint32_t>      m_Indices;
            std::vector<lod>                m_LODs;
            std::uint32_t                   m_iMaterial;
            int                             m_nWeights      { 0 };
            int                             m_nUVs          { 0 };
            bool                            m_bHasColor     { false };
            bool                            m_bHasNormal    { false };
            bool                            m_bHasBTN       { false };
        };

        struct mesh
        {
            std::string                     m_Name;
            std::vector<sub_mesh>           m_SubMesh;
        };

        implementation()
        {
            m_FinalGeom.Initialize();
        }

        virtual void LoadRaw( const std::string_view Path ) override
        {
            try
            {
                xraw3d::assimp::ImportAll(m_RawAnim, m_RawGeom, Path.data() );
            }
            catch (std::runtime_error Error)
            {
                printf("%s", Error.what());
            }
        }

        void ConvertToCompilerMesh( const xgeom_compiler::descriptor& CompilerOption )
        {
            for( auto& Mesh : m_RawGeom.m_Mesh )
            {
                auto& NewMesh = m_CompilerMesh.emplace_back();
                NewMesh.m_Name = Mesh.m_Name;
            }

            std::vector<std::int32_t> GeomToCompilerVertMesh( m_RawGeom.m_Facet.size() * 3          );
            std::vector<std::int32_t> MaterialToSubmesh     ( m_RawGeom.m_MaterialInstance.size()   );

            int MinVert = 0;
            int MaxVert = int(GeomToCompilerVertMesh.size()-1);
            int CurMaterial = -1;
            int LastMesh    = -1;

            for( auto& Face : m_RawGeom.m_Facet )
            {
                auto& Mesh = m_CompilerMesh[Face.m_iMesh];

                // Make sure all faces are shorted by the mesh
                xassert(Face.m_iMesh >= LastMesh);
                LastMesh = Face.m_iMesh;

                // are we dealing with a new mesh? if so we need to reset the remap of the submesh
                if( Mesh.m_SubMesh.size() == 0 )
                {
                    for (auto& R : MaterialToSubmesh) R = -1;
                }

                // are we dealiong with a new submesh is so we need to reset the verts
                if( MaterialToSubmesh[Face.m_iMaterialInstance] == -1 )
                {
                    MaterialToSubmesh[Face.m_iMaterialInstance] = int(Mesh.m_SubMesh.size());
                    auto& Submesh = Mesh.m_SubMesh.emplace_back();

                    Submesh.m_iMaterial = Face.m_iMaterialInstance;

                    for (int i = MinVert; i <= MaxVert; ++i)
                        GeomToCompilerVertMesh[i] = -1;

                    MaxVert = 0;
                    MinVert = int(GeomToCompilerVertMesh.size()-1);
                    CurMaterial = Face.m_iMaterialInstance;
                }
                else
                {
                    // Make sure that faces are shorted by materials
                    xassert(CurMaterial >= Face.m_iMaterialInstance );
                }

                auto& SubMesh = Mesh.m_SubMesh[MaterialToSubmesh[Face.m_iMaterialInstance]];

                for( int i=0; i<3; ++i )
                {
                    if( GeomToCompilerVertMesh[Face.m_iVertex[i]] == -1 )
                    {
                        GeomToCompilerVertMesh[Face.m_iVertex[i]] = int(SubMesh.m_Vertex.size());
                        auto& CompilerVert = SubMesh.m_Vertex.emplace_back();
                        auto& RawVert      = m_RawGeom.m_Vertex[Face.m_iVertex[i]];

                        CompilerVert.m_Binormal = RawVert.m_BTN[0].m_Binormal;
                        CompilerVert.m_Tangent  = RawVert.m_BTN[0].m_Tangent;
                        CompilerVert.m_Normal   = RawVert.m_BTN[0].m_Normal;
                        CompilerVert.m_Color    = RawVert.m_Color[0];               // This could be n in the future...
                        CompilerVert.m_Position = RawVert.m_Position;
                        
                        if ( RawVert.m_nTangents ) SubMesh.m_bHasBTN    = true;
                        if ( RawVert.m_nNormals  ) SubMesh.m_bHasNormal = true;
                        if ( RawVert.m_nColors   ) SubMesh.m_bHasColor  = true;

                        if( SubMesh.m_Indices.size() && SubMesh.m_nUVs != 0 && RawVert.m_nUVs < SubMesh.m_nUVs )
                        {
                            printf("WARNING: Found a vertex with an inconsistent set of uvs (Expecting %d, found %d) MeshName: %s \n"
                            , SubMesh.m_nUVs
                            , RawVert.m_nUVs
                            , Mesh.m_Name.data()
                            );
                        }
                        else
                        {
                            SubMesh.m_nUVs = RawVert.m_nUVs;

                            for (int j = 0; j < RawVert.m_nUVs; ++j)
                                CompilerVert.m_UVs[j] = RawVert.m_UV[j];
                        }

                        for( int j = 0; j < RawVert.m_nWeights; ++j )
                        {
                            CompilerVert.m_Weights[j].m_iBone  = RawVert.m_Weight[j].m_iBone;
                            CompilerVert.m_Weights[j].m_Weight = RawVert.m_Weight[j].m_Weight;
                        }

                        SubMesh.m_nWeights = std::max(SubMesh.m_nWeights , RawVert.m_nWeights );
                    }

                    xassert( GeomToCompilerVertMesh[Face.m_iVertex[i]] >= 0 );
                    xassert( GeomToCompilerVertMesh[Face.m_iVertex[i]] < m_RawGeom.m_Vertex.size() );
                    SubMesh.m_Indices.push_back(GeomToCompilerVertMesh[Face.m_iVertex[i]]);

                    MinVert = std::min( MinVert, (int)Face.m_iVertex[i] );
                    MaxVert = std::max( MaxVert, (int)Face.m_iVertex[i] );
                }
            }
        }

        void GenenateLODs( const xgeom_compiler::descriptor& CompilerOption )
        {
            if( CompilerOption.m_LOD.m_GenerateLODs == false ) return;

            for (auto& M : m_CompilerMesh)
            {
                for (auto& S : M.m_SubMesh)
                {
                    for ( size_t i = 1; i < CompilerOption.m_LOD.m_MaxLODs; ++i )
                    {
                        const float       threshold               = std::powf(CompilerOption.m_LOD.m_LODReduction, float(i));
                        const std::size_t target_index_count      = std::size_t(S.m_Indices.size() * threshold) / 3 * 3;
                        const float       target_error            = 1e-2f;
                        const auto&       Source                  = (S.m_LODs.size())? S.m_LODs.back().m_Indices : S.m_Indices;

                        if( Source.size() < target_index_count )
                            break;

                        auto& NewLod = S.m_LODs.emplace_back();

                        NewLod.m_Indices.resize(Source.size());
                        NewLod.m_Indices.resize( meshopt_simplify( NewLod.m_Indices.data(), Source.data(), Source.size(), &S.m_Vertex[0].m_Position.m_X, S.m_Vertex.size(), sizeof(vertex), target_index_count, target_error));
                    }
                }
            }
        }

        void optimizeFacesAndVerts( const xgeom_compiler::descriptor& CompilerOption )
        {
            for( auto& M : m_CompilerMesh)
            {
                const size_t kCacheSize = 16;
                for( auto& S : M.m_SubMesh )
                {
                    meshopt_optimizeVertexCache ( S.m_Indices.data(), S.m_Indices.data(), S.m_Indices.size(), S.m_Vertex.size() ); 
                    meshopt_optimizeOverdraw    ( S.m_Indices.data(), S.m_Indices.data(), S.m_Indices.size(), &S.m_Vertex[0].m_Position.m_X, S.m_Vertex.size(), sizeof(vertex), 1.0f );

                    for( auto& L : S.m_LODs )
                    {
                        meshopt_optimizeVertexCache ( L.m_Indices.data(), L.m_Indices.data(), L.m_Indices.size(), S.m_Vertex.size() );
                        meshopt_optimizeOverdraw    ( L.m_Indices.data(), L.m_Indices.data(), L.m_Indices.size(), &S.m_Vertex[0].m_Position.m_X, S.m_Vertex.size(), sizeof(vertex), 1.0f );
                    }
                }
            }
        }

        void GenerateFinalMesh(const xgeom_compiler::descriptor& CompilerOption)
        {
            std::vector<std::uint32_t>  Indices32;
            std::vector<vertex>         FinalVertex;
            std::vector<xgeom::mesh>    FinalMeshes;
            std::vector<xgeom::submesh> FinalSubmeshes;
            std::vector<xgeom::lod>     FinalLod;
            int                         UVDimensionCount     = 0;
            int                         WeightDimensionCount = 0;
            int                         ColorDimensionCount  = 0;

            FinalMeshes.resize(m_CompilerMesh.size() );

            for( int iMesh = 0; iMesh < FinalMeshes.size(); ++iMesh )
            {
                auto& FinalMesh = FinalMeshes[iMesh];
                auto& CompMesh  = m_CompilerMesh[iMesh];

                strcpy_s( FinalMesh.m_Name.data(), FinalMesh.m_Name.size(), CompMesh.m_Name.c_str() );

                FinalMesh.m_iLOD  = std::uint16_t( FinalLod.size() );
                FinalMesh.m_nLODs = 0;

                auto& FinalLOD = FinalLod.emplace_back();

                FinalLOD.m_iSubmesh = std::uint16_t(FinalSubmeshes.size());
                FinalLOD.m_nSubmesh = std::uint16_t(CompMesh.m_SubMesh.size());

                //
                // Gather LOD 0
                //
                std::vector<int> SubmeshStartIndex;
                for( auto& S : CompMesh.m_SubMesh )
                {
                    const auto iBaseVertex = FinalVertex.size();
                    SubmeshStartIndex.push_back((int)iBaseVertex);
                    auto& FinalSubmesh = FinalSubmeshes.emplace_back();

                    FinalSubmesh.m_iMaterial = S.m_iMaterial;
                    FinalSubmesh.m_iIndex    = std::uint32_t(Indices32.size());
                    FinalSubmesh.m_nIndices  = std::uint32_t(S.m_Indices.size());

                    UVDimensionCount        = std::max( S.m_nUVs, UVDimensionCount );
                    WeightDimensionCount    = std::max( S.m_nWeights, WeightDimensionCount );
                    ColorDimensionCount     = std::max( S.m_bHasColor?1:0, ColorDimensionCount );

                    for( const auto& Index : S.m_Indices )
                    {
                        Indices32.push_back( std::uint32_t(Index + iBaseVertex) );
                    }

                    for( const auto& Verts : S.m_Vertex )
                    {
                        FinalMesh.m_BBox.AddVerts( &Verts.m_Position, 1 );
                        m_FinalGeom.m_BBox.AddVerts( &Verts.m_Position, 1 );
                        FinalVertex.push_back(Verts);
                    }
                }

                //
                // Gather LOD 1... to n
                //
                do
                {
                    auto& LOD = FinalLod.emplace_back();
                    LOD.m_iSubmesh = std::uint16_t(FinalSubmeshes.size());
                    LOD.m_nSubmesh = 0;

                    int iLocalSubmesh = -1;
                    for( auto& S : CompMesh.m_SubMesh )
                    {
                        iLocalSubmesh++;
                        if( FinalMesh.m_nLODs >= S.m_LODs.size() ) continue;

                        const auto iBaseVertex = SubmeshStartIndex[iLocalSubmesh];
                        LOD.m_nSubmesh++;

                        auto& FinalSubmesh = FinalSubmeshes.emplace_back();

                        FinalSubmesh.m_iMaterial = S.m_iMaterial;
                        FinalSubmesh.m_iIndex    = std::uint32_t(Indices32.size());
                        FinalSubmesh.m_nIndices  = std::uint32_t(S.m_Indices.size());

                        for (const auto& Index : S.m_Indices)
                        {
                            Indices32.push_back(std::uint32_t(Index + iBaseVertex));
                        }
                    }

                    if( LOD.m_nSubmesh == 0 )
                    {
                        FinalLod.erase(FinalLod.end()-1);
                        break;
                    }
                    else
                    {
                        FinalMesh.m_nLODs++;
                    }

                } while(true);
            }

            //-----------------------------------------------------------------------------------
            // Final Optimization Step. Optimize vertex location and remap the indices.
            //-----------------------------------------------------------------------------------

            // vertex fetch optimization should go last as it depends on the final index order
            // note that the order of LODs above affects vertex fetch results
            FinalVertex.resize( meshopt_optimizeVertexFetch( FinalVertex.data(), Indices32.data(), Indices32.size(), FinalVertex.data(), FinalVertex.size(), sizeof(FinalVertex[0]) ) );

            const int kCacheSize = 16;
            m_VertCacheStats = meshopt_analyzeVertexCache(Indices32.data(), Indices32.size(), FinalVertex.size(), kCacheSize, 0, 0);
            m_VertFetchStats = meshopt_analyzeVertexFetch(Indices32.data(), Indices32.size(), FinalVertex.size(), sizeof(FinalVertex[0]));
            m_OverdrawStats  = meshopt_analyzeOverdraw   (Indices32.data(), Indices32.size(), &FinalVertex[0].m_Position.m_X, FinalVertex.size(), sizeof(FinalVertex[0]) );

            m_VertCacheNVidiaStats  = meshopt_analyzeVertexCache(Indices32.data(), Indices32.size(), FinalVertex.size(), 32, 32, 32);
            m_VertCacheAMDStats     = meshopt_analyzeVertexCache(Indices32.data(), Indices32.size(), FinalVertex.size(), 14, 64, 128);
            m_VertCacheIntelStats   = meshopt_analyzeVertexCache(Indices32.data(), Indices32.size(), FinalVertex.size(), 128, 0, 0);

            // TODO: Translate this information into a scoring system that goes from 100% to 0% (or something that makes more sense to casual users)
            printf("INFO: ACMR %f ATVR %f (NV %f AMD %f Intel %f) Overfetch %f Overdraw %f\n"
            , m_VertCacheStats.acmr         // transformed vertices / triangle count; best case 0.5, worst case 3.0, optimum depends on topology
            , m_VertCacheStats.atvr         // transformed vertices / vertex count; best case 1.0, worst case 6.0, optimum is 1.0 (each vertex is transformed once)
            , m_VertCacheNVidiaStats.atvr   // transformed vertices / vertex count; best case 1.0, worst case 6.0, optimum is 1.0 (each vertex is transformed once)
            , m_VertCacheAMDStats.atvr      // transformed vertices / vertex count; best case 1.0, worst case 6.0, optimum is 1.0 (each vertex is transformed once)
            , m_VertCacheIntelStats.atvr    // transformed vertices / vertex count; best case 1.0, worst case 6.0, optimum is 1.0 (each vertex is transformed once)
            , m_VertFetchStats.overfetch    // fetched bytes / vertex buffer size; best case 1.0 (each byte is fetched once)
            , m_OverdrawStats.overdraw      // fetched bytes / vertex buffer size; best case 1.0 (each byte is fetched once)
            );

            //-----------------------------------------------------------------------------------
            // Create Stream Infos
            //-----------------------------------------------------------------------------------
            std::size_t MaxVertAligment = 1u;

            m_FinalGeom.m_nStreamInfos        = 0;
            m_FinalGeom.m_nStreams            = 0;
            m_FinalGeom.m_StreamTypes.m_Value = 0;

            //
            // Deal in indices
            //
            {
                auto& Stream = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos];
                Stream.m_ElementsType.m_Value       = 0;

                Stream.m_VectorCount                = 1;
                Stream.m_Format                     = Indices32.size() > 0xffffu ? xgeom::stream_info::format::UINT32_1D : xgeom::stream_info::format::UINT16_1D;
                Stream.m_ElementsType.m_bIndex      = true;
                Stream.m_Offset                     = 0;
                Stream.m_iStream                    = m_FinalGeom.m_nStreams;

                m_FinalGeom.m_StreamTypes.m_bIndex  = true;
                m_FinalGeom.m_nStreams++;
                m_FinalGeom.m_nStreamInfos++;
            }


            //
            // Deal with Position
            //
            {
                auto& Stream = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos];
                Stream.m_ElementsType.m_Value       = 0;

                Stream.m_VectorCount                = 1;
                Stream.m_Format                     = xgeom::stream_info::format::FLOAT_3D;
                Stream.m_ElementsType.m_bPosition   = true;
                Stream.m_Offset                     = 0;
                Stream.m_iStream                    = m_FinalGeom.m_nStreams;

                m_FinalGeom.m_StreamTypes.m_bPosition = true;
                m_FinalGeom.m_nStreamInfos++;

                if( CompilerOption.m_Streams.m_UseElementStreams || CompilerOption.m_Streams.m_SeparatePosition )
                {
                    m_FinalGeom.m_nStreams++;
                }
            }

            //
            // Deal with UVs
            //
            if( UVDimensionCount 
             && (  CompilerOption.m_Cleanup.m_bRemoveUVs[0] == false
                || CompilerOption.m_Cleanup.m_bRemoveUVs[1] == false
                || CompilerOption.m_Cleanup.m_bRemoveUVs[2] == false
                || CompilerOption.m_Cleanup.m_bRemoveUVs[3] == false ) )
            {
                auto& Stream = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos];

                // Make sure the base offset is set
                Stream.m_Offset = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].m_iStream != m_FinalGeom.m_nStreams
                    ? 0
                    : m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].m_Offset 
                        + m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].getSize();

                Stream.m_ElementsType.m_Value       = 0;

                Stream.m_VectorCount     = [&] { int k = 0; for (int i = 0; i < UVDimensionCount; i++) if (CompilerOption.m_Cleanup.m_bRemoveUVs[i] == false) k++; return k; }();
                Stream.m_Format          = xgeom::stream_info::format::FLOAT_2D;

                // Do we still have UVs?
                if( Stream.m_VectorCount )
                {
                    Stream.m_ElementsType.m_bUVs        = true;
                    Stream.m_iStream                    = m_FinalGeom.m_nStreams;

                    Stream.m_Offset = xcore::bits::Align(Stream.m_Offset, alignof(float));
                    MaxVertAligment = std::max(MaxVertAligment, alignof(float));

                    m_FinalGeom.m_nStreamInfos++;
                    m_FinalGeom.m_StreamTypes.m_bUVs = true;

                    if (CompilerOption.m_Streams.m_UseElementStreams) m_FinalGeom.m_nStreams++;
                }
            }

            //
            // Deal with Color
            //
            if( CompilerOption.m_Cleanup.m_bRemoveColor  == false && ColorDimensionCount )
            {
                auto& Stream = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos];

                // Make sure the base offset is set
                Stream.m_Offset = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].m_iStream != m_FinalGeom.m_nStreams
                    ? 0
                    : m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].m_Offset 
                        + m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].getSize();

                Stream.m_ElementsType.m_Value       = 0;

                Stream.m_VectorCount                = 1;
                Stream.m_Format                     = xgeom::stream_info::format::UINT8_4D_NORMALIZED;
                Stream.m_ElementsType.m_bColor      = true;
                Stream.m_iStream                    = m_FinalGeom.m_nStreams;

                Stream.m_Offset = xcore::bits::Align(Stream.m_Offset, alignof(xcore::icolor));
                MaxVertAligment = std::max(MaxVertAligment, alignof(xcore::icolor));

                m_FinalGeom.m_nStreamInfos++;
                m_FinalGeom.m_StreamTypes.m_bColor = true;

                if (CompilerOption.m_Streams.m_UseElementStreams) m_FinalGeom.m_nStreams++;
            }

            //
            // Deal with Bone Weights
            // 
            if( WeightDimensionCount && CompilerOption.m_Cleanup.m_bRemoveBones == false )
            {
                auto& Stream = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos];

                // Make sure the base offset is set
                Stream.m_Offset = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].m_iStream != m_FinalGeom.m_nStreams
                    ? 0
                    : m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].m_Offset
                    + m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].getSize();

                if( CompilerOption.m_Streams.m_bCompressWeights )
                {
                    Stream.m_ElementsType.m_Value       = 0;

                    Stream.m_VectorCount                = std::uint8_t(WeightDimensionCount);
                    Stream.m_Format                     = xgeom::stream_info::format::UINT8_1D_NORMALIZED;
                    Stream.m_ElementsType.m_bBoneWeights= true;
                    Stream.m_iStream                    = m_FinalGeom.m_nStreams;

                    Stream.m_Offset = xcore::bits::Align(Stream.m_Offset, alignof(std::uint8_t));
                    MaxVertAligment = std::max(MaxVertAligment, alignof(std::uint8_t));
                }
                else
                {
                    Stream.m_ElementsType.m_Value       = 0;

                    Stream.m_VectorCount                = std::uint8_t(WeightDimensionCount);
                    Stream.m_Format                     = xgeom::stream_info::format::FLOAT_1D;
                    Stream.m_ElementsType.m_bBoneWeights= true; 
                    Stream.m_iStream                    = m_FinalGeom.m_nStreams;

                    Stream.m_Offset = xcore::bits::Align(Stream.m_Offset, alignof(float));
                    MaxVertAligment = std::max(MaxVertAligment, alignof(float));
                }

                m_FinalGeom.m_nStreamInfos++;
                m_FinalGeom.m_StreamTypes.m_bBoneWeights = true;

                if (CompilerOption.m_Streams.m_UseElementStreams) m_FinalGeom.m_nStreams++;
            }

            //
            // Deal with Bone Indices
            // 
            if( WeightDimensionCount && CompilerOption.m_Cleanup.m_bRemoveBones == false )
            {
                auto& Stream = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos];

                // Make sure the base offset is set
                Stream.m_Offset = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].m_iStream != m_FinalGeom.m_nStreams
                    ? 0
                    : m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].m_Offset
                    + m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].getSize();

                if( CompilerOption.m_Streams.m_bCompressWeights || (m_RawGeom.m_Bone.size() < 0xff) )
                {
                    Stream.m_ElementsType.m_Value       = 0;

                    Stream.m_VectorCount                = std::uint8_t(WeightDimensionCount);
                    Stream.m_Format                     = xgeom::stream_info::format::UINT8_1D;
                    Stream.m_ElementsType.m_bBoneIndices= true;
                    Stream.m_iStream                    = m_FinalGeom.m_nStreams;

                    Stream.m_Offset = xcore::bits::Align(Stream.m_Offset, alignof(std::uint8_t));
                    MaxVertAligment = std::max(MaxVertAligment, alignof(std::uint8_t));
                }
                else
                {
                    Stream.m_ElementsType.m_Value       = 0;

                    Stream.m_VectorCount                = std::uint8_t(WeightDimensionCount);
                    Stream.m_Format                     = xgeom::stream_info::format::UINT16_1D;
                    Stream.m_ElementsType.m_bBoneIndices= true;
                    Stream.m_iStream                    = m_FinalGeom.m_nStreams;

                    Stream.m_Offset = xcore::bits::Align(Stream.m_Offset, alignof(std::uint16_t));
                    MaxVertAligment = std::max(MaxVertAligment, alignof(std::uint16_t));
                }

                m_FinalGeom.m_nStreamInfos++;
                m_FinalGeom.m_StreamTypes.m_bBoneIndices = true;

                if (CompilerOption.m_Streams.m_UseElementStreams) m_FinalGeom.m_nStreams++;
            }

            //
            // Deal with normals
            //
            if( CompilerOption.m_Cleanup.m_bRemoveBTN == false )
            {
                auto& Stream = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos];

                // If the position has a different stream then our offset should be zero
                // Other wise we are dealing with a single vertex structure. In that case
                // we should set our offset propertly.
                Stream.m_Offset = m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].m_iStream != m_FinalGeom.m_nStreams 
                    ? 0
                    : m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].m_Offset 
                        + m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].getSize();

                if( CompilerOption.m_Streams.m_bCompressBTN == false )
                {
                    Stream.m_ElementsType.m_Value       = 0;

                    Stream.m_VectorCount                = 3;
                    Stream.m_Format                     = xgeom::stream_info::format::FLOAT_3D;
                    Stream.m_ElementsType.m_bBTNs       = true;
                    Stream.m_iStream                    = m_FinalGeom.m_nStreams;

                    Stream.m_Offset = xcore::bits::Align(Stream.m_Offset, alignof(xcore::vector3d));
                    MaxVertAligment = std::max( MaxVertAligment, alignof(xcore::vector3d) );
                }
                else
                {
                    Stream.m_ElementsType.m_Value       = 0;

                    Stream.m_VectorCount                = 3;
                    Stream.m_Format                     = xgeom::stream_info::format::SINT8_3D_NORMALIZED;
                    Stream.m_ElementsType.m_bBTNs       = true;
                    Stream.m_iStream                    = m_FinalGeom.m_nStreams;

                    Stream.m_Offset = xcore::bits::Align(Stream.m_Offset, alignof(std::int8_t));
                    MaxVertAligment = std::max(MaxVertAligment, alignof(std::int8_t));
                }

                m_FinalGeom.m_nStreamInfos++;
                m_FinalGeom.m_StreamTypes.m_bBTNs = true;

                if (CompilerOption.m_Streams.m_UseElementStreams) m_FinalGeom.m_nStreams++;
            }

            // We add another one if we are not doing streams
            if (CompilerOption.m_Streams.m_UseElementStreams == false )
            {
                m_FinalGeom.m_nStreams++;
            }

            //
            // Fill up the rest of the info
            //
            m_FinalGeom.m_nMaterials          = std::uint16_t(m_RawGeom.m_MaterialInstance.size());
            m_FinalGeom.m_nMeshes             = std::uint16_t(FinalMeshes.size());
            m_FinalGeom.m_nSubMeshs           = std::uint16_t(FinalSubmeshes.size());
            m_FinalGeom.m_nIndices            = std::uint16_t(Indices32.size());
            m_FinalGeom.m_nVertices           = std::uint32_t(FinalVertex.size());
            m_FinalGeom.m_nLODs               = std::uint16_t(FinalLod.size());

            m_FinalGeom.m_CompactedVertexSize = CompilerOption.m_Streams.m_UseElementStreams
                                                ? std::uint8_t(0)
                                                : std::uint8_t( xcore::bits::Align((std::uint32_t)m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].m_Offset
                                                                                                + m_FinalGeom.m_StreamInfo[m_FinalGeom.m_nStreamInfos - 1].getSize()
                                                                                                , (int)MaxVertAligment ) );
            m_FinalGeom.m_nBones        = 0;
            m_FinalGeom.m_nDisplayLists = 0;

            //
            // Compute the size of the buffer
            //
            constexpr auto max_aligment_v = 16;
            using max_align_byte = std::byte alignas(max_aligment_v);
            m_FinalGeom.m_DataSize  = 0;
            m_FinalGeom.m_Stream[0] = 0;
            for (int i = 0; i < m_FinalGeom.m_nStreams; ++i)
            {
                if (i) m_FinalGeom.m_Stream[i] = m_FinalGeom.m_DataSize;
                const auto Size = m_FinalGeom.getStreamSize(i);
                m_FinalGeom.m_DataSize = xcore::bits::Align(m_FinalGeom.m_DataSize + Size, max_aligment_v);
            }

            m_FinalGeom.m_pData = new max_align_byte[m_FinalGeom.m_DataSize];

            //-----------------------------------------------------------------------------------
            // Create the streams and copy data
            //-----------------------------------------------------------------------------------
            for( int i=0; i<m_FinalGeom.m_nStreamInfos; ++i )
            {
                const auto& StreamInfo = m_FinalGeom.m_StreamInfo[i];

                switch( StreamInfo.m_ElementsType.m_Value )
                {
                //
                // Indices
                //
                case xgeom::stream_info::element_def::index_mask_v: 
                {
                    if (StreamInfo.m_Format == xgeom::stream_info::format::UINT32_1D)
                    {
                        auto pIndexData = reinterpret_cast<std::uint32_t*>(m_FinalGeom.getStreamInfoData(i));
                        for (auto i = 0u; i < Indices32.size(); ++i)
                        {
                            pIndexData[i] = Indices32[i];
                        }
                    }
                    else
                    {
                        auto pIndexData = reinterpret_cast<std::uint16_t*>(m_FinalGeom.getStreamInfoData(i));
                        for (auto i = 0u; i < Indices32.size(); ++i)
                        {
                            pIndexData[i] = std::uint16_t(Indices32[i]);
                        }
                    }
                    break;
                }
                //
                // Positions
                //
                case xgeom::stream_info::element_def::position_mask_v:
                {
                    xassert(StreamInfo.getVectorElementSize() == 4);

                    std::byte* pVertex = m_FinalGeom.getStreamInfoData(i);
                    auto       Stride  = m_FinalGeom.getStreamInfoStride(i);
                    for( auto i=0u; i< FinalVertex.size(); ++i )
                    {
                        *((xcore::vector3d*)pVertex) =  FinalVertex[i].m_Position;
                        pVertex += Stride;
                    }

                    break;
                }

                //
                // BTN
                //
                case xgeom::stream_info::element_def::btn_mask_v:
                {
                    std::byte* pVertex = m_FinalGeom.getStreamInfoData(i);
                    auto       Stride = m_FinalGeom.getStreamInfoStride(i);

                    if( CompilerOption.m_Streams.m_bCompressBTN == false )
                    {
                        xassert(StreamInfo.getVectorElementSize() == 4);
                        for( auto i=0u; i< FinalVertex.size(); ++i )
                        {
                            std::memcpy(&pVertex[0 * sizeof(xcore::vector3d)], &FinalVertex[i].m_Binormal, sizeof(xcore::vector3d));
                            std::memcpy(&pVertex[1 * sizeof(xcore::vector3d)], &FinalVertex[i].m_Tangent,  sizeof(xcore::vector3d));
                            std::memcpy(&pVertex[2 * sizeof(xcore::vector3d)], &FinalVertex[i].m_Normal,   sizeof(xcore::vector3d));
                            pVertex += Stride;
                        }
                    }
                    else
                    {
                        xassert(StreamInfo.getVectorElementSize() == 1);
                        Stride -= 3*3;
                        for( auto i=0u; i< FinalVertex.size(); ++i )
                        {
                            *pVertex = std::byte(FinalVertex[i].m_Binormal.m_X * (0xff >> 1)); pVertex++;
                            *pVertex = std::byte(FinalVertex[i].m_Binormal.m_Y * (0xff >> 1)); pVertex++;
                            *pVertex = std::byte(FinalVertex[i].m_Binormal.m_Z * (0xff >> 1)); pVertex++;

                            *pVertex = std::byte(FinalVertex[i].m_Tangent.m_X * (0xff >> 1)); pVertex++;
                            *pVertex = std::byte(FinalVertex[i].m_Tangent.m_Y * (0xff >> 1)); pVertex++;
                            *pVertex = std::byte(FinalVertex[i].m_Tangent.m_Z * (0xff >> 1)); pVertex++;

                            *pVertex = std::byte(FinalVertex[i].m_Normal.m_X * (0xff >> 1)); pVertex++;
                            *pVertex = std::byte(FinalVertex[i].m_Normal.m_Y * (0xff >> 1)); pVertex++;
                            *pVertex = std::byte(FinalVertex[i].m_Normal.m_Z * (0xff >> 1)); pVertex++;

                            pVertex += Stride;
                        }
                    }
            
                    break;
                }

                //
                // UVs
                //
                case xgeom::stream_info::element_def::uv_mask_v:
                {
                    xassert(StreamInfo.getVectorElementSize() == 4);
                    std::byte* pVertex = m_FinalGeom.getStreamInfoData(i);
                    auto       Stride = m_FinalGeom.getStreamInfoStride(i);

                    for( auto i = 0u; i < FinalVertex.size(); ++i )
                    {
                        for( int j = 0, k = 0; j < UVDimensionCount; ++j )
                        {
                            if (CompilerOption.m_Cleanup.m_bRemoveUVs[j]) continue;
                            std::memcpy(&pVertex[k*sizeof(xcore::vector2)], &FinalVertex[i].m_UVs[j], sizeof(xcore::vector2));
                            k++;
                        }

                        pVertex += Stride;
                    }

                    break;
                }

                //
                // Colors
                //
                case xgeom::stream_info::element_def::color_mask_v:
                {
                    xassert(StreamInfo.getVectorElementSize() == 1);
                    std::byte* pVertex = m_FinalGeom.getStreamInfoData(i);
                    auto       Stride = m_FinalGeom.getStreamInfoStride(i);

                    for( auto i = 0u; i < FinalVertex.size(); ++i )
                    {
                        (*(xcore::icolor*)pVertex) = FinalVertex[i].m_Color;
                        pVertex += Stride;
                    }

                    break;
                }

                //
                // Bone Weights
                //
                case xgeom::stream_info::element_def::bone_weight_mask_v:
                {
                    std::byte* pVertex = m_FinalGeom.getStreamInfoData(i);
                    auto       Stride = m_FinalGeom.getStreamInfoStride(i);

                    if( StreamInfo.getVectorElementSize() == 1 )
                    {
                        for (auto i = 0u; i < FinalVertex.size(); ++i)
                        {
                            for (int j = 0; j < StreamInfo.m_VectorCount; ++j)
                            {
                                pVertex[j] = std::byte(FinalVertex[i].m_Weights[j].m_Weight * 0xff);
                            }

                            pVertex += Stride;
                        }
                    }
                    else
                    {
                        xassert(StreamInfo.getVectorElementSize() == 4);
                        for( auto i = 0u; i < FinalVertex.size(); ++i )
                        {
                            for (int j = 0; j < StreamInfo.m_VectorCount; ++j)
                            {
                                ((float*)pVertex)[j] = FinalVertex[i].m_Weights[j].m_Weight;
                            }

                            pVertex += Stride;
                        }
                    }

                    break;
                } 

                //
                // Bone Indices
                //
                case xgeom::stream_info::element_def::bone_index_mask_v:
                {
                    std::byte* pVertex = m_FinalGeom.getStreamInfoData(i);
                    auto       Stride = m_FinalGeom.getStreamInfoStride(i);

                    if( StreamInfo.getVectorElementSize() == 1 )
                    {
                        for (auto i = 0u; i < FinalVertex.size(); ++i)
                        {
                            for (int j = 0; j < StreamInfo.m_VectorCount; ++j)
                            {
                                *pVertex = std::byte(FinalVertex[i].m_Weights[j].m_iBone);
                            }

                            pVertex += Stride;
                        }
                    }
                    else
                    {
                        xassert(StreamInfo.getVectorElementSize() == 2);
                        for( auto i = 0u; i < FinalVertex.size(); ++i )
                        {
                            for (int j = 0; j < StreamInfo.m_VectorCount; ++j)
                            {
                                ((std::uint16_t*)pVertex)[j] = FinalVertex[i].m_Weights[j].m_iBone;
                            }

                            pVertex += Stride;
                        }
                    }

                    break;
                } 

                } // end case
            }// end for loop

            auto Transfer = []< typename T >( std::vector<T>& V )
            {
                auto Own = std::make_unique<T[]>(V.size());
                for( auto i=0u; i<V.size(); ++i ) Own[i] = V[i];
                return Own.release();
            };

            //
            // Set the rest of the pointers
            //
            m_FinalGeom.m_pLOD      = Transfer(FinalLod);
            m_FinalGeom.m_pMesh     = Transfer(FinalMeshes);
            m_FinalGeom.m_pSubMesh  = Transfer(FinalSubmeshes);
            m_FinalGeom.m_pBone     = nullptr;
            m_FinalGeom.m_pDList    = nullptr;
        }

        virtual void Compile( const xgeom_compiler::descriptor& CompilerOption ) override
        {
            if (CompilerOption.m_Cleanup.m_bForceAddColorIfNone) m_RawGeom.ForceAddColorIfNone();
            if (CompilerOption.m_Cleanup.m_bMergeMeshes) m_RawGeom.CollapseMeshes(CompilerOption.m_Cleanup.m_RenameMesh.c_str());
            m_RawGeom.CleanMesh();
            m_RawGeom.SortFacetsByMeshMaterialBone();

            //
            // Convert to mesh
            //
            m_FinalGeom.Reset();
            ConvertToCompilerMesh(CompilerOption);
            GenenateLODs(CompilerOption);
            optimizeFacesAndVerts(CompilerOption);
            GenerateFinalMesh(CompilerOption);
        }

        virtual void Serialize(const std::string_view FilePath) override
        {
            xcore::serializer::stream Stream;
            if( auto Err = Stream.Save( xcore::string::To<wchar_t>(FilePath), m_FinalGeom, {}, false ); Err )
                throw(std::runtime_error( xcore::string::Fmt("Failed to serialize geometry (%s)", Err.getCode().m_pString).data() ));
        }

        meshopt_VertexCacheStatistics   m_VertCacheAMDStats;
        meshopt_VertexCacheStatistics   m_VertCacheNVidiaStats;
        meshopt_VertexCacheStatistics   m_VertCacheIntelStats;
        meshopt_VertexCacheStatistics   m_VertCacheStats;
        meshopt_VertexFetchStatistics   m_VertFetchStats;
        meshopt_OverdrawStatistics      m_OverdrawStats;
        xgeom                           m_FinalGeom;
        std::vector<mesh>               m_CompilerMesh;
        xraw3d::anim                    m_RawAnim;
        xraw3d::geom                    m_RawGeom;
    };

    //------------------------------------------------------------------------------------

    std::unique_ptr<instance> MakeInstance()
    {
        return std::make_unique<implementation>();
    }
}


