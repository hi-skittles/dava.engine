#include "rhi_NullRenderer.h"
#include "../rhi_Type.h"

#include "../Common/rhi_BackendImpl.h"
#include "../Common/rhi_Pool.h"
#include "../Common/rhi_Utils.h"

namespace rhi
{
struct TextureNull_t : public ResourceImpl<TextureNull_t, Texture::Descriptor>
{
    void* mappedData = nullptr;
};
RHI_IMPL_RESOURCE(TextureNull_t, Texture::Descriptor)

using TextureNullPool = ResourcePool<TextureNull_t, RESOURCE_TEXTURE, Texture::Descriptor>;
RHI_IMPL_POOL(TextureNull_t, RESOURCE_TEXTURE, Texture::Descriptor, false);

//////////////////////////////////////////////////////////////////////////

Handle null_Texture_Create(const Texture::Descriptor& desc)
{
    Handle h = TextureNullPool::Alloc();
    TextureNull_t* self = TextureNullPool::Get(h);
    self->UpdateCreationDesc(desc);

    return h;
}

void null_Texture_Delete(Handle h, bool)
{
    TextureNull_t* self = TextureNullPool::Get(h);
    DVASSERT(self->mappedData == nullptr);

    TextureNullPool::Free(h);
}

void* null_Texture_Map(Handle h, unsigned level, TextureFace)
{
    TextureNull_t* self = TextureNullPool::Get(h);

    TextureFormat format = self->CreationDesc().format;
    uint32 data_sz = TextureSize(format, self->CreationDesc().width, self->CreationDesc().height, level);
    self->mappedData = ::malloc(data_sz);

    return self->mappedData;
}

void null_Texture_Unmap(Handle h)
{
    TextureNull_t* self = TextureNullPool::Get(h);
    DVASSERT(self->mappedData != nullptr);

    ::free(self->mappedData);
    self->mappedData = nullptr;
}

void null_Texture_Update(Handle, const void*, uint32, TextureFace)
{
}

bool null_Texture_NeedRestore(Handle)
{
    return false;
}

//////////////////////////////////////////////////////////////////////////

namespace TextureNull
{
void Init(uint32 maxCount)
{
    TextureNullPool::Reserve(maxCount);
}

void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_Texture_Create = null_Texture_Create;
    dispatch->impl_Texture_Delete = null_Texture_Delete;
    dispatch->impl_Texture_Map = null_Texture_Map;
    dispatch->impl_Texture_Unmap = null_Texture_Unmap;
    dispatch->impl_Texture_Update = null_Texture_Update;
    dispatch->impl_Texture_NeedRestore = null_Texture_NeedRestore;
}
}
} //ns rhi