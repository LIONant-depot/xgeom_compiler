#ifndef XGEOM_COMPILER_RUNTIME_HPP
#define XGEOM_COMPILER_RUNTIME_HPP

#include "xcore.h"

struct xgeom
{
    enum
    {
        VERSION = 1
    };

    struct bone
    {
        xcore::bbox             m_BBox;
    };

    struct lod
    {
        float                   m_ScreenArea;
        std::uint16_t           m_iSubmesh;     // Start the submeshes
        std::uint16_t           m_nSubmesh;
    };

    struct mesh
    {
        std::array<char, 32>    m_Name;
        float                   m_WorldPixelSize;   // Average World Pixel size for this SubMesh
        xcore::bbox             m_BBox;
        std::uint16_t           m_nLODs;
        std::uint16_t           m_iLOD;
    };

    struct submesh
    {
        std::uint32_t           m_BaseSortKey;      // used internally by the rendering system
        std::uint32_t           m_iIndex;           // Where the index starts
        std::uint32_t           m_nIndices;         // Where the index starts
        std::uint16_t           m_iDList;           // Index into list of display lists
        std::uint16_t           m_nDLists;          // Number of display lists
        std::uint16_t           m_iMaterial;        // Index of the Material that this SubMesh uses
    };

    enum class cmd_type : std::uint8_t
    { CMD_LOAD_MATRIX
    , CMD_LOAD_MATRICES
    , CMD_RENDER                            // Arg0 = CacheIndex, Arg1 = SourceBoneIndex, 
                                            //        (ArgX1=cmd+1) = IndexOffset from ibase, 
                                            //        (ArgX2=cmd+2) = nIndices   
    };

    struct cmd
    {
        cmd_type                        m_Type;              // Upper bit of the command means that it this structure is valid
        std::array<std::uint8_t, 3>     m_Pad;

        union
        {
            std::uint32_t   m_ArgX;

            struct // load matrix/matrices
            {
                std::uint16_t   m_iSrcMatrix;
                std::uint16_t   m_iMatrixCacheOffset;   //
                std::uint16_t   m_nMatrices;            //
            };

            struct // render command
            {
                std::uint32_t   m_IndexCount;
            };
        };
    };

    struct stream_info
    {
        union element_def
        {
            static constexpr auto btn_mask_v         = 1<<6;
            static constexpr auto bone_weight_mask_v = 1<<5;
            static constexpr auto bone_index_mask_v  = 1<<4;
            static constexpr auto color_mask_v       = 1<<3;
            static constexpr auto uv_mask_v          = 1<<2;
            static constexpr auto position_mask_v    = 1<<1;
            static constexpr auto index_mask_v       = 1<<0;

            std::uint8_t    m_Value;
            struct
            {
                std::uint8_t m_bIndex       :1      // indices
                ,            m_bPosition    :1      // position
                ,            m_bUVs         :1      // uvs
                ,            m_bColor       :1      // a color
                ,            m_bBoneIndices :1      // Indices
                ,            m_bBoneWeights :1      // weights
                ,            m_bBTNs        :1      // binormal tangents and normals
                ;
            };
        };

        enum class format : std::uint8_t
        {   FLOAT_1D
        ,   FLOAT_2D
        ,   FLOAT_3D
        ,   FLOAT_4D
        ,   UINT8_1D_NORMALIZED
        ,   UINT8_4D_NORMALIZED
        ,   UINT8_1D
        ,   UINT16_1D
        ,   UINT32_1D
        ,   SINT8_3D_NORMALIZED
        ,   SINT8_4D_NORMALIZED
        ,   UINT8_4D_UINT
        ,   ENUM_COUNT
        };

        struct vector_info
        {
            std::uint8_t m_Dimensions       :5  // How many dimensions the vector has
            ,            m_ElementSize      :5  // size in bytes of one element in the vector 
            ,            m_bInt             :1  // is int or a float
            ,            m_bSigned          :1  // is it signed or unsigned element
            ,            m_bNormalized      :1  // if the vector represents a normalize number (this is for intergers)
            ;
        };

        constexpr static const auto vector_info_v = []() consteval noexcept
        {
            std::array<vector_info, (int)format::ENUM_COUNT > Info{};
            Info[(int)format::FLOAT_1D]              = vector_info{ .m_Dimensions = 1, .m_ElementSize = sizeof(float),          .m_bInt = false, .m_bSigned = true,  .m_bNormalized = false };
            Info[(int)format::FLOAT_2D]              = vector_info{ .m_Dimensions = 2, .m_ElementSize = sizeof(float),          .m_bInt = false, .m_bSigned = true,  .m_bNormalized = false };
            Info[(int)format::FLOAT_3D]              = vector_info{ .m_Dimensions = 3, .m_ElementSize = sizeof(float),          .m_bInt = false, .m_bSigned = true,  .m_bNormalized = false };
            Info[(int)format::FLOAT_4D]              = vector_info{ .m_Dimensions = 4, .m_ElementSize = sizeof(float),          .m_bInt = false, .m_bSigned = true,  .m_bNormalized = false };
            Info[(int)format::UINT8_1D_NORMALIZED]   = vector_info{ .m_Dimensions = 1, .m_ElementSize = sizeof(std::uint8_t),   .m_bInt = true,  .m_bSigned = false, .m_bNormalized = true  };
            Info[(int)format::UINT8_4D_NORMALIZED]   = vector_info{ .m_Dimensions = 4, .m_ElementSize = sizeof(std::uint8_t),   .m_bInt = true,  .m_bSigned = false, .m_bNormalized = true  };
            Info[(int)format::UINT8_1D]              = vector_info{ .m_Dimensions = 1, .m_ElementSize = sizeof(std::uint8_t),   .m_bInt = true,  .m_bSigned = false, .m_bNormalized = false };
            Info[(int)format::UINT16_1D]             = vector_info{ .m_Dimensions = 1, .m_ElementSize = sizeof(std::uint16_t),  .m_bInt = true,  .m_bSigned = false, .m_bNormalized = false };
            Info[(int)format::UINT32_1D]             = vector_info{ .m_Dimensions = 1, .m_ElementSize = sizeof(std::uint32_t),  .m_bInt = true,  .m_bSigned = false, .m_bNormalized = false };
            Info[(int)format::SINT8_3D_NORMALIZED]   = vector_info{ .m_Dimensions = 3, .m_ElementSize = sizeof(std::uint8_t),   .m_bInt = true,  .m_bSigned = false, .m_bNormalized = true  };
            return Info;
        }();

        constexpr std::uint32_t getSize             ( void ) const noexcept { return getVectorSize() * getVectorCount(); }
        constexpr std::uint32_t getVectorSize       ( void ) const noexcept { return vector_info_v[(int)m_Format].m_ElementSize * getVectorDimension(); }
        constexpr std::uint32_t getVectorElementSize( void ) const noexcept { return vector_info_v[(int)m_Format].m_ElementSize; }
        constexpr std::uint32_t getVectorCount      ( void ) const noexcept { return m_VectorCount; }
        constexpr std::uint32_t getVectorDimension  ( void ) const noexcept { return vector_info_v[(int)m_Format].m_Dimensions; }

        element_def     m_ElementsType;
        format          m_Format;              // Format
        std::uint8_t    m_VectorCount;         // How many vectors in total
        std::uint8_t    m_Offset;              // Offset from the base of the stream to the first element of this type
        std::uint8_t    m_iStream;
    };

    static constexpr auto max_stream_count_v = 6;

    //-------------------------------------------------------------------------
            
                    xgeom                       ( void ) = default;
    inline          xgeom                       ( xcore::serializer::stream& Steaming ) noexcept;
    inline 
    int             getFaceCount                ( void ) const noexcept;
    inline 
    int             getVertexCount              ( void ) const noexcept;
    inline 
    int             findMeshIndex               ( const char* pName ) const noexcept;
    inline 
    int             getSubMeshIndex             ( int iMesh, int iMaterial ) const noexcept;
    inline 
    void            Initialize                  ( void ) noexcept;
    inline
    void            Kill                        ( void ) noexcept;
    inline
    void            Reset                       ( void ) noexcept;
    inline
    bool            isStreamBased               ( void ) const noexcept;
    inline
    bool            hasSeparatedPositions       ( void ) const noexcept;
    inline
    int             getVertexSize               ( int iStream ) const noexcept;
    inline 
    std::uint32_t   getStreamSize               ( int iStream ) const noexcept;
    inline
    std::byte*      getStreamData               ( int iStream ) noexcept;
    inline
    std::byte*      getStreamInfoData           ( int iStreamInfo ) noexcept;
    inline
    int             getStreamInfoStride         ( int iStreamInfo ) noexcept;

    bone*                                           m_pBone;
    mesh*                                           m_pMesh;
    submesh*                                        m_pSubMesh;
    lod*                                            m_pLOD;
    cmd*                                            m_pDList;
    std::byte*                                      m_pData;
    std::uint32_t                                   m_DataSize;
    std::array<std::uint32_t, max_stream_count_v>   m_Stream;
    xcore::bbox                                     m_BBox;
    std::uint32_t                                   m_nIndices;
    std::uint32_t                                   m_nVertices;
    std::uint16_t                                   m_nLODs;
    std::uint16_t                                   m_nBones;
    std::uint16_t                                   m_nMeshes;
    std::uint16_t                                   m_nSubMeshs;
    std::uint16_t                                   m_nMaterials;
    std::uint16_t                                   m_nDisplayLists;
    stream_info::element_def                        m_StreamTypes;
    std::uint8_t                                    m_nStreams;
    std::uint8_t                                    m_nStreamInfos;
    std::uint8_t                                    m_CompactedVertexSize;
    std::array<stream_info, max_stream_count_v>     m_StreamInfo;
};

//-------------------------------------------------------------------------

xgeom::xgeom(xcore::serializer::stream& Steaming) noexcept
{
    //xassert( Steaming.getResourceVersion() == xgeom::VERSION );
}

//-------------------------------------------------------------------------

void xgeom::Initialize(void) noexcept
{
    std::memset( this, 0, sizeof(*this) );
}

//-------------------------------------------------------------------------

void xgeom::Kill(void) noexcept
{
    if (m_pBone)    delete[] m_pBone;
    if (m_pMesh)    delete[] m_pMesh;
    if (m_pSubMesh) delete[] m_pSubMesh;
    if (m_pLOD)     delete[] m_pLOD;
    if (m_pDList)   delete[] m_pDList;
    if( m_pData)    delete[] m_pData;
}

//-------------------------------------------------------------------------

void xgeom::Reset(void) noexcept
{
    Kill();
    Initialize();
}


//-------------------------------------------------------------------------

int xgeom::findMeshIndex( const char* pName ) const noexcept
{
    for ( auto i = 0u; i < m_nMeshes; i++ )
    {
        if ( !std::strcmp(m_pMesh[i].m_Name.data(), pName) )
        {
            return i;
        }
    }

    return -1;
}

//-------------------------------------------------------------------------

bool xgeom::isStreamBased(void) const noexcept 
{ 
    return !m_CompactedVertexSize; 
}

//-------------------------------------------------------------------------

bool xgeom::hasSeparatedPositions(void) const noexcept 
{ 
    return isStreamBased() || (isStreamBased() == false && m_nStreams == 3); 
}

//-------------------------------------------------------------------------

int xgeom::getVertexSize(int iStream)  const noexcept
{
    xassert(iStream < m_nStreams);
    if (m_CompactedVertexSize)
    {
        if (m_nStreams == 3)
        {
            if (iStream == 1) return m_StreamInfo[iStream].getSize();
            return m_CompactedVertexSize;
        }
        else
        {
            xassert(m_nStreams == 2);
            return m_CompactedVertexSize;
        }
    }
    else
    {
        return m_StreamInfo[iStream].getSize();
    }
}

//-------------------------------------------------------------------------

std::uint32_t xgeom::getStreamSize(int iStream) const noexcept
{
    xassert(iStream < m_nStreams);
    if (iStream == 0 || isStreamBased()) return m_StreamInfo[iStream].getSize() * (iStream == 0 ? m_nIndices : m_nVertices);
    if (m_nStreams == 3) return m_nVertices * ((iStream == 1) ? m_StreamInfo[1].getSize() : static_cast<std::uint32_t>(m_CompactedVertexSize));
    return static_cast<std::uint32_t>(m_CompactedVertexSize) * m_nVertices;
}

//-------------------------------------------------------------------------

std::byte* xgeom::getStreamData(int iStream) noexcept
{
    xassert(iStream < m_nStreams);
    return m_pData + m_Stream[iStream];
}

//-------------------------------------------------------------------------

std::byte* xgeom::getStreamInfoData(int iStreamInfo) noexcept
{
    xassert(iStreamInfo < m_nStreamInfos);
    return getStreamData(m_StreamInfo[iStreamInfo].m_iStream) + m_StreamInfo[iStreamInfo].m_Offset;
}

//-------------------------------------------------------------------------

int xgeom::getStreamInfoStride(int iStreamInfo) noexcept
{
    if (iStreamInfo == 0 || isStreamBased()) return m_StreamInfo[iStreamInfo].getSize();
    if (m_nStreams == 3) return (iStreamInfo == 1) ? m_StreamInfo[1].getSize() : static_cast<std::uint32_t>(m_CompactedVertexSize);
    return static_cast<std::uint32_t>(m_CompactedVertexSize);
}

//-------------------------------------------------------------------------
// serializer
//-------------------------------------------------------------------------
namespace xcore::serializer::io_functions
{
    //-------------------------------------------------------------------------

    template<>
    xcore::err SerializeIO<xgeom::stream_info::element_def>(xcore::serializer::stream& Stream, const xgeom::stream_info::element_def& ElementDef ) noexcept
    {
        return Stream.Serialize(ElementDef.m_Value);
    }

    //-------------------------------------------------------------------------

    template<>
    xcore::err SerializeIO<xgeom::bone>(xcore::serializer::stream& Stream, const xgeom::bone& Bone ) noexcept
    {
        xcore::err Err;
        false
        || (Err = Stream.Serialize(Bone.m_BBox.m_Min.m_X    ))
        || (Err = Stream.Serialize(Bone.m_BBox.m_Min.m_Y    ))
        || (Err = Stream.Serialize(Bone.m_BBox.m_Min.m_Z    ))
        || (Err = Stream.Serialize(Bone.m_BBox.m_Max.m_X    ))
        || (Err = Stream.Serialize(Bone.m_BBox.m_Max.m_Y    ))
        || (Err = Stream.Serialize(Bone.m_BBox.m_Max.m_Z    ))
        ;
        return Err;
    }

    //-------------------------------------------------------------------------

    template<>
    xcore::err SerializeIO<xgeom::lod>(xcore::serializer::stream& Stream, const xgeom::lod& Lod ) noexcept
    {
        xcore::err Err;
        false
        || (Err = Stream.Serialize(Lod.m_ScreenArea     ))
        || (Err = Stream.Serialize(Lod.m_iSubmesh       ))
        || (Err = Stream.Serialize(Lod.m_nSubmesh       ))
        ;
        return Err;
    }

    //-------------------------------------------------------------------------

    template<>
    xcore::err SerializeIO<xgeom::mesh>(xcore::serializer::stream& Stream, const xgeom::mesh& Mesh ) noexcept
    {
        xcore::err Err;
        false
        || (Err = Stream.Serialize(Mesh.m_Name              ))
        || (Err = Stream.Serialize(Mesh.m_WorldPixelSize    ))
        || (Err = Stream.Serialize(Mesh.m_BBox.m_Min.m_X    ))
        || (Err = Stream.Serialize(Mesh.m_BBox.m_Min.m_Y    ))
        || (Err = Stream.Serialize(Mesh.m_BBox.m_Min.m_Z    ))
        || (Err = Stream.Serialize(Mesh.m_BBox.m_Max.m_X    ))
        || (Err = Stream.Serialize(Mesh.m_BBox.m_Max.m_Y    ))
        || (Err = Stream.Serialize(Mesh.m_BBox.m_Max.m_Z    ))
        || (Err = Stream.Serialize(Mesh.m_nLODs             ))
        || (Err = Stream.Serialize(Mesh.m_iLOD              ))
        ;
        return Err;
    }

    //-------------------------------------------------------------------------

    template<>
    xcore::err SerializeIO<xgeom::submesh>(xcore::serializer::stream& Stream, const xgeom::submesh& Submesh ) noexcept
    {
        xcore::err Err;
        false
        || (Err = Stream.Serialize(Submesh.m_BaseSortKey ))
        || (Err = Stream.Serialize(Submesh.m_iIndex      ))
        || (Err = Stream.Serialize(Submesh.m_nIndices    ))
        || (Err = Stream.Serialize(Submesh.m_iDList      ))
        || (Err = Stream.Serialize(Submesh.m_nDLists     ))
        || (Err = Stream.Serialize(Submesh.m_iMaterial   ))
        ;
        return Err;
    }

    //-------------------------------------------------------------------------

    template<>
    xcore::err SerializeIO<xgeom::cmd>(xcore::serializer::stream& Stream, const xgeom::cmd& Cmd ) noexcept
    {
        xcore::err Err;
        false
        || (Err = Stream.Serialize( Cmd.m_Type))
        || (Err = Stream.Serialize( Cmd.m_ArgX))
        ;
        return Err;
    }

    //-------------------------------------------------------------------------

    template<>
    xcore::err SerializeIO<xgeom::stream_info>(xcore::serializer::stream& Stream, const xgeom::stream_info& StreamInfo ) noexcept
    {
        xcore::err Err;
        false
        || (Err = Stream.Serialize( StreamInfo.m_ElementsType   ))
        || (Err = Stream.Serialize(StreamInfo.m_Format          ))
        || (Err = Stream.Serialize(StreamInfo.m_VectorCount     ))
        || (Err = Stream.Serialize(StreamInfo.m_Offset          ))
        || (Err = Stream.Serialize(StreamInfo.m_iStream         ))
        ;
        return Err;
    }

    //-------------------------------------------------------------------------

    template<>
    xcore::err SerializeIO<xgeom>(xcore::serializer::stream& Stream, const xgeom& Geom ) noexcept
    {
        xcore::err Err;
        false
        || (Err = Stream.Serialize( Geom.m_pBone,    Geom.m_nBones          ))
        || (Err = Stream.Serialize( Geom.m_pMesh,    Geom.m_nMeshes         ))
        || (Err = Stream.Serialize( Geom.m_pSubMesh, Geom.m_nSubMeshs       ))
        || (Err = Stream.Serialize( Geom.m_pLOD,     Geom.m_nLODs           ))
        || (Err = Stream.Serialize( Geom.m_pDList,   Geom.m_nDisplayLists   ))
        || (Err = Stream.Serialize( Geom.m_pData,    Geom.m_DataSize, mem_type::Flags(mem_type::flags::UNIQUE)))
        || (Err = Stream.Serialize( Geom.m_DataSize             ))
        || (Err = Stream.Serialize( Geom.m_Stream               ))
        || (Err = Stream.Serialize( Geom.m_BBox.m_Min.m_X       ))
        || (Err = Stream.Serialize( Geom.m_BBox.m_Min.m_Y       ))
        || (Err = Stream.Serialize( Geom.m_BBox.m_Max.m_X       ))
        || (Err = Stream.Serialize( Geom.m_BBox.m_Max.m_Y       ))
        || (Err = Stream.Serialize( Geom.m_nIndices             ))
        || (Err = Stream.Serialize( Geom.m_nVertices            ))
        || (Err = Stream.Serialize( Geom.m_nBones               ))
        || (Err = Stream.Serialize( Geom.m_nMeshes              ))
        || (Err = Stream.Serialize( Geom.m_nSubMeshs            ))
        || (Err = Stream.Serialize( Geom.m_nMaterials           ))
        || (Err = Stream.Serialize( Geom.m_nDisplayLists        ))
        || (Err = Stream.Serialize( Geom.m_StreamTypes          ))
        || (Err = Stream.Serialize( Geom.m_nStreams             ))
        || (Err = Stream.Serialize( Geom.m_nStreamInfos         ))
        || (Err = Stream.Serialize( Geom.m_CompactedVertexSize  ))
        || (Err = Stream.Serialize( Geom.m_StreamInfo           ))
        ;
        return Err;
    }

    //-------------------------------------------------------------------------

}


#endif

