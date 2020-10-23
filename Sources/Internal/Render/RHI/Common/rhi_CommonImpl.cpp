#include "../rhi_Public.h"
#include "rhi_Private.h"
#include "rhi_Pool.h"

#include "rhi_CommonImpl.h"
namespace rhi
{
typedef ResourcePool<CommonImpl::TextureSet_t, RESOURCE_TEXTURE_SET, CommonImpl::TextureSet_t::Desc, false> TextureSetPool;
RHI_IMPL_POOL(CommonImpl::TextureSet_t, RESOURCE_TEXTURE_SET, CommonImpl::TextureSet_t::Desc, false);

namespace CommonImpl
{
void Frame::Reset()
{
    sync = InvalidHandle;
    perfQueryStart = InvalidHandle;
    perfQueryEnd = InvalidHandle;
    pass.clear();
    readyToExecute = false;
    discarded = false;
}
}

namespace TextureSet
{
Handle Create()
{
    return TextureSetPool::Alloc();
}
CommonImpl::TextureSet_t* Get(Handle h)
{
    return TextureSetPool::Get(h);
}
void Delete(Handle h)
{
    TextureSetPool::Free(h);
}
void InitTextreSetPool(uint32 maxCount)
{
    TextureSetPool::Reserve(maxCount);
}
}
}
