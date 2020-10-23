#ifndef __DAVAENGINE_RENDER_HELPER_H__
#define __DAVAENGINE_RENDER_HELPER_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Render/RenderBase.h"
#include "Render/RHI/rhi_Public.h"
#include "Animation/Interpolation.h"

namespace DAVA
{
/**
    \brief You can use this class to perform various important drawing operations in 3D.
    In most cases we use these functions for debug rendering, but in some cases it can be usefull in production. 
    Keep in mind that output of all line-drawing functions can depend on hardware and look differently on different systems
 */

class NMaterial;
class RenderHelper
{
    enum eDrawFlags
    {
        FLAG_DRAW_SOLID = 1,
        FLAG_DRAW_NO_DEPTH = 2,
    };

public:
    enum eDrawType
    {
        DRAW_WIRE_DEPTH = 0,
        DRAW_SOLID_DEPTH = FLAG_DRAW_SOLID,
        DRAW_WIRE_NO_DEPTH = FLAG_DRAW_NO_DEPTH,
        DRAW_SOLID_NO_DEPTH = FLAG_DRAW_SOLID | FLAG_DRAW_NO_DEPTH,

        DRAW_TYPE_COUNT
    };

    RenderHelper();
    ~RenderHelper();

    void Present(rhi::HPacketList packetList, const Matrix4* view, const Matrix4* projection);
    void Clear();
    bool IsEmpty();

    void InvalidateMaterials();

    void DrawLine(const Vector3& pt1, const Vector3& pt2, const Color& color, eDrawType drawType = DRAW_WIRE_DEPTH);
    void DrawPolygon(const Polygon3& polygon, const Color& color, eDrawType drawType);
    void DrawAABox(const AABBox3& box, const Color& color, eDrawType drawType);
    void DrawAABoxTransformed(const AABBox3& box, const Matrix4& matrix, const Color& color, eDrawType drawType);
    void DrawAABoxCorners(const AABBox3& box, const Color& color, eDrawType drawType);
    void DrawAABoxCornersTransformed(const AABBox3& box, const Matrix4& matrix, const Color& color, eDrawType drawType);
    void DrawIcosahedron(const Vector3& position, float32 radius, const Color& color, eDrawType drawType);
    void DrawArrow(const Vector3& from, const Vector3& to, float32 arrowLength, const Color& color, eDrawType drawType);
    void DrawCircle(const Vector3& center, const Vector3& direction, float32 radius, uint32 segmentCount, const Color& color, eDrawType drawType);
    void DrawBSpline(BezierSpline3* bSpline, int segments, float ts, float te, const Color& color, eDrawType drawType);
    void DrawInterpolationFunc(Interpolation::Func func, const Rect& destRect, const Color& color, eDrawType drawType);

    static void CreateClearPass(rhi::HTexture colorBuffer, rhi::HTexture depthBuffer, int32 passPriority, const Color& clearColor, const rhi::Viewport& viewport);

private:
    enum eDrawCommandID
    {
        COMMAND_DRAW_LINE = 0,
        COMMAND_DRAW_POLYGON,
        COMMAND_DRAW_BOX,
        COMMAND_DRAW_BOX_CORNERS,
        COMMAND_DRAW_CIRCLE,
        COMMAND_DRAW_ICOSA,
        COMMAND_DRAW_ARROW,

        COMMAND_COUNT
    };
    struct DrawCommand
    {
        DrawCommand(eDrawCommandID _id) //-V730
        : id(_id)
        {
            Memset(params, 0, sizeof(params));
        }

        DrawCommand(eDrawCommandID _id, eDrawType _drawType, Vector<float32>&& _extraParams)
            : id(_id)
            , drawType(_drawType)
            , extraParams(std::move(_extraParams))
        {
            Memset(params, 0, sizeof(params));
        }

        eDrawCommandID id;
        eDrawType drawType;
        float32 params[16];
        Vector<float32> extraParams;
    };
    struct ColoredVertex
    {
        Vector3 position;
        uint32 color;
    };

    struct RenderStruct
    {
        rhi::Packet packet;
        ColoredVertex* vBufferPtr = nullptr;
        uint16* iBufferPtr = nullptr;
        uint32 vBufferOffset = 0;
        uint32 vBufferSize = 0;
        uint32 iBufferSize = 0;
        bool valid = true;
    };

    void QueueCommand(const DrawCommand& command);
    void GetRequestedVertexCount(const DrawCommand& command, uint32& vertexCount, uint32& indexCount);

    void QueueDrawBoxCommand(eDrawCommandID commandID, const AABBox3& box, const Matrix4* matrix, const Color& color, eDrawType drawType);

    void FillIndeciesFromArray(uint16* buffer, uint16 baseIndex, uint16* indexArray, uint32 indexCount);
    void FillPolygonIndecies(uint16* buffer, uint16 baseIndex, uint32 indexCount, uint32 vertexCount, bool isWire);

    void FillBoxVBuffer(ColoredVertex* buffer, const Vector3& basePoint, const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis, uint32 nativeColor);
    void FillBoxCornersVBuffer(ColoredVertex* buffer, const Vector3& basePoint, const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis, uint32 nativeColor);
    void FillCircleVBuffer(ColoredVertex* buffer, const Vector3& center, const Vector3& direction, float32 radius, uint32 pointCount, uint32 nativeColor);
    void FillArrowVBuffer(ColoredVertex* buffer, const Vector3& from, const Vector3& to, uint32 nativeColor);

    RenderStruct AllocateRenderStruct(eDrawType);
    void CommitRenderStruct(rhi::HPacketList packetList, const RenderStruct& rs);

    uint32 coloredVertexLayoutUID;

    Vector<DrawCommand> commandQueue;
    uint32 vBuffersElemCount[DRAW_TYPE_COUNT];
    uint32 iBuffersElemCount[DRAW_TYPE_COUNT];
    NMaterial* materials[DRAW_TYPE_COUNT];

    DrawCommand drawLineCommand;
    DrawCommand drawIcosahedronCommand;
    DrawCommand drawArrowCommand;
    DrawCommand drawCircleCommand;
    DrawCommand drawBoxCommand;
};
}

#endif // __DAVAENGINE_OBJC_FRAMEWORK_RENDER_HELPER_H__
