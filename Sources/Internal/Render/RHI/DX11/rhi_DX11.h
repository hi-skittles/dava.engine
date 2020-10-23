#pragma once

#include "_dx11.h"

namespace rhi
{
extern DX11Resources dx11;

void dx11_Initialize(const InitParam& param);

namespace VertexBufferDX11
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle vb, uint32 stream_i, uint32 offset, uint32 stride, ID3D11DeviceContext* context);
}

namespace IndexBufferDX11
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle vb, uint32 offset, ID3D11DeviceContext* context);
}

namespace QueryBufferDX11
{
void SetupDispatch(Dispatch* dispatch);
void SetQueryIndex(Handle buf, uint32 objectIndex, ID3D11DeviceContext* context);
void QueryComplete(Handle buf, ID3D11DeviceContext* context);
bool QueryIsCompleted(Handle buf);

void ReleaseQueryPool();
}

namespace PerfQueryDX11
{
void IssueTimestampQuery(Handle handle, ID3D11DeviceContext* context);
void BeginMeasurment(ID3D11DeviceContext* context);
void EndMeasurment(ID3D11DeviceContext* context);
void DeferredPerfQueriesIssued(const std::vector<Handle>& queries);
void IssueTimestampQueryDeferred(Handle handle, ID3D11DeviceContext* context);
void SetupDispatch(Dispatch* dispatch);
void ObtainPerfQueryMeasurment(ID3D11DeviceContext* context);
void ReleasePerfQueryPool();
}

namespace PipelineStateDX11
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
uint32 VertexLayoutStride(Handle ps, uint32 stream_i);
uint32 VertexLayoutStreamCount(Handle ps);
void GetConstBufferCount(Handle ps, uint32* vertexBufCount, uint32* fragmentBufCount);
void SetToRHI(Handle ps, uint32 layoutUID, ID3D11DeviceContext* context);
}

namespace ConstBufferDX11
{
Handle Alloc(ProgType ptype, uint32 bufIndex, uint32 regCnt);
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void InitializeRingBuffer(uint32 size);
void InvalidateAll();
void InvalidateAllInstances();
void SetToRHI(Handle cb, ID3D11DeviceContext* context, ID3D11Buffer** buffer);
void SetToRHI(Handle cb, const void* instData);
const void* Instance(Handle cb);
}

namespace TextureDX11
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHIFragment(Handle tex, uint32 unitIndex, ID3D11DeviceContext* context);
void SetToRHIVertex(Handle tex, uint32 unitIndex, ID3D11DeviceContext* context);
void SetRenderTarget(Handle tex, uint32 level, TextureFace face, ID3D11DeviceContext* context, ID3D11RenderTargetView** view);
void SetDepthStencil(Handle tex, ID3D11DepthStencilView** view);
void ResolveMultisampling(Handle from, Handle to, ID3D11DeviceContext* context);
Size2i Size(Handle tex);
}

namespace DepthStencilStateDX11
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle state, ID3D11DeviceContext* context);
}

namespace SamplerStateDX11
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void SetToRHI(Handle state, ID3D11DeviceContext* context);
}

namespace RenderPassDX11
{
void Init(uint32 maxCount);
void SetupDispatch(Dispatch* dispatch);
void RejectCommandBuffersAndRelease(Handle handle);
}

namespace CommandBufferDX11
{
void Init(uint32 maxCount);
void BindHardwareCommandBufferDispatch(Dispatch* dispatch);
void BindSoftwareCommandBufferDispatch(Dispatch* dispatch);

Handle Allocate(const RenderPassConfig& passConfig, bool isFirstInPass, bool isLastInPass);
void ExecuteAndRelease(Handle handle, uint32 frameNumber);
void SignalAndRelease(Handle handle);
}

namespace SyncObjectDX11
{
bool IsAlive(Handle handle);
void SetupDispatch(Dispatch* dispatch);
}

void ValidateDX11Device(const char* call);
void ExecDX11(DX11Command* cmd, uint32 cmdCount, bool forceExecute = false);
bool ExecDX11DeviceCommand(DX11Command cmd, const char* cmdName, const char* fileName, uint32 line);
bool DX11_CheckResult(HRESULT, const char* call, const char* fileName, const uint32 line);
void DX11_ProcessCallResult(HRESULT hr, const char* call, const char* fileName, const uint32 line);
uint32 DX11_GetMaxSupportedMultisampleCount(ID3D11Device* device);

#define DX11DeviceCommand(CMD, ...) ExecDX11DeviceCommand(DX11Command(CMD, __VA_ARGS__), #CMD, __FILE__, __LINE__)
#define DX11Check(HR) DX11_CheckResult(HR, #HR, __FILE__, __LINE__)
}
