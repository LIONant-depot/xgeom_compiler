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
            
                    xgeom               ( void ) = default;
    inline 
    int             getFaceCount        ( void ) const noexcept;
    inline 
    int             getVertexCount      ( void ) const noexcept;
    inline 
    int             findMeshIndex       ( const char* pName ) const noexcept;
    inline 
    int             getSubMeshIndex     ( int iMesh, int iMaterial ) const noexcept;
    inline 
    void            Initialize          ( void ) noexcept;
    inline
    void            Kill                ( void ) noexcept;
    inline
    void            Reset               ( void ) noexcept;

    int             getVertexSize       ( int iStream )
    {
        if(m_CompactedVertexSize)
        {
            if( m_nStreams == 3 )
            {
                if( iStream == 1 ) return m_StreamInfo[iStream].getSize();
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


    bone*                       m_pBone;
    mesh*                       m_pMesh;
    submesh*                    m_pSubMesh;
    lod*                        m_pLOD;
    cmd*                        m_pDList;
    std::array<std::byte*, max_stream_count_v> m_Stream;
    xcore::bbox                 m_BBox;
    std::uint32_t               m_nIndices;
    std::uint32_t               m_nVertices;
    std::uint16_t               m_nBones;
    std::uint16_t               m_nMeshes;
    std::uint16_t               m_nSubMeshs;
    std::uint16_t               m_nMaterials;
    std::uint16_t               m_nDisplayLists;
    stream_info::element_def    m_StreamTypes;
    std::uint8_t                m_nStreams;
    std::uint8_t                m_nStreamInfos;
    std::uint8_t                m_CompactedVertexSize;
    std::array<stream_info, max_stream_count_v>  m_StreamInfo;
};

//-------------------------------------------------------------------------
inline
void xgeom::Initialize(void) noexcept
{
    std::memset( this, 0, sizeof(*this) );
}

//-------------------------------------------------------------------------
inline
void xgeom::Kill(void) noexcept
{
    if (m_pBone) delete[] m_pBone;
    if (m_pMesh) delete[] m_pMesh;
    if (m_pSubMesh) delete[] m_pSubMesh;
    if (m_pLOD) delete[] m_pLOD;
    if (m_pDList) delete[] m_pDList;
    for( auto i=0u; i< m_nStreams; ++i )
    {
        if(m_Stream[i]) delete[] m_Stream[i];
    }
}

//-------------------------------------------------------------------------
inline
void xgeom::Reset(void) noexcept
{
    Kill();
    Initialize();
}

//-------------------------------------------------------------------------
/*
int geom::getFaceCount( void ) const noexcept
{
    int Total = 0;
    for ( auto i = 0u; i < m_nMeshes; i++ )
    {
        Total += m_pMesh[i].m_nFaces;
    }

    return Total;
}

//-------------------------------------------------------------------------

int geom::getVertexCount( void ) const noexcept
{
    int Total = 0;
    for ( auto i = 0u; i < m_nMeshes; i++ )
    {
        Total += m_pMesh[i].m_nVertices;
    }

    return Total;
}
*/
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
/*
int geom::getSubMeshIndex( int iMesh, int iMaterial ) const noexcept
{
    xassert( (iMesh>=0) && (iMesh<m_nMeshes) );
    for ( auto i     = m_pMesh[iMesh].m_iSubMesh
          ,    end_i = m_pMesh[iMesh].m_iSubMesh + m_pMesh[iMesh].m_nSubMeshs
        ; i < end_i
        ; i++ 
        )
    {
        if ( m_pSubMesh[i].m_iMaterial == iMaterial )
            return i;
    }

    return -1;
}
*/
#endif

