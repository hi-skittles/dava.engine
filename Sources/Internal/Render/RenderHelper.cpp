#include "Render/Renderer.h"
#include "Render/RenderHelper.h"
#include "Render/Highlevel/RenderPassNames.h"
#include "Render/DynamicBufferAllocator.h"
#include "Material/NMaterial.h"

#include "Debug/ProfilerGPU.h"

const DAVA::float32 ISO_X = 0.525731f;
const DAVA::float32 ISO_Z = 0.850650f;

std::array<DAVA::Vector3, 12> gIcosaVertexes = {
    DAVA::Vector3(-ISO_X, 0.0, ISO_Z),
    DAVA::Vector3(ISO_X, 0.0, ISO_Z),
    DAVA::Vector3(-ISO_X, 0.0, -ISO_Z),
    DAVA::Vector3(ISO_X, 0.0, -ISO_Z),
    DAVA::Vector3(0.0, ISO_Z, ISO_X),
    DAVA::Vector3(0.0, ISO_Z, -ISO_X),
    DAVA::Vector3(0.0, -ISO_Z, ISO_X),
    DAVA::Vector3(0.0, -ISO_Z, -ISO_X),
    DAVA::Vector3(ISO_Z, ISO_X, 0.0),
    DAVA::Vector3(-ISO_Z, ISO_X, 0.0),
    DAVA::Vector3(ISO_Z, -ISO_X, 0.0),
    DAVA::Vector3(-ISO_Z, -ISO_X, 0.0)
};

std::array<DAVA::uint16, 60> gWireIcosaIndexes = {
    0, 1, 1, 4, 4, 8, 8, 5, 5, 3, 3, 2, 2, 7, 7, 11, 11, 6, 6, 0,
    0, 4, 4, 5, 5, 2, 2, 11, 11, 0, 1, 8, 8, 3, 3, 7, 7, 6, 6, 1,
    9, 0, 9, 4, 9, 5, 9, 2, 9, 11, 10, 1, 10, 8, 10, 3, 10, 7, 10, 6,
};

std::array<DAVA::uint16, 60> gSolidIcosaIndexes = {
    0, 4, 1, 0, 9, 4, 9, 5, 4, 4, 5, 8, 4, 8, 1,
    8, 10, 1, 8, 3, 10, 5, 3, 8, 5, 2, 3, 2, 7, 3,
    7, 10, 3, 7, 6, 10, 7, 11, 6, 11, 0, 6, 0, 1, 6,
    6, 1, 10, 9, 0, 11, 9, 11, 2, 9, 2, 5, 7, 2, 11,
};

std::array<DAVA::uint16, 24> gWireBoxIndexes = {
    0, 2, 2, 6, 6, 3, 3, 0,
    1, 4, 4, 7, 7, 5, 5, 1,
    0, 1, 2, 4, 6, 7, 3, 5,
};

std::array<DAVA::uint16, 36> gSolidBoxIndexes = {
    0, 6, 3, 0, 2, 6, 4, 5, 7, 4, 1, 5,
    2, 7, 6, 2, 4, 7, 1, 3, 5, 1, 0, 3,
    3, 7, 5, 3, 6, 7, 1, 2, 0, 1, 4, 2,
};

std::array<DAVA::uint16, 48> gWireBoxCornersIndexes = {
    0, 8, 0, 9, 0, 10, 1, 11, 1, 12, 1, 13,
    2, 14, 2, 15, 2, 16, 3, 17, 3, 18, 3, 19,
    4, 20, 4, 21, 4, 22, 5, 23, 5, 24, 5, 25,
    6, 26, 6, 27, 6, 28, 7, 29, 7, 30, 7, 31,
};

std::array<DAVA::uint16, 72> gSolidBoxCornersIndexes = {
    0, 8, 10, 0, 10, 9, 0, 9, 8, 1, 12, 13, 1, 11, 12, 1, 13, 11,
    2, 14, 16, 2, 16, 15, 2, 15, 14, 3, 19, 17, 3, 17, 18, 3, 18, 19,
    4, 22, 21, 4, 20, 22, 4, 21, 20, 5, 23, 25, 5, 24, 23, 5, 25, 24,
    6, 27, 26, 6, 26, 28, 6, 28, 27, 7, 29, 30, 7, 30, 31, 7, 31, 29,
};

std::array<DAVA::uint16, 16> gWireArrowIndexes = {
    0, 1, 0, 2, 0, 3, 0, 4,
    1, 2, 2, 3, 3, 4, 4, 1,
};

std::array<DAVA::uint16, 18> gSolidArrowIndexes = {
    0, 1, 2, 0, 2, 3, 0, 3, 4,
    0, 4, 1, 1, 4, 2, 2, 4, 3,
};

namespace DAVA
{
RenderHelper::RenderHelper()
    : drawLineCommand(COMMAND_DRAW_LINE)
    , drawIcosahedronCommand(COMMAND_DRAW_ICOSA)
    , drawArrowCommand(COMMAND_DRAW_ARROW)
    , drawCircleCommand(COMMAND_DRAW_CIRCLE)
    , drawBoxCommand(COMMAND_DRAW_BOX)
{
    rhi::VertexLayout layout;
    layout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    layout.AddElement(rhi::VS_COLOR, 0, rhi::VDT_UINT8N, 4);
    coloredVertexLayoutUID = rhi::VertexLayout::UniqueId(layout);

    for (NMaterial*& material : materials)
        material = new NMaterial();

    materials[DRAW_WIRE_DEPTH]->SetFXName(NMaterialName::VERTEXCOLOR_OPAQUE);
    materials[DRAW_SOLID_DEPTH]->SetFXName(NMaterialName::VERTEXCOLOR_ALPHABLEND);
    materials[DRAW_WIRE_NO_DEPTH]->SetFXName(NMaterialName::VERTEXCOLOR_OPAQUE_NODEPTHTEST);
    materials[DRAW_SOLID_NO_DEPTH]->SetFXName(NMaterialName::VERTEXCOLOR_ALPHABLEND_NODEPTHTEST);

    for (NMaterial* material : materials)
        material->PreBuildMaterial(PASS_FORWARD);

    Clear();
}

void RenderHelper::InvalidateMaterials()
{
    for (NMaterial*& material : materials)
        material->InvalidateRenderVariants();
}

RenderHelper::~RenderHelper()
{
    for (int32 i = 0; i < DRAW_TYPE_COUNT; ++i)
        SafeRelease(materials[i]);
}

RenderHelper::RenderStruct RenderHelper::AllocateRenderStruct(eDrawType drawType)
{
    RenderHelper::RenderStruct result;

    result.valid = materials[drawType]->PreBuildMaterial(PASS_FORWARD);
    if (!result.valid)
        return result;

    materials[drawType]->BindParams(result.packet);

    result.packet.primitiveType = (drawType & FLAG_DRAW_SOLID) ? rhi::PRIMITIVE_TRIANGLELIST : rhi::PRIMITIVE_LINELIST;

    result.packet.vertexStreamCount = 1;
    result.packet.vertexLayoutUID = coloredVertexLayoutUID;
    if (vBuffersElemCount[drawType])
    {
        DynamicBufferAllocator::AllocResultVB vb = DynamicBufferAllocator::AllocateVertexBuffer(sizeof(ColoredVertex), vBuffersElemCount[drawType]);
        result.vBufferSize = vb.allocatedVertices;

        result.vBufferPtr = reinterpret_cast<ColoredVertex*>(vb.data);
        result.packet.vertexStream[0] = vb.buffer;
        result.packet.vertexCount = result.vBufferSize;
        result.packet.baseVertex = vb.baseVertex;

        vBuffersElemCount[drawType] -= result.vBufferSize;
    }

    if (iBuffersElemCount[drawType])
    {
        DynamicBufferAllocator::AllocResultIB ib = DynamicBufferAllocator::AllocateIndexBuffer(iBuffersElemCount[drawType]);
        result.iBufferSize = ib.allocatedindices;

        result.iBufferPtr = ib.data;
        result.packet.indexBuffer = ib.buffer;
        result.packet.startIndex = ib.baseIndex;

        iBuffersElemCount[drawType] -= result.iBufferSize;
    }

    return result;
}

void RenderHelper::CommitRenderStruct(rhi::HPacketList packetList, const RenderStruct& rs)
{
    if (rs.packet.primitiveCount)
        rhi::AddPacket(packetList, rs.packet);
}

void RenderHelper::Present(rhi::HPacketList packetList, const Matrix4* viewMatrix, const Matrix4* projectionMatrix)
{
    if (commandQueue.empty())
    {
        return;
    }

    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_WORLD, &Matrix4::IDENTITY, reinterpret_cast<pointer_size>(&Matrix4::IDENTITY));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_VIEW, viewMatrix, reinterpret_cast<pointer_size>(viewMatrix));
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_PROJ, projectionMatrix, reinterpret_cast<pointer_size>(projectionMatrix));

    RenderStruct renderStructs[DRAW_TYPE_COUNT];
    for (uint32 i = 0; i < uint32(DRAW_TYPE_COUNT); ++i)
        renderStructs[i] = AllocateRenderStruct(eDrawType(i));

    DrawCommand* commands = commandQueue.data();
    uint32 commandsCount = uint32(commandQueue.size());
    for (uint32 c = 0; c < commandsCount; ++c)
    {
        const DrawCommand& command = commands[c];
        RenderStruct& currentRenderStruct = renderStructs[command.drawType];

        if (!currentRenderStruct.valid)
            continue;

        uint32 vertexCount = 0;
        uint32 indexCount = 0;
        GetRequestedVertexCount(command, vertexCount, indexCount);

        if (currentRenderStruct.vBufferSize < vertexCount || currentRenderStruct.iBufferSize < indexCount)
        {
            vBuffersElemCount[command.drawType] += currentRenderStruct.vBufferSize;
            iBuffersElemCount[command.drawType] += currentRenderStruct.iBufferSize;

            CommitRenderStruct(packetList, currentRenderStruct);
            currentRenderStruct = AllocateRenderStruct(command.drawType);
        }

        if (currentRenderStruct.vBufferOffset + indexCount >= std::numeric_limits<uint16>::max())
        {
            vBuffersElemCount[command.drawType] += currentRenderStruct.vBufferSize;
            iBuffersElemCount[command.drawType] += currentRenderStruct.iBufferSize;

            CommitRenderStruct(packetList, currentRenderStruct);
            currentRenderStruct = AllocateRenderStruct(command.drawType);
        }

        ColoredVertex* commandVBufferPtr = currentRenderStruct.vBufferPtr;
        uint16* commandIBufferPtr = currentRenderStruct.iBufferPtr;
        uint32 commandVBufferOffset = currentRenderStruct.vBufferOffset;
        bool isWireDraw = (command.drawType & FLAG_DRAW_SOLID) == 0;

        uint32 nativePrimitiveColor = rhi::NativeColorRGBA(command.params[0], command.params[1], command.params[2], command.params[3]);

        switch (command.id)
        {
        case COMMAND_DRAW_LINE:
        {
            commandVBufferPtr[0].position = Vector3(command.params[4], command.params[5], command.params[6]);
            commandVBufferPtr[0].color = nativePrimitiveColor;
            commandVBufferPtr[1].position = Vector3(command.params[7], command.params[8], command.params[9]);
            commandVBufferPtr[1].color = nativePrimitiveColor;

            DVASSERT(commandVBufferOffset + 1 <= std::numeric_limits<uint16>::max());
            commandIBufferPtr[0] = commandVBufferOffset;
            commandIBufferPtr[1] = commandVBufferOffset + 1;
        }
        break;

        case COMMAND_DRAW_POLYGON:
        {
            const uint32 pointCount = static_cast<uint32>(command.extraParams.size() / 3);
            const Vector3* const polygonPoints = reinterpret_cast<const Vector3*>(command.extraParams.data());
            for (uint32 i = 0; i < pointCount; ++i)
            {
                commandVBufferPtr[i].position = polygonPoints[i];
                commandVBufferPtr[i].color = nativePrimitiveColor;
            }

            FillPolygonIndecies(commandIBufferPtr, commandVBufferOffset, indexCount, vertexCount, isWireDraw);
        }
        break;

        case COMMAND_DRAW_BOX:
        {
            const Vector3 basePoint(command.params + 4), xAxis(command.params + 7), yAxis(command.params + 10), zAxis(command.params + 13);
            FillBoxVBuffer(commandVBufferPtr, basePoint, xAxis, yAxis, zAxis, nativePrimitiveColor);
            FillIndeciesFromArray(commandIBufferPtr, commandVBufferOffset, isWireDraw ? gWireBoxIndexes.data() : gSolidBoxIndexes.data(), indexCount);
        }
        break;

        case COMMAND_DRAW_BOX_CORNERS:
        {
            const Vector3 basePoint(command.params + 4), xAxis(command.params + 7), yAxis(command.params + 10), zAxis(command.params + 13);
            FillBoxCornersVBuffer(commandVBufferPtr, basePoint, xAxis, yAxis, zAxis, nativePrimitiveColor);
            FillIndeciesFromArray(commandIBufferPtr, commandVBufferOffset, isWireDraw ? gWireBoxCornersIndexes.data() : gSolidBoxCornersIndexes.data(), indexCount);
        }
        break;

        case COMMAND_DRAW_CIRCLE:
        {
            const uint32 pointCount = static_cast<uint32>(command.params[11]);
            const Vector3 center(command.params + 4), direction(command.params + 7);
            const float32 radius = command.params[10];
            FillCircleVBuffer(commandVBufferPtr, center, direction, radius, pointCount, nativePrimitiveColor);

            FillPolygonIndecies(commandIBufferPtr, commandVBufferOffset, indexCount, vertexCount, isWireDraw);
            if (isWireDraw)
            {
                commandIBufferPtr[vertexCount * 2 - 2] = commandVBufferOffset + vertexCount - 1;
                commandIBufferPtr[vertexCount * 2 - 1] = commandVBufferOffset;
            }
        }
        break;

        case COMMAND_DRAW_ICOSA:
        {
            const Vector3 icosaPosition(command.params[4], command.params[5], command.params[6]);
            const float32 icosaSize = command.params[7];
            for (size_t i = 0; i < gIcosaVertexes.size(); ++i)
            {
                commandVBufferPtr[i].position = gIcosaVertexes[i] * icosaSize + icosaPosition;
                commandVBufferPtr[i].color = nativePrimitiveColor;
            }
            FillIndeciesFromArray(commandIBufferPtr, commandVBufferOffset, isWireDraw ? gWireIcosaIndexes.data() : gSolidIcosaIndexes.data(), indexCount);
        }
        break;

        case COMMAND_DRAW_ARROW:
        {
            const Vector3 from(command.params + 4);
            const Vector3 to(command.params + 7);
            FillArrowVBuffer(commandVBufferPtr, from, to, nativePrimitiveColor);
            FillIndeciesFromArray(commandIBufferPtr, commandVBufferOffset, isWireDraw ? gWireArrowIndexes.data() : gSolidArrowIndexes.data(), indexCount);
        }
        break;

        default:
            break;
        }

        DVASSERT(currentRenderStruct.vBufferSize >= vertexCount);
        DVASSERT(currentRenderStruct.iBufferSize >= indexCount);

        currentRenderStruct.vBufferSize -= vertexCount;
        currentRenderStruct.iBufferSize -= indexCount;

        currentRenderStruct.vBufferPtr += vertexCount;
        currentRenderStruct.iBufferPtr += indexCount;
        currentRenderStruct.vBufferOffset += vertexCount;

        currentRenderStruct.packet.primitiveCount += indexCount / ((command.drawType & FLAG_DRAW_SOLID) ? 3 : 2);
    }

    for (uint32 i = 0; i < uint32(DRAW_TYPE_COUNT); ++i)
    {
        CommitRenderStruct(packetList, renderStructs[i]);
        DVASSERT(!renderStructs[i].valid || (vBuffersElemCount[i] == 0 && iBuffersElemCount[i] == 0));
    }
}

void RenderHelper::Clear()
{
    commandQueue.clear();
    Memset(vBuffersElemCount, 0, sizeof(vBuffersElemCount));
    Memset(iBuffersElemCount, 0, sizeof(iBuffersElemCount));
}

bool RenderHelper::IsEmpty()
{
    return commandQueue.empty();
}

void RenderHelper::QueueCommand(const DrawCommand& command)
{
    commandQueue.emplace_back(command);

    uint32 vertexCount = 0, indexCount = 0;
    GetRequestedVertexCount(command, vertexCount, indexCount);

    vBuffersElemCount[command.drawType] += vertexCount;
    iBuffersElemCount[command.drawType] += indexCount;
}

void RenderHelper::GetRequestedVertexCount(const DrawCommand& command, uint32& vertexCount, uint32& indexCount)
{
    bool isSolidDraw = (command.drawType & FLAG_DRAW_SOLID) != 0;

    switch (command.id)
    {
    case COMMAND_DRAW_LINE:
    {
        vertexCount = 2;
        indexCount = 2;
    }
    break;

    case COMMAND_DRAW_POLYGON:
    {
        vertexCount = static_cast<uint32>(command.extraParams.size() / 3);
        indexCount = isSolidDraw ? (vertexCount - 2) * 3 : (vertexCount - 1) * 2;
    }
    break;

    case COMMAND_DRAW_BOX:
    {
        vertexCount = 8;
        size_t count = isSolidDraw ? gSolidBoxIndexes.size() : gWireBoxIndexes.size();
        indexCount = static_cast<uint32>(count);
    }
    break;

    case COMMAND_DRAW_BOX_CORNERS:
    {
        vertexCount = 32;
        size_t count = isSolidDraw ? gSolidBoxCornersIndexes.size() : gWireBoxCornersIndexes.size();
        indexCount = static_cast<uint32>(count);
    }
    break;

    case COMMAND_DRAW_CIRCLE:
    {
        vertexCount = static_cast<uint32>(command.params[11]);
        size_t count = isSolidDraw ? (vertexCount - 2) * 3 : vertexCount * 2;
        indexCount = static_cast<uint32>(count);
    }
    break;

    case COMMAND_DRAW_ICOSA:
    {
        vertexCount = static_cast<uint32>(gIcosaVertexes.size());
        size_t count = isSolidDraw ? gSolidIcosaIndexes.size() : gWireIcosaIndexes.size();
        indexCount = static_cast<uint32>(count);
    }
    break;

    case COMMAND_DRAW_ARROW:
    {
        vertexCount = 5;
        size_t count = isSolidDraw ? gSolidArrowIndexes.size() : gWireArrowIndexes.size();
        indexCount = static_cast<uint32>(count);
    }
    break;

    default:
    {
        DVASSERT(false && "DrawCommand not implemented");
    }
    break;
    }
}

void RenderHelper::DrawLine(const Vector3& pt1, const Vector3& pt2, const Color& color, eDrawType drawType /*  = DRAW_WIRE_DEPTH */)
{
    DVASSERT((drawType & FLAG_DRAW_SOLID) == 0);

    drawLineCommand.drawType = drawType;

    drawLineCommand.params[0] = color.r;
    drawLineCommand.params[1] = color.g;
    drawLineCommand.params[2] = color.b;
    drawLineCommand.params[3] = color.a;

    drawLineCommand.params[4] = pt1.x;
    drawLineCommand.params[5] = pt1.y;
    drawLineCommand.params[6] = pt1.z;

    drawLineCommand.params[7] = pt2.x;
    drawLineCommand.params[8] = pt2.y;
    drawLineCommand.params[9] = pt2.z;

    QueueCommand(drawLineCommand);
}
void RenderHelper::DrawPolygon(const Polygon3& polygon, const Color& color, eDrawType drawType)
{
    DrawCommand drawCommand(COMMAND_DRAW_POLYGON);
    drawCommand.drawType = drawType;

    drawCommand.extraParams.resize(polygon.pointCount * 3);
    Memcpy(drawCommand.extraParams.data(), polygon.points.data(), sizeof(Vector3) * polygon.pointCount);

    drawCommand.params[0] = color.r;
    drawCommand.params[1] = color.g;
    drawCommand.params[2] = color.b;
    drawCommand.params[3] = color.a;

    QueueCommand(drawCommand);
}
void RenderHelper::DrawAABox(const AABBox3& box, const Color& color, eDrawType drawType)
{
    QueueDrawBoxCommand(COMMAND_DRAW_BOX, box, nullptr, color, drawType);
}
void RenderHelper::DrawAABoxTransformed(const AABBox3& box, const Matrix4& matrix, const Color& color, eDrawType drawType)
{
    QueueDrawBoxCommand(COMMAND_DRAW_BOX, box, &matrix, color, drawType);
}
void RenderHelper::DrawAABoxCorners(const AABBox3& box, const Color& color, eDrawType drawType)
{
    QueueDrawBoxCommand(COMMAND_DRAW_BOX_CORNERS, box, nullptr, color, drawType);
}
void RenderHelper::DrawAABoxCornersTransformed(const AABBox3& box, const Matrix4& matrix, const Color& color, eDrawType drawType)
{
    QueueDrawBoxCommand(COMMAND_DRAW_BOX_CORNERS, box, &matrix, color, drawType);
}
void RenderHelper::DrawArrow(const Vector3& from, const Vector3& to, float32 arrowLength, const Color& color, eDrawType drawType)
{
    Vector3 direction = to - from;
    Vector3 lineEnd = to - (direction * arrowLength / direction.Length());

    drawArrowCommand.drawType = drawType;

    drawArrowCommand.params[0] = color.r;
    drawArrowCommand.params[1] = color.g;
    drawArrowCommand.params[2] = color.b;
    drawArrowCommand.params[3] = color.a;

    drawArrowCommand.params[4] = lineEnd.x;
    drawArrowCommand.params[5] = lineEnd.y;
    drawArrowCommand.params[6] = lineEnd.z;

    drawArrowCommand.params[7] = to.x;
    drawArrowCommand.params[8] = to.y;
    drawArrowCommand.params[9] = to.z;

    QueueCommand(drawArrowCommand);
    DrawLine(from, lineEnd, color, eDrawType(drawType & FLAG_DRAW_NO_DEPTH));
}
void RenderHelper::DrawIcosahedron(const Vector3& position, float32 radius, const Color& color, eDrawType drawType)
{
    drawIcosahedronCommand.drawType = drawType;

    drawIcosahedronCommand.params[0] = color.r;
    drawIcosahedronCommand.params[1] = color.g;
    drawIcosahedronCommand.params[2] = color.b;
    drawIcosahedronCommand.params[3] = color.a;

    drawIcosahedronCommand.params[4] = position.x;
    drawIcosahedronCommand.params[5] = position.y;
    drawIcosahedronCommand.params[6] = position.z;

    drawIcosahedronCommand.params[7] = radius;

    QueueCommand(drawIcosahedronCommand);
}
void RenderHelper::DrawCircle(const Vector3& center, const Vector3& direction, float32 radius, uint32 segmentCount, const Color& color, eDrawType drawType)
{
    drawCircleCommand.drawType = drawType;

    drawCircleCommand.params[0] = color.r;
    drawCircleCommand.params[1] = color.g;
    drawCircleCommand.params[2] = color.b;
    drawCircleCommand.params[3] = color.a;

    drawCircleCommand.params[4] = center.x;
    drawCircleCommand.params[5] = center.y;
    drawCircleCommand.params[6] = center.z;

    drawCircleCommand.params[7] = direction.x;
    drawCircleCommand.params[8] = direction.y;
    drawCircleCommand.params[9] = direction.z;

    drawCircleCommand.params[10] = radius;
    drawCircleCommand.params[11] = static_cast<float32>(segmentCount);

    QueueCommand(drawCircleCommand);
}
void RenderHelper::DrawBSpline(BezierSpline3* bSpline, int segments, float ts, float te, const Color& color, eDrawType drawType)
{
    Polygon3 pts;
    pts.points.reserve(segments);
    for (int k = 0; k < segments; ++k)
    {
        pts.AddPoint(bSpline->Evaluate(0, ts + (te - ts) * (static_cast<float32>(k) / (segments - 1))));
    }
    DrawPolygon(pts, color, drawType);
}
void RenderHelper::DrawInterpolationFunc(Interpolation::Func func, const Rect& destRect, const Color& color, eDrawType drawType)
{
    Polygon3 pts;
    int segmentsCount = 20;
    pts.points.reserve(segmentsCount);
    for (int k = 0; k < segmentsCount; ++k)
    {
        Vector3 v;
        float32 fk = static_cast<float32>(k);
        v.x = destRect.x + (fk / (segmentsCount - 1)) * destRect.dx;
        v.y = destRect.y + func((fk / (segmentsCount - 1))) * destRect.dy;
        v.z = 0.0f;
        pts.AddPoint(v);
    }
    DrawPolygon(pts, color, drawType);
}

void RenderHelper::QueueDrawBoxCommand(eDrawCommandID commandID, const AABBox3& box, const Matrix4* matrix, const Color& color, eDrawType drawType)
{
    Vector3 minPt = box.min;
    Vector3 xAxis(box.max.x - box.min.x, 0.f, 0.f);
    Vector3 yAxis(0.f, box.max.y - box.min.y, 0.f);
    Vector3 zAxis(0.f, 0.f, box.max.z - box.min.z);

    if (matrix)
    {
        minPt = minPt * (*matrix);
        xAxis = MultiplyVectorMat3x3(xAxis, *matrix);
        yAxis = MultiplyVectorMat3x3(yAxis, *matrix);
        zAxis = MultiplyVectorMat3x3(zAxis, *matrix);
    }

    drawBoxCommand.id = commandID;
    drawBoxCommand.drawType = drawType;

    drawBoxCommand.params[0] = color.r;
    drawBoxCommand.params[1] = color.g;
    drawBoxCommand.params[2] = color.b;
    drawBoxCommand.params[3] = color.a;

    drawBoxCommand.params[4] = minPt.x;
    drawBoxCommand.params[5] = minPt.y;
    drawBoxCommand.params[6] = minPt.z;

    drawBoxCommand.params[7] = xAxis.x;
    drawBoxCommand.params[8] = xAxis.y;
    drawBoxCommand.params[9] = xAxis.z;

    drawBoxCommand.params[10] = yAxis.x;
    drawBoxCommand.params[11] = yAxis.y;
    drawBoxCommand.params[12] = yAxis.z;

    drawBoxCommand.params[13] = zAxis.x;
    drawBoxCommand.params[14] = zAxis.y;
    drawBoxCommand.params[15] = zAxis.z;

    QueueCommand(drawBoxCommand);
}

void RenderHelper::FillBoxVBuffer(ColoredVertex* buffer, const Vector3& basePoint, const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis, uint32 nativeColor)
{
    buffer[0].position = basePoint;
    buffer[0].color = nativeColor;
    buffer[1].position = basePoint + xAxis;
    buffer[1].color = nativeColor;
    buffer[2].position = basePoint + yAxis;
    buffer[2].color = nativeColor;
    buffer[3].position = basePoint + zAxis;
    buffer[3].color = nativeColor;

    buffer[4].position = basePoint + xAxis + yAxis;
    buffer[4].color = nativeColor;
    buffer[5].position = basePoint + xAxis + zAxis;
    buffer[5].color = nativeColor;
    buffer[6].position = basePoint + yAxis + zAxis;
    buffer[6].color = nativeColor;
    buffer[7].position = basePoint + xAxis + yAxis + zAxis;
    buffer[7].color = nativeColor;
}

void RenderHelper::FillBoxCornersVBuffer(ColoredVertex* buffer, const Vector3& basePoint, const Vector3& xAxis, const Vector3& yAxis, const Vector3& zAxis, uint32 nativeColor)
{
    FillBoxVBuffer(buffer, basePoint, xAxis, yAxis, zAxis, nativeColor);

    const float32 cornerLength = ((buffer[0].position - buffer[7].position).Length()) * 0.1f + 0.1f;

    const Vector3 xCorner = Normalize(xAxis) * cornerLength;
    const Vector3 yCorner = Normalize(yAxis) * cornerLength;
    const Vector3 zCorner = Normalize(zAxis) * cornerLength;

    buffer[8 + 0 * 3 + 0].position = buffer[0].position + xCorner;
    buffer[8 + 0 * 3 + 1].position = buffer[0].position + yCorner;
    buffer[8 + 0 * 3 + 2].position = buffer[0].position + zCorner;

    buffer[8 + 1 * 3 + 0].position = buffer[1].position - xCorner;
    buffer[8 + 1 * 3 + 1].position = buffer[1].position + yCorner;
    buffer[8 + 1 * 3 + 2].position = buffer[1].position + zCorner;

    buffer[8 + 2 * 3 + 0].position = buffer[2].position + xCorner;
    buffer[8 + 2 * 3 + 1].position = buffer[2].position - yCorner;
    buffer[8 + 2 * 3 + 2].position = buffer[2].position + zCorner;

    buffer[8 + 3 * 3 + 0].position = buffer[3].position + xCorner;
    buffer[8 + 3 * 3 + 1].position = buffer[3].position + yCorner;
    buffer[8 + 3 * 3 + 2].position = buffer[3].position - zCorner;

    buffer[8 + 4 * 3 + 0].position = buffer[4].position - xCorner;
    buffer[8 + 4 * 3 + 1].position = buffer[4].position - yCorner;
    buffer[8 + 4 * 3 + 2].position = buffer[4].position + zCorner;

    buffer[8 + 5 * 3 + 0].position = buffer[5].position - xCorner;
    buffer[8 + 5 * 3 + 1].position = buffer[5].position + yCorner;
    buffer[8 + 5 * 3 + 2].position = buffer[5].position - zCorner;

    buffer[8 + 6 * 3 + 0].position = buffer[6].position + xCorner;
    buffer[8 + 6 * 3 + 1].position = buffer[6].position - yCorner;
    buffer[8 + 6 * 3 + 2].position = buffer[6].position - zCorner;

    buffer[8 + 7 * 3 + 0].position = buffer[7].position - xCorner;
    buffer[8 + 7 * 3 + 1].position = buffer[7].position - yCorner;
    buffer[8 + 7 * 3 + 2].position = buffer[7].position - zCorner;

    for (int32 i = 8; i < 32; ++i)
        buffer[i].color = nativeColor;
}
void RenderHelper::FillCircleVBuffer(ColoredVertex* buffer, const Vector3& center, const Vector3& dir, float32 radius, uint32 pointCount, uint32 nativeColor)
{
    const Vector3 direction = Normalize(dir);
    const Vector3 ortho = Abs(direction.x) < Abs(direction.y) ? direction.CrossProduct(Vector3(1.f, 0.f, 0.f)) : direction.CrossProduct(Vector3(0.f, 1.f, 0.f));

    Matrix4 rotationMx;
    float32 angleDelta = PI_2 / pointCount;
    for (uint32 i = 0; i < pointCount; ++i)
    {
        rotationMx.BuildRotation(direction, -angleDelta * i);
        buffer[i].position = center + (ortho * radius) * rotationMx;
        buffer[i].color = nativeColor;
    }
}
void RenderHelper::FillArrowVBuffer(ColoredVertex* buffer, const Vector3& from, const Vector3& to, uint32 nativeColor)
{
    Vector3 direction = to - from;
    float32 arrowlength = direction.Normalize();
    float32 arrowWidth = arrowlength / 4.f;

    const Vector3 ortho1 = Abs(direction.x) < Abs(direction.y) ? direction.CrossProduct(Vector3(1.f, 0.f, 0.f)) : direction.CrossProduct(Vector3(0.f, 1.f, 0.f));
    const Vector3 ortho2 = ortho1.CrossProduct(direction);

    buffer[0].position = to;
    buffer[0].color = nativeColor;
    buffer[1].position = from + ortho1 * arrowWidth;
    buffer[1].color = nativeColor;
    buffer[2].position = from + ortho2 * arrowWidth;
    buffer[2].color = nativeColor;
    buffer[3].position = from - ortho1 * arrowWidth;
    buffer[3].color = nativeColor;
    buffer[4].position = from - ortho2 * arrowWidth;
    buffer[4].color = nativeColor;
}
void RenderHelper::FillIndeciesFromArray(uint16* buffer, uint16 baseIndex, uint16* indexArray, uint32 indexCount)
{
    for (uint32 i = 0; i < indexCount; ++i)
    {
        DVASSERT(baseIndex + indexArray[i] < std::numeric_limits<uint16>::max());
        buffer[i] = baseIndex + indexArray[i];
    }
}
void RenderHelper::FillPolygonIndecies(uint16* buffer, uint16 baseIndex, uint32 indexCount, uint32 vertexCount, bool isWire)
{
    if (isWire)
    {
        const uint32 linesCount = vertexCount - 1;
        for (uint32 i = 0; i < linesCount; ++i)
        {
            DVASSERT(baseIndex + i + 1 < std::numeric_limits<uint16>::max());
            buffer[i * 2 + 0] = baseIndex + i;
            buffer[i * 2 + 1] = baseIndex + i + 1;
        }
    }
    else
    {
        const uint32 triangleCount = vertexCount - 2;
        for (uint32 i = 0; i < triangleCount; ++i)
        {
            DVASSERT(baseIndex + i + 2 < std::numeric_limits<uint16>::max());
            buffer[i * 3 + 0] = baseIndex + i + 2;
            buffer[i * 3 + 1] = baseIndex + i + 1;
            buffer[i * 3 + 2] = baseIndex;
        }
    }
}

void RenderHelper::CreateClearPass(rhi::HTexture colorBuffer, rhi::HTexture depthBuffer, int32 passPriority, const Color& clearColor, const rhi::Viewport& viewport)
{
    rhi::RenderPassConfig clearPassConfig;
    clearPassConfig.priority = passPriority;
    clearPassConfig.colorBuffer[0].texture = colorBuffer;
    clearPassConfig.colorBuffer[0].clearColor[0] = clearColor.r;
    clearPassConfig.colorBuffer[0].clearColor[1] = clearColor.g;
    clearPassConfig.colorBuffer[0].clearColor[2] = clearColor.b;
    clearPassConfig.colorBuffer[0].clearColor[3] = clearColor.a;
    clearPassConfig.colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
    clearPassConfig.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    clearPassConfig.depthStencilBuffer.texture = depthBuffer;
    clearPassConfig.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    clearPassConfig.depthStencilBuffer.storeAction = rhi::STOREACTION_STORE;
    clearPassConfig.viewport = viewport;

    rhi::HPacketList emptyPacketList;
    rhi::HRenderPass clearPass = rhi::AllocateRenderPass(clearPassConfig, 1, &emptyPacketList);

    if (clearPass != rhi::InvalidHandle)
    {
        rhi::BeginRenderPass(clearPass);
        rhi::BeginPacketList(emptyPacketList);
        rhi::EndPacketList(emptyPacketList);
        rhi::EndRenderPass(clearPass);
    }
}
};
