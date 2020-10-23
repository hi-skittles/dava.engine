#ifndef __DAVAENGINE_DYNAMIC_BUFFER_ALLOCATOR_H_
#define __DAVAENGINE_DYNAMIC_BUFFER_ALLOCATOR_H_

#include "Render/RHI/rhi_Public.h"
#include "Base/BaseTypes.h"

namespace DAVA
{
namespace DynamicBufferAllocator
{
static const uint32 DEFAULT_PAGE_SIZE = 131072;

struct AllocResultVB
{
    rhi::HVertexBuffer buffer;
    uint8* data;
    uint32 baseVertex;
    uint32 allocatedVertices;
};

struct AllocResultIB
{
    rhi::HIndexBuffer buffer;
    uint16* data;
    uint32 baseIndex;
    uint32 allocatedindices;
};

AllocResultVB AllocateVertexBuffer(uint32 vertexSize, uint32 vertexCount);
AllocResultIB AllocateIndexBuffer(uint32 indexCount);

//it has a bit different life cycle - it is put to eviction queue only once greater size buffer is requested (so client code should still request it every frame), still trying to share existing one
rhi::HIndexBuffer AllocateQuadListIndexBuffer(uint32 quadCount);

void BeginFrame();
void EndFrame();
void Clear();

void SetPageSize(uint32 size);
}
}

#endif // !__DAVAENGINE_PARTICLE_RENDER_OBJECT_H_
