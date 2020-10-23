#include "../Common/rhi_Private.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"
#include "rhi_GLES2.h"

#include "Debug/DVAssert.h"
#include "Logger/Logger.h"
using DAVA::Logger;

#include "_gl.h"

namespace rhi
{
//==============================================================================

struct
IndexBufferGLES2_t
: public ResourceImpl<IndexBufferGLES2_t, IndexBuffer::Descriptor>
{
public:
    IndexBufferGLES2_t();

    bool Create(const IndexBuffer::Descriptor& desc, bool forceExecute = false);
    void Destroy(bool forceExecute);

    unsigned size;
    GLenum usage;
    unsigned uid;
    void* mappedData = nullptr;
    uint32 isMapped : 1;
    uint32 updatePending : 1;
    uint32 is_32bit : 1;
    uint32 isUPBuffer : 1;
};

RHI_IMPL_RESOURCE(IndexBufferGLES2_t, IndexBuffer::Descriptor)

typedef ResourcePool<IndexBufferGLES2_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor, true> IndexBufferGLES2Pool;
RHI_IMPL_POOL_SIZE(IndexBufferGLES2_t, RESOURCE_INDEX_BUFFER, IndexBuffer::Descriptor, true, 3072);

//------------------------------------------------------------------------------

IndexBufferGLES2_t::IndexBufferGLES2_t()
    : size(0)
    , usage(GL_STATIC_DRAW)
    , uid(0)
    , isMapped(0)
    , updatePending(0)
    , is_32bit(false)
    , isUPBuffer(false)
{
}

//------------------------------------------------------------------------------

bool IndexBufferGLES2_t::Create(const IndexBuffer::Descriptor& desc, bool forceExecute)
{
    bool success = false;
    UpdateCreationDesc(desc);

    DVASSERT(desc.size);
    if (desc.size)
    {
        isUPBuffer = (desc.usage == USAGE_DYNAMICDRAW) && _GLES2_UseUserProvidedIndices;

        switch (desc.usage)
        {
        case USAGE_DEFAULT:
            usage = GL_DYNAMIC_DRAW;
            break;
        case USAGE_STATICDRAW:
            usage = GL_STATIC_DRAW;
            break;
        case USAGE_DYNAMICDRAW:
            usage = GL_DYNAMIC_DRAW;
            break;
        }

        GLuint b = 0;
        if (isUPBuffer)
        {
            mappedData = reinterpret_cast<uint8*>(::malloc(desc.size));

            if (desc.initialData)
                memcpy(mappedData, desc.initialData, desc.size);

            success = true;
        }
        else
        {
            GLCommand cmd[] =
            {
              { GLCommand::GEN_BUFFERS, { 1, reinterpret_cast<uint64>(&b) } },
              { GLCommand::BIND_BUFFER, { GL_ELEMENT_ARRAY_BUFFER, uint64(&b) } },
              { GLCommand::BUFFER_DATA, { GL_ELEMENT_ARRAY_BUFFER, desc.size, reinterpret_cast<uint64>(desc.initialData), usage } },
              { GLCommand::RESTORE_INDEX_BUFFER, {} }
            };

            if (!desc.initialData)
            {
                DVASSERT(desc.usage != USAGE_STATICDRAW);
                cmd[2].func = GLCommand::NOP;
            }

            ExecGL(cmd, countof(cmd), forceExecute);

            if (cmd[1].status == GL_NO_ERROR)
            {
                success = true;
                mappedData = nullptr;
            }
        }

        if (success)
        {
            size = desc.size;
            uid = b;
            is_32bit = desc.indexSize == INDEX_SIZE_32BIT;
            isMapped = false;
            updatePending = false;
        }
    }

    return success;
}

//------------------------------------------------------------------------------

void IndexBufferGLES2_t::Destroy(bool forceExecute)
{
    if (uid)
    {
        GLCommand cmd = { GLCommand::DELETE_BUFFERS, { 1, reinterpret_cast<uint64>(&uid) } };
        ExecGL(&cmd, 1, forceExecute);
    }

    if (mappedData)
    {
        ::free(mappedData);
        mappedData = nullptr;
    }

    size = 0;
    uid = 0;
    MarkRestored();
}

//==============================================================================

static Handle
gles2_IndexBuffer_Create(const IndexBuffer::Descriptor& desc)
{
    Handle handle = IndexBufferGLES2Pool::Alloc();
    IndexBufferGLES2_t* ib = IndexBufferGLES2Pool::Get(handle);

    if (ib->Create(desc) == false)
    {
        IndexBufferGLES2Pool::Free(handle);
        handle = InvalidHandle;
    }

    return handle;
}

//------------------------------------------------------------------------------

static void
gles2_IndexBuffer_Delete(Handle ib, bool forceExecute)
{
    IndexBufferGLES2_t* self = IndexBufferGLES2Pool::Get(ib);
    self->Destroy(forceExecute);
    IndexBufferGLES2Pool::Free(ib);
}

//------------------------------------------------------------------------------

static bool
gles2_IndexBuffer_Update(Handle ib, const void* data, unsigned offset, unsigned size)
{
    bool success = false;
    IndexBufferGLES2_t* self = IndexBufferGLES2Pool::Get(ib);

    DVASSERT(!self->isMapped);
    DVASSERT(self->usage != GL_STATIC_DRAW);

    if (offset + size <= self->size)
    {
        if (self->isUPBuffer)
        {
            DVASSERT(self->mappedData);
            memcpy(static_cast<uint8*>(self->mappedData) + offset, static_cast<const uint8*>(data) + offset, size);

            success = true;
        }
        else
        {
            GLCommand cmd[] =
            {
              { GLCommand::BIND_BUFFER, { GL_ELEMENT_ARRAY_BUFFER, uint64(&(self->uid)) } },
              { GLCommand::BUFFER_DATA, { GL_ELEMENT_ARRAY_BUFFER, self->size, reinterpret_cast<uint64>(data), self->usage } },
              { GLCommand::RESTORE_INDEX_BUFFER, {} }
            };

            ExecGL(cmd, countof(cmd));
            success = cmd[1].status == GL_NO_ERROR;
        }
        self->MarkRestored();
    }

    return success;
}

//------------------------------------------------------------------------------

static void*
gles2_IndexBuffer_Map(Handle ib, unsigned offset, unsigned size)
{
    IndexBufferGLES2_t* self = IndexBufferGLES2Pool::Get(ib);
    void* data = nullptr;

    DVASSERT(self->usage != GL_STATIC_DRAW);
    DVASSERT(!self->isMapped);

    if (offset + size <= self->size)
    {
        if (!self->mappedData)
            self->mappedData = ::malloc(self->size);

        self->isMapped = true;
        data = static_cast<uint8*>(self->mappedData) + offset;
    }

    return data;
}

//------------------------------------------------------------------------------

static void
gles2_IndexBuffer_Unmap(Handle ib)
{
    IndexBufferGLES2_t* self = IndexBufferGLES2Pool::Get(ib);

    DVASSERT(self->usage != GL_STATIC_DRAW);
    DVASSERT(self->isMapped);

    if (self->isUPBuffer)
    {
        self->isMapped = false;
        self->MarkRestored();
    }
    else if (self->usage == GL_DYNAMIC_DRAW)
    {
        self->isMapped = false;
        self->updatePending = true;
    }
    else
    {
        GLCommand cmd[] =
        {
          { GLCommand::BIND_BUFFER, { GL_ELEMENT_ARRAY_BUFFER, uint64(&(self->uid)) } },
          { GLCommand::BUFFER_DATA, { GL_ELEMENT_ARRAY_BUFFER, self->size, reinterpret_cast<uint64>(self->mappedData), self->usage } },
          { GLCommand::RESTORE_INDEX_BUFFER, {} }
        };

        ExecGL(cmd, countof(cmd));

        self->isMapped = false;
        self->MarkRestored();

        ::free(self->mappedData);
        self->mappedData = nullptr;
    }
}

//------------------------------------------------------------------------------

static bool
gles2_IndexBuffer_NeedRestore(Handle ib)
{
    IndexBufferGLES2_t* self = IndexBufferGLES2Pool::Get(ib);

    return self->NeedRestore();
}

//------------------------------------------------------------------------------

namespace IndexBufferGLES2
{
void Init(uint32 maxCount)
{
    IndexBufferGLES2Pool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_IndexBuffer_Create = &gles2_IndexBuffer_Create;
    dispatch->impl_IndexBuffer_Delete = &gles2_IndexBuffer_Delete;
    dispatch->impl_IndexBuffer_Update = &gles2_IndexBuffer_Update;
    dispatch->impl_IndexBuffer_Map = &gles2_IndexBuffer_Map;
    dispatch->impl_IndexBuffer_Unmap = &gles2_IndexBuffer_Unmap;
    dispatch->impl_IndexBuffer_NeedRestore = &gles2_IndexBuffer_NeedRestore;
}

IndexSize
SetToRHI(Handle ib)
{
    IndexBufferGLES2_t* self = IndexBufferGLES2Pool::Get(ib);

    DVASSERT(!self->isMapped);

    if (self->isUPBuffer)
    {
        DVASSERT(self->mappedData);

        _GLES2_LastSetIndices = static_cast<uint8*>(self->mappedData);

        GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
        _GLES2_LastSetIB = 0;
    }
    else
    {
        GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, self->uid));
        _GLES2_LastSetIB = self->uid;
        _GLES2_LastSetIndices = nullptr;

        if (self->updatePending)
        {
            DVASSERT(self->mappedData);
            GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, self->size, self->mappedData, self->usage));
            self->updatePending = false;
        }
    }

    return (self->is_32bit) ? INDEX_SIZE_32BIT : INDEX_SIZE_16BIT;
}

void ReCreateAll()
{
    IndexBufferGLES2Pool::ReCreateAll();
}

unsigned
NeedRestoreCount()
{
    return IndexBufferGLES2Pool::PendingRestoreCount();
}

} // namespace IndexBufferGLES

//==============================================================================
} // namespace rhi
