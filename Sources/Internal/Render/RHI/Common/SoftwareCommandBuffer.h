#pragma once

#include "../rhi_Type.h"
#include "rhi_RingBuffer.h"

namespace rhi
{
enum SoftwareCommandType
{
    CMD_BEGIN = 1,
    CMD_END = 2,

    CMD_SET_VERTEX_DATA = 11,
    CMD_SET_INDICES = 12,
    CMD_SET_QUERY_BUFFER = 13,
    CMD_SET_QUERY_INDEX = 14,
    CMD_ISSUE_TIMESTAMP_QUERY = 17,

    CMD_SET_PIPELINE_STATE = 21,
    CMD_SET_DEPTHSTENCIL_STATE = 22,
    CMD_SET_SAMPLER_STATE = 23,
    CMD_SET_CULL_MODE = 24,
    CMD_SET_SCISSOR_RECT = 25,
    CMD_SET_VIEWPORT = 26,
    CMD_SET_FILLMODE = 27,

    CMD_SET_VERTEX_PROG_CONST_BUFFER = 31,
    CMD_SET_FRAGMENT_PROG_CONST_BUFFER = 32,
    CMD_SET_VERTEX_TEXTURE = 33,
    CMD_SET_FRAGMENT_TEXTURE = 34,

    CMD_DRAW_PRIMITIVE = 41,
    CMD_DRAW_INDEXED_PRIMITIVE = 42,
    CMD_DRAW_INDEXED_PRIMITIVE_RANGED = 43,
    CMD_DRAW_INSTANCED_PRIMITIVE = 46,
    CMD_DRAW_INSTANCED_INDEXED_PRIMITIVE = 47,
    CMD_DRAW_INSTANCED_INDEXED_PRIMITIVE_RANGED = 48,

    CMD_SET_MARKER = 51,
};

struct SWCommand
{
    uint16 type;
    uint16 size;

    SWCommand(uint16 t, uint16 sz)
        : type(t)
        , size(sz)
    {
    }
};

template <class T, SoftwareCommandType t>
struct SWCommandImpl : public SWCommand
{
    SWCommandImpl()
        : SWCommand(t, sizeof(T))
    {
    }
};

struct SWCommand_Begin : public SWCommandImpl<SWCommand_Begin, CMD_BEGIN>
{
};

struct SWCommand_End : public SWCommandImpl<SWCommand_End, CMD_END>
{
    Handle syncObject;
};

struct SWCommand_SetVertexData : public SWCommandImpl<SWCommand_SetVertexData, CMD_SET_VERTEX_DATA>
{
    Handle vb;
    uint32 streamIndex;
};

struct SWCommand_SetIndices : public SWCommandImpl<SWCommand_SetIndices, CMD_SET_INDICES>
{
    Handle ib;
};

struct SWCommand_SetQueryBuffer : public SWCommandImpl<SWCommand_SetQueryBuffer, CMD_SET_QUERY_BUFFER>
{
    Handle queryBuf;
};

struct SWCommand_SetQueryIndex : public SWCommandImpl<SWCommand_SetQueryIndex, CMD_SET_QUERY_INDEX>
{
    uint32 objectIndex;
};

struct SWCommand_IssueTimestamptQuery : public SWCommandImpl<SWCommand_IssueTimestamptQuery, CMD_ISSUE_TIMESTAMP_QUERY>
{
    Handle perfQuery;
};

struct SWCommand_SetPipelineState : public SWCommandImpl<SWCommand_SetPipelineState, CMD_SET_PIPELINE_STATE>
{
    uint32 vdecl;
    uint32 ps;
};

struct SWCommand_SetDepthStencilState : public SWCommandImpl<SWCommand_SetDepthStencilState, CMD_SET_DEPTHSTENCIL_STATE>
{
    Handle depthStencilState;
};

struct SWCommand_SetSamplerState : public SWCommandImpl<SWCommand_SetSamplerState, CMD_SET_SAMPLER_STATE>
{
    Handle samplerState;
};

struct SWCommand_SetCullMode : public SWCommandImpl<SWCommand_SetCullMode, CMD_SET_CULL_MODE>
{
    uint32 mode;
};

struct SWCommand_SetScissorRect : public SWCommandImpl<SWCommand_SetScissorRect, CMD_SET_SCISSOR_RECT>
{
    uint16 x;
    uint16 y;
    uint16 width;
    uint16 height;
};

struct SWCommand_SetViewport : public SWCommandImpl<SWCommand_SetViewport, CMD_SET_VIEWPORT>
{
    uint16 x;
    uint16 y;
    uint16 width;
    uint16 height;
};

struct SWCommand_SetFillMode : public SWCommandImpl<SWCommand_SetFillMode, CMD_SET_FILLMODE>
{
    uint32 mode;
};

struct SWCommand_SetVertexProgConstBuffer : public SWCommandImpl<SWCommand_SetVertexProgConstBuffer, CMD_SET_VERTEX_PROG_CONST_BUFFER>
{
    const void* inst;
    Handle buffer;
    uint8 bufIndex;
};

struct SWCommand_SetFragmentProgConstBuffer : public SWCommandImpl<SWCommand_SetFragmentProgConstBuffer, CMD_SET_FRAGMENT_PROG_CONST_BUFFER>
{
    const void* inst;
    Handle buffer;
    uint8 bufIndex;
};

struct SWCommand_SetVertexTexture : public SWCommandImpl<SWCommand_SetVertexTexture, CMD_SET_VERTEX_TEXTURE>
{
    Handle tex;
    uint8 unitIndex;
};

struct SWCommand_SetFragmentTexture : public SWCommandImpl<SWCommand_SetFragmentTexture, CMD_SET_FRAGMENT_TEXTURE>
{
    Handle tex;
    uint8 unitIndex;
};

struct SWCommand_DrawPrimitive : public SWCommandImpl<SWCommand_DrawPrimitive, CMD_DRAW_PRIMITIVE>
{
    uint32 vertexCount;
    uint8 mode;
};

struct SWCommand_DrawInstancedPrimitive : public SWCommandImpl<SWCommand_DrawInstancedPrimitive, CMD_DRAW_INSTANCED_PRIMITIVE>
{
    uint32 vertexCount;
    uint32 instanceCount;
    uint32 baseInstance;
    uint8 mode;
};

struct SWCommand_DrawIndexedPrimitive : public SWCommandImpl<SWCommand_DrawIndexedPrimitive, CMD_DRAW_INDEXED_PRIMITIVE>
{
    uint32 indexCount;
    uint32 firstVertex;
    uint32 startIndex;
    uint8 mode;
};

struct SWCommand_DrawIndexedPrimitiveRanged : public SWCommandImpl<SWCommand_DrawIndexedPrimitiveRanged, CMD_DRAW_INDEXED_PRIMITIVE_RANGED>
{
    uint32 indexCount;
    uint32 firstVertex;
    uint32 startIndex;
    uint32 vertexCount;
    uint8 mode;
};

struct SWCommand_DrawInstancedIndexedPrimitive : public SWCommandImpl<SWCommand_DrawInstancedIndexedPrimitive, CMD_DRAW_INSTANCED_INDEXED_PRIMITIVE>
{
    uint32 indexCount;
    uint32 firstVertex;
    uint32 startIndex;
    uint32 instanceCount;
    uint32 baseInstance;
    uint8 mode;
};

struct SWCommand_DrawInstancedIndexedPrimitiveRanged : public SWCommandImpl<SWCommand_DrawInstancedIndexedPrimitiveRanged, CMD_DRAW_INSTANCED_INDEXED_PRIMITIVE_RANGED>
{
    uint32 indexCount;
    uint32 firstVertex;
    uint32 startIndex;
    uint32 vertexCount;
    uint32 instanceCount;
    uint32 baseInstance;
    uint8 mode;
};

struct SWCommand_SetMarker : public SWCommandImpl<SWCommand_SetMarker, CMD_SET_MARKER>
{
    const char* text;
};

struct SoftwareCommandBuffer
{
    enum : uint32
    {
        CapacityGrowStep = 4 * 1024,
        Alignment = 4,
    };

    template <class T>
    T* allocCmd()
    {
        uint32 alignedSize = (sizeof(T) + Alignment - 1) & (~(Alignment - 1));
        if (curUsedSize + alignedSize >= cmdDataSize)
        {
            cmdDataSize += CapacityGrowStep;
            cmdData = reinterpret_cast<uint8*>(::realloc(cmdData, cmdDataSize));
        }

        T* command = new (cmdData + curUsedSize) T();
        command->size = alignedSize;
        curUsedSize += alignedSize;
        return command;
    }

    uint8* cmdData = nullptr;
    uint32 cmdDataSize = 0;
    uint32 curUsedSize = 0;
    RingBuffer* text = nullptr;
};
}
