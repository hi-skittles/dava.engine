#pragma once

#include "Base/BaseTypes.h"

#include "../Common/rhi_Pool.h"

namespace rhi
{
using DAVA::uint32;

struct Dispatch;
struct InitParam;

void nullRenderer_Initialize(const InitParam& param);

namespace VertexBufferNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace IndexBufferNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace QueryBufferNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace PerfQueryNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace TextureNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace PipelineStateNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace ConstBufferNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace DepthStencilStateNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace SamplerStateNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace RenderPassNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace SyncObjectNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

namespace CommandBufferNull
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
}

} //ns rhi