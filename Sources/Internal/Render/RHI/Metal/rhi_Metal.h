#ifndef __RHI_METAL_H__
#define __RHI_METAL_H__

#include "../Common/rhi_Private.h"
#include "../Common/rhi_BackendImpl.h"
#if defined __OBJC__
#include <Metal/Metal.h>
#endif

namespace rhi
{
struct InitParam;

void metal_Initialize(const InitParam& param);


#if defined __OBJC__

namespace VertexBufferMetal
{
id<MTLBuffer> GetBuffer(Handle ib, unsigned* base);
}

namespace IndexBufferMetal
{
id<MTLBuffer> GetBuffer(Handle ib, unsigned* base);
MTLIndexType GetType(Handle ib);
}

namespace QueryBufferMetal
{
id<MTLBuffer> GetBuffer(Handle qb);
}

namespace TextureMetal
{
void SetToRHIFragment(Handle tex, unsigned unitIndex, id<MTLRenderCommandEncoder> ce);
void SetToRHIVertex(Handle tex, unsigned unitIndex, id<MTLRenderCommandEncoder> ce);
void SetAsRenderTarget(Handle tex, MTLRenderPassDescriptor* desc, unsigned target_i = 0);
void SetAsResolveRenderTarget(Handle tex, MTLRenderPassDescriptor* desc);
void SetAsDepthStencil(Handle tex, MTLRenderPassDescriptor* desc);
void SetAsResolveDepthStencil(Handle tex, MTLRenderPassDescriptor* desc);
}

namespace PipelineStateMetal
{
uint32 SetToRHI(Handle ps, uint32 layoutUID, const MTLPixelFormat* color_fmt, unsigned color_count, bool ds_used, id<MTLRenderCommandEncoder> ce, uint32 sampleCount);
uint32 VertexStreamCount(Handle ps);
}

namespace DepthStencilStateMetal
{
void SetToRHI(Handle ds, id<MTLRenderCommandEncoder> ce);
}

namespace SamplerStateMetal
{
void SetToRHI(Handle ss, id<MTLRenderCommandEncoder> ce);
}

namespace ConstBufferMetal
{
void InitializeRingBuffer(uint32 size);
void InvalidateAllInstances();

void SetToRHI(Handle buf, unsigned bufIndex, id<MTLRenderCommandEncoder> ce);
unsigned Instance(Handle buf);
void SetToRHI(Handle buf, unsigned bufIndex, unsigned instOffset, id<MTLRenderCommandEncoder> ce);
}


#endif

namespace VertexBufferMetal
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}
namespace IndexBufferMetal
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}
namespace QueryBufferMetal
{
void SetupDispatch(Dispatch* dispatch);
}
namespace PerfQueryMetal
{
void SetupDispatch(Dispatch* dispatch);
}
namespace TextureMetal
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
unsigned NeedRestoreCount();
void MarkAllNeedRestore();
void ReCreateAll();
}
namespace SamplerStateMetal
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}
namespace PipelineStateMetal
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}
namespace DepthStencilStateMetal
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}
namespace ConstBufferMetal
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void ResetRingBuffer();
}
namespace RenderPassMetal
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace CommandBufferMetal
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

const unsigned QueryBUfferElemeentAlign = 8;

//==============================================================================
}
#endif // __RHI_METAL_H__
