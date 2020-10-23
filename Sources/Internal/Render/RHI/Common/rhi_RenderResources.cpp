#include "../rhi_Public.h"
#include "rhi_Private.h"
#include "rhi_CommonImpl.h"
#include "rhi_Pool.h"
#include "RenderLoop.h"

namespace rhi
{
struct TextureSetInfo
{
    TextureSetDescriptor desc;
    Handle handle;
};

struct DepthStencilState_t
{
    DepthStencilState::Descriptor desc;
    Handle state;
    int refCount;
};

struct SamplerState_t
{
    SamplerState::Descriptor desc;
    Handle state;
    int refCount;
};

static DAVA::Mutex _TextureSetInfoMutex;
static std::vector<TextureSetInfo> _TextureSetInfo;

static DAVA::Mutex _DepthStencilStateInfoMutex;
static std::vector<DepthStencilState_t> _DepthStencilStateInfo;

static DAVA::Mutex _SamplerStateInfoMutex;
static std::vector<SamplerState_t> _SamplerStateInfo;

HVertexBuffer CreateVertexBuffer(const VertexBuffer::Descriptor& desc)
{
    return HVertexBuffer(VertexBuffer::Create(desc));
}

//------------------------------------------------------------------------------

void DeleteVertexBuffer(HVertexBuffer vb, bool scheduleDeletion)
{
    if (scheduleDeletion)
        RenderLoop::ScheduleResourceDeletion(vb, RESOURCE_VERTEX_BUFFER);
    else
        VertexBuffer::Delete(vb);
}

//------------------------------------------------------------------------------

void* MapVertexBuffer(HVertexBuffer vb, uint32 offset, uint32 size)
{
    return VertexBuffer::Map(vb, offset, size);
}

//------------------------------------------------------------------------------

void UnmapVertexBuffer(HVertexBuffer vb)
{
    VertexBuffer::Unmap(vb);
}

//------------------------------------------------------------------------------

void UpdateVertexBuffer(HVertexBuffer vb, const void* data, uint32 offset, uint32 size)
{
    VertexBuffer::Update(vb, data, offset, size);
}

//------------------------------------------------------------------------------

bool NeedRestoreVertexBuffer(HVertexBuffer vb)
{
    return VertexBuffer::NeedRestore(vb);
}

//------------------------------------------------------------------------------

HIndexBuffer CreateIndexBuffer(const IndexBuffer::Descriptor& desc)
{
    return HIndexBuffer(IndexBuffer::Create(desc));
}

//------------------------------------------------------------------------------

void DeleteIndexBuffer(HIndexBuffer ib, bool scheduleDeletion)
{
    if (scheduleDeletion)
        RenderLoop::ScheduleResourceDeletion(ib, RESOURCE_INDEX_BUFFER);
    else
        IndexBuffer::Delete(ib);
}

//------------------------------------------------------------------------------

void* MapIndexBuffer(HIndexBuffer ib, uint32 offset, uint32 size)
{
    return IndexBuffer::Map(ib, offset, size);
}

//------------------------------------------------------------------------------

void UnmapIndexBuffer(HIndexBuffer ib)
{
    IndexBuffer::Unmap(ib);
}

//------------------------------------------------------------------------------

void UpdateIndexBuffer(HIndexBuffer ib, const void* data, uint32 offset, uint32 size)
{
    IndexBuffer::Update(ib, data, offset, size);
}

//------------------------------------------------------------------------------

bool NeedRestoreIndexBuffer(HIndexBuffer ib)
{
    return IndexBuffer::NeedRestore(ib);
}

//------------------------------------------------------------------------------

HQueryBuffer CreateQueryBuffer(uint32 maxObjectCount)
{
    return HQueryBuffer(QueryBuffer::Create(maxObjectCount));
}

//------------------------------------------------------------------------------

void ResetQueryBuffer(HQueryBuffer buf)
{
    QueryBuffer::Reset(buf);
}

//------------------------------------------------------------------------------

void DeleteQueryBuffer(HQueryBuffer buf, bool scheduleDeletion)
{
    if (scheduleDeletion)
        RenderLoop::ScheduleResourceDeletion(buf, RESOURCE_QUERY_BUFFER);
    else
        QueryBuffer::Delete(buf);
}

//------------------------------------------------------------------------------

bool QueryBufferIsReady(HQueryBuffer buf)
{
    return QueryBuffer::BufferIsReady(buf);
}

//------------------------------------------------------------------------------

bool QueryIsReady(HQueryBuffer buf, uint32 objectIndex)
{
    return QueryBuffer::IsReady(buf, objectIndex);
}

//------------------------------------------------------------------------------

int QueryValue(HQueryBuffer buf, uint32 objectIndex)
{
    return QueryBuffer::Value(buf, objectIndex);
}

//------------------------------------------------------------------------------

HPerfQuery CreatePerfQuery()
{
    return HPerfQuery(PerfQuery::Create());
}

//------------------------------------------------------------------------------

void DeletePerfQuery(HPerfQuery handle, bool scheduleDeletion)
{
    if (scheduleDeletion)
        RenderLoop::ScheduleResourceDeletion(handle, RESOURCE_PERFQUERY);
    else
        PerfQuery::Delete(handle);
}

void ResetPerfQuery(HPerfQuery handle)
{
    PerfQuery::Reset(handle);
}

//------------------------------------------------------------------------------

bool PerfQueryIsReady(HPerfQuery handle)
{
    return PerfQuery::IsReady(handle);
}

//------------------------------------------------------------------------------

uint64 PerfQueryTimeStamp(HPerfQuery handle)
{
    return PerfQuery::Value(handle);
}

//------------------------------------------------------------------------------

HPipelineState AcquireRenderPipelineState(const PipelineState::Descriptor& desc)
{
    return HPipelineState(PipelineState::Create(desc));
}

//------------------------------------------------------------------------------

void ReleaseRenderPipelineState(HPipelineState rps, bool scheduleDeletion)
{
    //    PipelineState::Delete( rps );
}

//------------------------------------------------------------------------------

HConstBuffer CreateVertexConstBuffer(HPipelineState rps, uint32 bufIndex)
{
    return HConstBuffer(PipelineState::CreateVertexConstBuffer(rps, bufIndex));
}

//------------------------------------------------------------------------------

void CreateVertexConstBuffers(HPipelineState rps, uint32 maxCount, HConstBuffer* constBuf)
{
    for (unsigned i = 0; i != maxCount; ++i)
        constBuf[i] = HConstBuffer(PipelineState::CreateVertexConstBuffer(rps, i));
}

//------------------------------------------------------------------------------

HConstBuffer CreateFragmentConstBuffer(HPipelineState rps, uint32 bufIndex)
{
    return HConstBuffer(PipelineState::CreateFragmentConstBuffer(rps, bufIndex));
}

//------------------------------------------------------------------------------

void CreateFragmentConstBuffers(HPipelineState rps, uint32 maxCount, HConstBuffer* constBuf)
{
    for (unsigned i = 0; i != maxCount; ++i)
        constBuf[i] = HConstBuffer(PipelineState::CreateFragmentConstBuffer(rps, i));
}

//------------------------------------------------------------------------------

bool UpdateConstBuffer4fv(HConstBuffer constBuf, uint32 constIndex, const float* data, uint32 constCount)
{
    return ConstBuffer::SetConst(constBuf, constIndex, constCount, data);
}

//------------------------------------------------------------------------------

bool UpdateConstBuffer1fv(HConstBuffer constBuf, uint32 constIndex, uint32 constSubIndex, const float* data, uint32 dataCount)
{
    return ConstBuffer::SetConst(constBuf, constIndex, constSubIndex, data, dataCount);
}

//------------------------------------------------------------------------------

void DeleteConstBuffer(HConstBuffer constBuf, bool scheduleDeletion)
{
    if (scheduleDeletion)
        RenderLoop::ScheduleResourceDeletion(constBuf, RESOURCE_CONST_BUFFER);
    else
        ConstBuffer::Delete(constBuf);
}

HTextureSet AcquireTextureSet(const TextureSetDescriptor& desc)
{
    HTextureSet handle;

    DAVA::LockGuard<DAVA::Mutex> lock(_TextureSetInfoMutex);
    for (std::vector<TextureSetInfo>::const_iterator i = _TextureSetInfo.begin(), i_end = _TextureSetInfo.end(); i != i_end; ++i)
    {
        if (i->desc.fragmentTextureCount == desc.fragmentTextureCount && i->desc.vertexTextureCount == desc.vertexTextureCount && memcmp(i->desc.fragmentTexture, desc.fragmentTexture, desc.fragmentTextureCount * sizeof(Handle)) == 0 && memcmp(i->desc.vertexTexture, desc.vertexTexture, desc.vertexTextureCount * sizeof(Handle)) == 0)
        {
            CommonImpl::TextureSet_t* ts = TextureSet::Get(i->handle);

            ++ts->refCount;

            handle = HTextureSet(i->handle);
            break;
        }
    }

    if (!handle.IsValid())
    {
        handle = HTextureSet(TextureSet::Create());

        CommonImpl::TextureSet_t* ts = TextureSet::Get(handle);
        TextureSetInfo info;

        ts->refCount = 1;
        ts->fragmentTextureCount = desc.fragmentTextureCount;
        ts->vertexTextureCount = desc.vertexTextureCount;
        memcpy(ts->fragmentTexture, desc.fragmentTexture, desc.fragmentTextureCount * sizeof(Handle));
        memcpy(ts->vertexTexture, desc.vertexTexture, desc.vertexTextureCount * sizeof(Handle));

        info.desc = desc;
        info.handle = handle;
        _TextureSetInfo.push_back(info);
    }

    return handle;
}

//------------------------------------------------------------------------------

HTextureSet CopyTextureSet(HTextureSet tsh)
{
    HTextureSet handle;
    CommonImpl::TextureSet_t* ts = TextureSet::Get(tsh);

    if (ts)
    {
        ++ts->refCount;
        handle = tsh;
    }

    return handle;
}

//------------------------------------------------------------------------------

void ReleaseTextureSet(HTextureSet tsh, bool scheduleDeletion)
{
    if (tsh != InvalidHandle)
    {
        CommonImpl::TextureSet_t* ts = TextureSet::Get(tsh);
        if (--ts->refCount == 0)
        {
            if (scheduleDeletion)
                RenderLoop::ScheduleResourceDeletion(tsh, RESOURCE_TEXTURE_SET);
            else
                TextureSet::Delete(tsh);

            DAVA::LockGuard<DAVA::Mutex> lock(_TextureSetInfoMutex);
            for (std::vector<TextureSetInfo>::iterator i = _TextureSetInfo.begin(), i_end = _TextureSetInfo.end(); i != i_end; ++i)
            {
                if (i->handle == tsh)
                {
                    _TextureSetInfo.erase(i);
                    break;
                }
            }
        }
    }
}

void ReplaceTextureInAllTextureSets(HTexture oldHandle, HTexture newHandle)
{
    DAVA::LockGuard<DAVA::Mutex> lock(_TextureSetInfoMutex);
    for (std::vector<TextureSetInfo>::iterator s = _TextureSetInfo.begin(), s_end = _TextureSetInfo.end(); s != s_end; ++s)
    {
        // update texture-set itself

        CommonImpl::TextureSet_t* ts = TextureSet::Get(s->handle);

        if (ts)
        {
            for (unsigned i = 0; i != ts->fragmentTextureCount; ++i)
            {
                if (ts->fragmentTexture[i] == oldHandle)
                    ts->fragmentTexture[i] = newHandle;
            }
            for (unsigned i = 0; i != ts->vertexTextureCount; ++i)
            {
                if (ts->vertexTexture[i] == oldHandle)
                    ts->vertexTexture[i] = newHandle;
            }
        }

        // update desc as well

        for (uint32 t = 0; t != s->desc.fragmentTextureCount; ++t)
        {
            if (s->desc.fragmentTexture[t] == oldHandle)
                s->desc.fragmentTexture[t] = newHandle;
        }
        for (uint32 t = 0; t != s->desc.vertexTextureCount; ++t)
        {
            if (s->desc.vertexTexture[t] == oldHandle)
                s->desc.vertexTexture[t] = newHandle;
        }
    }
}

//------------------------------------------------------------------------------

HDepthStencilState AcquireDepthStencilState(const DepthStencilState::Descriptor& desc)
{
    Handle ds = InvalidHandle;
    DAVA::LockGuard<DAVA::Mutex> lock(_DepthStencilStateInfoMutex);
    for (std::vector<DepthStencilState_t>::iterator i = _DepthStencilStateInfo.begin(), i_end = _DepthStencilStateInfo.end(); i != i_end; ++i)
    {
        if (memcmp(&(i->desc), &desc, sizeof(DepthStencilState::Descriptor)) == 0)
        {
            ds = i->state;
            ++i->refCount;
            break;
        }
    }

    if (ds == InvalidHandle)
    {
        DepthStencilState_t info;

        info.desc = desc;
        info.state = DepthStencilState::Create(desc);
        info.refCount = 1;

        _DepthStencilStateInfo.push_back(info);
        ds = info.state;
    }

    return HDepthStencilState(ds);
}

//------------------------------------------------------------------------------

HDepthStencilState CopyDepthStencilState(HDepthStencilState ds)
{
    HDepthStencilState handle;

    DAVA::LockGuard<DAVA::Mutex> lock(_DepthStencilStateInfoMutex);
    for (std::vector<DepthStencilState_t>::iterator i = _DepthStencilStateInfo.begin(), i_end = _DepthStencilStateInfo.end(); i != i_end; ++i)
    {
        if (i->state == ds)
        {
            ++i->refCount;
            handle = ds;
            break;
        }
    }

    return handle;
}

//------------------------------------------------------------------------------

void ReleaseDepthStencilState(HDepthStencilState ds, bool scheduleDeletion)
{
    DAVA::LockGuard<DAVA::Mutex> lock(_DepthStencilStateInfoMutex);
    for (std::vector<DepthStencilState_t>::iterator i = _DepthStencilStateInfo.begin(), i_end = _DepthStencilStateInfo.end(); i != i_end; ++i)
    {
        if (i->state == ds)
        {
            if (--i->refCount == 0)
            {
                if (scheduleDeletion)
                    RenderLoop::ScheduleResourceDeletion(i->state, RESOURCE_DEPTHSTENCIL_STATE);
                else
                    DepthStencilState::Delete(i->state);

                _DepthStencilStateInfo.erase(i);
            }

            break;
        }
    }
}

//------------------------------------------------------------------------------

HSamplerState AcquireSamplerState(const SamplerState::Descriptor& desc)
{
    Handle ss = InvalidHandle;

    DAVA::LockGuard<DAVA::Mutex> lock(_SamplerStateInfoMutex);
    for (std::vector<SamplerState_t>::iterator i = _SamplerStateInfo.begin(), i_end = _SamplerStateInfo.end(); i != i_end; ++i)
    {
        if (memcmp(&(i->desc), &desc, sizeof(SamplerState::Descriptor)) == 0)
        {
            ss = i->state;
            ++i->refCount;
            break;
        }
    }

    if (ss == InvalidHandle)
    {
        SamplerState_t info;

        info.desc = desc;
        info.state = SamplerState::Create(desc);
        info.refCount = 1;

        _SamplerStateInfo.push_back(info);
        ss = info.state;
    }

    return HSamplerState(ss);
}

//------------------------------------------------------------------------------

HSamplerState CopySamplerState(HSamplerState ss)
{
    Handle handle = InvalidHandle;

    DAVA::LockGuard<DAVA::Mutex> lock(_SamplerStateInfoMutex);
    for (std::vector<SamplerState_t>::iterator i = _SamplerStateInfo.begin(), i_end = _SamplerStateInfo.end(); i != i_end; ++i)
    {
        if (i->state == ss)
        {
            ++i->refCount;
            handle = i->state;
            break;
        }
    }

    return HSamplerState(handle);
}

//------------------------------------------------------------------------------

void ReleaseSamplerState(HSamplerState ss, bool scheduleDeletion)
{
    DAVA::LockGuard<DAVA::Mutex> lock(_SamplerStateInfoMutex);
    for (std::vector<SamplerState_t>::iterator i = _SamplerStateInfo.begin(), i_end = _SamplerStateInfo.end(); i != i_end; ++i)
    {
        if (i->state == ss)
        {
            if (--i->refCount == 0)
            {
                if (scheduleDeletion)
                    RenderLoop::ScheduleResourceDeletion(i->state, RESOURCE_SAMPLER_STATE);
                else
                    SamplerState::Delete(i->state);

                _SamplerStateInfo.erase(i);
            }

            break;
        }
    }
}
//------------------------------------------------------------------------------

HTexture CreateTexture(const Texture::Descriptor& desc)
{
    return HTexture(Texture::Create(desc));
}

//------------------------------------------------------------------------------

void DeleteTexture(HTexture tex, bool scheduleDeletion)
{
    if (scheduleDeletion)
        RenderLoop::ScheduleResourceDeletion(tex, RESOURCE_TEXTURE);
    else
        Texture::Delete(tex);
}

//------------------------------------------------------------------------------

void* MapTexture(HTexture tex, uint32 level)
{
    return Texture::Map(tex, level);
}

//------------------------------------------------------------------------------

void UnmapTexture(HTexture tex)
{
    Texture::Unmap(tex);
}

//------------------------------------------------------------------------------

void UpdateTexture(HTexture tex, const void* data, uint32 level, TextureFace face)
{
    Texture::Update(tex, data, level, face);
}

//------------------------------------------------------------------------------

bool NeedRestoreTexture(HTexture tex)
{
    return Texture::NeedRestore(tex);
}

//------------------------------------------------------------------------------

HSyncObject CreateSyncObject()
{
    return HSyncObject(SyncObject::Create());
}

//------------------------------------------------------------------------------

void DeleteSyncObject(HSyncObject obj)
{
    SyncObject::Delete(obj);
}

//------------------------------------------------------------------------------

bool SyncObjectSignaled(HSyncObject obj)
{
    return SyncObject::IsSignaled(obj);
}

} //namespace rhi
