#include "Render/RenderBase.h"
#include "Concurrency/Thread.h"

namespace DAVA
{
const String CMP_FUNC_NAMES[CMP_TEST_MODE_COUNT] =
{
  "CMP_NEVER",
  "CMP_LESS",
  "CMP_EQUAL",
  "CMP_LEQUAL",
  "CMP_GREATER",
  "CMP_NOTEQUAL",
  "CMP_GEQUAL",
  "CMP_ALWAYS"
};

const String STENCIL_OP_NAMES[STENCILOP_COUNT] =
{
  "STENCILOP_KEEP",
  "STENCILOP_ZERO",
  "STENCILOP_REPLACE",
  "STENCILOP_INVERT",
  "STENCILOP_INCR",
  "STENCILOP_DECR",
  "STENCILOP_INCR_WRAP",
  "STENCILOP_DECR_WRAP"

};

const String FILL_MODE_NAMES[FILLMODE_COUNT] =
{
  "FILLMODE_POINT",
  "FILLMODE_WIREFRAME",
  "FILLMODE_SOLID"
};

rhi::CmpFunc GetCmpFuncByName(const String& cmpFuncStr)
{
    for (uint32 i = 0; i < CMP_TEST_MODE_COUNT; i++)
        if (cmpFuncStr == CMP_FUNC_NAMES[i])
            return static_cast<rhi::CmpFunc>(i);

    return static_cast<rhi::CmpFunc>(CMP_TEST_MODE_COUNT);
}

rhi::StencilOperation GetStencilOpByName(const String& stencilOpStr)
{
    for (uint32 i = 0; i < STENCILOP_COUNT; i++)
        if (stencilOpStr == STENCIL_OP_NAMES[i])
            return static_cast<rhi::StencilOperation>(i);

    return static_cast<rhi::StencilOperation>(STENCILOP_COUNT);
}

/*RHI_COMPLETE - make this stuff correspond with PolygonGroup::UpdateDataPointersAndStreams*/
inline uint32 GetPossibleTexcoordSemantic(uint32 index)
{
    switch (index)
    {
    case 0:
        return EVF_TEXCOORD0; // | EVF_CUBETEXCOORD0;
    case 1:
        return EVF_TEXCOORD1; // | EVF_CUBETEXCOORD1;
    case 2:
        return EVF_TEXCOORD2; // | EVF_CUBETEXCOORD2;
    case 3:
        return EVF_TEXCOORD3; // | EVF_CUBETEXCOORD3;
    case 4:
        return EVF_PIVOT4;
    case 5:
        return EVF_FLEXIBILITY;
    case 6:
        return EVF_ANGLE_SIN_COS;
    }

    return 0;
}

uint32 GetVertexLayoutRequiredFormat(const rhi::VertexLayout& layout)
{
    uint32 res = 0;
    for (uint32 i = 0, sz = layout.ElementCount(); i < sz; ++i)
    {
        rhi::VertexSemantics semantic = layout.ElementSemantics(i);
        switch (semantic)
        {
        case rhi::VS_POSITION:
            res |= EVF_VERTEX;
            break;
        case rhi::VS_NORMAL:
            res |= EVF_NORMAL;
            break;
        case rhi::VS_COLOR:
            res |= EVF_COLOR;
            break;
        case rhi::VS_TEXCOORD:
            res |= GetPossibleTexcoordSemantic(layout.ElementSemanticsIndex(i));
            break;
        case rhi::VS_TANGENT:
            res |= EVF_TANGENT;
            break;
        case rhi::VS_BINORMAL:
            res |= EVF_BINORMAL;
            break;
        case rhi::VS_BLENDWEIGHT:
            res |= EVF_JOINTWEIGHT;
            break;
        case rhi::VS_BLENDINDEX:
        {
            if (layout.ElementDataCount(i) == 1)
            {
                res |= EVF_HARD_JOINTINDEX;
            }
            else
            {
                res |= EVF_JOINTINDEX;
            }
        }
        break;
        default:
            break;
        }
    }
    return res;
}
};

ENUM_DECLARE(DAVA::eVertexFormat)
{
    ENUM_ADD_DESCR(DAVA::EVF_VERTEX, "Vertex");
    ENUM_ADD_DESCR(DAVA::EVF_NORMAL, "Normal");
    ENUM_ADD_DESCR(DAVA::EVF_COLOR, "Color");
    ENUM_ADD_DESCR(DAVA::EVF_TEXCOORD0, "TexCoord 0");
    ENUM_ADD_DESCR(DAVA::EVF_TEXCOORD1, "TexCoord 1");
    ENUM_ADD_DESCR(DAVA::EVF_TEXCOORD2, "TexCoord 2");
    ENUM_ADD_DESCR(DAVA::EVF_TEXCOORD3, "TexCoord 3");
    ENUM_ADD_DESCR(DAVA::EVF_TANGENT, "Tangent");
    ENUM_ADD_DESCR(DAVA::EVF_BINORMAL, "Binormal");
    ENUM_ADD_DESCR(DAVA::EVF_HARD_JOINTINDEX, "Joint index hard");
    ENUM_ADD_DESCR(DAVA::EVF_PIVOT4, "Pivot");
    ENUM_ADD_DESCR(DAVA::EVF_PIVOT_DEPRECATED, "PivotDeprecated");
    ENUM_ADD_DESCR(DAVA::EVF_FLEXIBILITY, "Flexibility");
    ENUM_ADD_DESCR(DAVA::EVF_ANGLE_SIN_COS, "Angle sin cos");
    ENUM_ADD_DESCR(DAVA::EVF_JOINTINDEX, "Joint index");
    ENUM_ADD_DESCR(DAVA::EVF_JOINTWEIGHT, "Joint weight");
    ENUM_ADD_DESCR(DAVA::EVF_CUBETEXCOORD0, "Cube TexCoord 0");
    ENUM_ADD_DESCR(DAVA::EVF_CUBETEXCOORD1, "Cube TexCoord 1");
    ENUM_ADD_DESCR(DAVA::EVF_CUBETEXCOORD2, "Cube TexCoord 2");
    ENUM_ADD_DESCR(DAVA::EVF_CUBETEXCOORD3, "Cube TexCoord 3");
}

ENUM_DECLARE(DAVA::eIndexFormat)
{
    ENUM_ADD_DESCR(DAVA::EIF_16, "16 bit");
    ENUM_ADD_DESCR(DAVA::EIF_32, "32 bit");
}
