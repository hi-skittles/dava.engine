#include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"
#include "rhi_DX9.h"

#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
using DAVA::Logger;

#include "_dx9.h"

namespace rhi
{
//==============================================================================

class
IndexBufferDX9_t
: public ResourceImpl<IndexBufferDX9_t, IndexBuffer::Descriptor>
{
public:
    IndexBufferDX9_t();

    bool Create(const IndexBuffer::Descriptor& desc, bool forceExecute = false);
    void Destroy(bool forceExecute);

    uint32 size = 0;
    IDirect3DIndexBuffer9* buffer = nullptr;
    void* mappedData = nullptr;

    uint32 isMapped : 1;
    uint32 updatePending : 1;
};

RHI_IMPL_RESOURCE(IndexBufferDX9_t, IndexBuffer::Descriptor)

typedef ResourcePool<IndexBufferDX9_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor, true> IndexBufferDX9Pool;
RHI_IMPL_POOL(IndexBufferDX9_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor, true);

//==============================================================================

IndexBufferDX9_t::IndexBufferDX9_t()
    : isMapped(0)
    , updatePending(0)
{
}

//------------------------------------------------------------------------------

bool IndexBufferDX9_t::Create(const IndexBuffer::Descriptor& desc, bool forceExecute)
{
    DVASSERT(desc.size);
    bool success = false;

    SetRecreatePending(false);

    UpdateCreationDesc(desc);

    if (desc.size)
    {
        DWORD usage = D3DUSAGE_WRITEONLY;

        switch (desc.usage)
        {
        case USAGE_DEFAULT:
            usage = D3DUSAGE_WRITEONLY;
            break;
        case USAGE_STATICDRAW:
            usage = D3DUSAGE_WRITEONLY;
            break;
        case USAGE_DYNAMICDRAW:
            usage = D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC;
            break;
        }

        DVASSERT(buffer == nullptr);

        uint32 cmd_cnt = 2;
        uint64 indexSizeParam = static_cast<uint64>((desc.indexSize == INDEX_SIZE_32BIT) ? D3DFMT_INDEX32 : D3DFMT_INDEX16);
        DX9Command cmd[] =
        {
          { DX9Command::CREATE_INDEX_BUFFER, { desc.size, usage, indexSizeParam, D3DPOOL_DEFAULT, uint64_t(&buffer), NULL } },
          { DX9Command::UPDATE_INDEX_BUFFER, { uint64_t(&buffer), uint64_t(desc.initialData), desc.size } }
        };

        if (!desc.initialData)
        {
            cmd[1].func = DX9Command::NOP;
            cmd_cnt = 1;
        }

        ExecDX9(cmd, cmd_cnt, forceExecute);

        if (SUCCEEDED(cmd[0].retval))
        {
            size = desc.size;
            success = true;
        }
        else
        {
            Logger::Error("FAILED to create index-buffer:\n%s\n", D3D9ErrorText(cmd[0].retval));
        }
    }

    return success;
}

//------------------------------------------------------------------------------

void IndexBufferDX9_t::Destroy(bool forceExecute)
{
    if (buffer)
    {
        DX9Command cmd[] = { DX9Command::RELEASE, { uint64_t(&buffer) } };
        ExecDX9(cmd, countof(cmd), forceExecute);
        DVASSERT(cmd[0].retval == 0);
        buffer = nullptr;
    }
    else
    {
        SetRecreatePending(false);
    }

    if (!RecreatePending() && (mappedData != nullptr))
    {
        DVASSERT(!isMapped);
        ::free(mappedData);
        mappedData = nullptr;
        updatePending = false;
    }

    MarkRestored();
}

//------------------------------------------------------------------------------

static Handle
dx9_IndexBuffer_Create(const IndexBuffer::Descriptor& desc)
{
    Handle handle = IndexBufferDX9Pool::Alloc();
    IndexBufferDX9_t* ib = IndexBufferDX9Pool::Get(handle);

    if (ib->Create(desc) == false)
    {
        IndexBufferDX9Pool::Free(handle);
        handle = InvalidHandle;
    }

    return handle;
}

//------------------------------------------------------------------------------

static void
dx9_IndexBuffer_Delete(Handle ib, bool forceExecute)
{
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);
    self->SetRecreatePending(false);
    self->Destroy(forceExecute);
    IndexBufferDX9Pool::Free(ib);
}

//------------------------------------------------------------------------------

static bool
dx9_IndexBuffer_Update(Handle ib, const void* data, unsigned offset, unsigned size)
{
    bool success = false;
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);

    DVASSERT(!self->isMapped);

    if (offset + size <= self->size)
    {
        void* ptr = nullptr;
        DX9Command cmd1 = { DX9Command::LOCK_INDEX_BUFFER, { uint64_t(&(self->buffer)), offset, size, uint64_t(&ptr), 0 } };

        ExecDX9(&cmd1, 1, false);
        if (SUCCEEDED(cmd1.retval))
        {
            memcpy(ptr, data, size);

            DX9Command cmd2 = { DX9Command::UNLOCK_INDEX_BUFFER, { uint64_t(&(self->buffer)) } };

            ExecDX9(&cmd2, 1, false);
            success = true;

            self->MarkRestored();
        }
    }

    return success;
}

//------------------------------------------------------------------------------

static void*
dx9_IndexBuffer_Map(Handle ib, unsigned offset, unsigned size)
{
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);
    DVASSERT(self->CreationDesc().usage != Usage::USAGE_STATICDRAW);
    DVASSERT(offset + size <= self->size);
    DVASSERT(!self->isMapped);

    if (self->mappedData == nullptr)
    {
        self->mappedData = ::malloc(self->size);
    }

    DVASSERT(!self->isMapped);
    self->isMapped = true;

    return static_cast<uint8*>(self->mappedData) + offset;
}

//------------------------------------------------------------------------------

static void
dx9_IndexBuffer_Unmap(Handle ib)
{
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);
    DVASSERT(self->isMapped);
    DVASSERT(self->CreationDesc().usage != Usage::USAGE_STATICDRAW);

    self->isMapped = false;

    if (self->CreationDesc().usage == Usage::USAGE_DYNAMICDRAW)
    {
        self->updatePending = true;
    }
    else
    {
        DX9Command cmd = { DX9Command::UPDATE_INDEX_BUFFER, { uint64_t(&self->buffer), uint64_t(self->mappedData), self->size } };
        ExecDX9(&cmd, 1, true);

        ::free(self->mappedData);
        self->mappedData = nullptr;
        self->updatePending = false;
    }
}

//------------------------------------------------------------------------------

static bool
dx9_IndexBuffer_NeedRestore(Handle ib)
{
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);

    return self->NeedRestore();
}

//------------------------------------------------------------------------------

namespace IndexBufferDX9
{
void Init(uint32 maxCount)
{
    IndexBufferDX9Pool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_IndexBuffer_Create = &dx9_IndexBuffer_Create;
    dispatch->impl_IndexBuffer_Delete = &dx9_IndexBuffer_Delete;
    dispatch->impl_IndexBuffer_Update = &dx9_IndexBuffer_Update;
    dispatch->impl_IndexBuffer_Map = &dx9_IndexBuffer_Map;
    dispatch->impl_IndexBuffer_Unmap = &dx9_IndexBuffer_Unmap;
    dispatch->impl_IndexBuffer_NeedRestore = &dx9_IndexBuffer_NeedRestore;
}

void SetToRHI(Handle ib)
{
    IndexBufferDX9_t* self = IndexBufferDX9Pool::Get(ib);

    DVASSERT(!self->isMapped);
    if (self->updatePending)
    {
        void* bufferData = nullptr;
        HRESULT hr = self->buffer->Lock(0, self->size, &bufferData, D3DLOCK_DISCARD);
        DVASSERT(SUCCEEDED(hr));

        memcpy(bufferData, self->mappedData, self->size);

        hr = self->buffer->Unlock();
        DVASSERT(SUCCEEDED(hr));

        self->updatePending = false;
    }

    HRESULT hr = _D3D9_Device->SetIndices(self->buffer);

    if (FAILED(hr))
        Logger::Error("SetIndices failed:\n%s\n", D3D9ErrorText(hr));
}

void ReleaseAll()
{
    IndexBufferDX9Pool::ReleaseAll();
}

void ReCreateAll()
{
    IndexBufferDX9Pool::ReCreateAll();
}

void LogUnrestoredBacktraces()
{
    IndexBufferDX9Pool::LogUnrestoredBacktraces();
}

unsigned NeedRestoreCount()
{
    return IndexBufferDX9Pool::PendingRestoreCount();
}
}

//==============================================================================
} // namespace rhi
