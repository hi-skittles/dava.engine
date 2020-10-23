#ifndef __DAVAENGINE_ALLOCPOOLS_H__
#define __DAVAENGINE_ALLOCPOOLS_H__

namespace DAVA
{
// Predefined allocation pools
enum ePredefAllocPools
{
    ALLOC_POOL_TOTAL = 0, // Virtual allocation pool for total allocations - sum of all other allocation pools
    ALLOC_POOL_DEFAULT, // Allocation pool used for all other application memory allocations, except custom if any
    ALLOC_GPU_TEXTURE, // Virtual allocation pool for GPU textures
    ALLOC_GPU_RDO_VERTEX, // Virtual allocation pool for GPU vertices from RenderDataObject
    ALLOC_GPU_RDO_INDEX, // Virtual allocation pool for GPU indices from RenderDataObject
    ALLOC_POOL_SYSTEM, // Virtual allocation pool for memory usage reported by system
    ALLOC_POOL_FMOD,
    ALLOC_POOL_BULLET,
    ALLOC_POOL_BASEOBJECT,
    ALLOC_POOL_POLYGONGROUP,
    ALLOC_POOL_COMPONENT,
    ALLOC_POOL_ENTITY,
    ALLOC_POOL_LANDSCAPE,
    ALLOC_POOL_IMAGE,
    ALLOC_POOL_TEXTURE,
    ALLOC_POOL_NMATERIAL,

    ALLOC_POOL_RHI_BUFFER,
    ALLOC_POOL_RHI_VERTEX_MAP,
    ALLOC_POOL_RHI_INDEX_MAP,
    ALLOC_POOL_RHI_TEXTURE_MAP,
    ALLOC_POOL_RHI_RESOURCE_POOL,

    ALLOC_POOL_LUA,
    ALLOC_POOL_SQLITE,

    ALLOC_POOL_PHYSICS,

    PREDEF_POOL_COUNT,
    FIRST_CUSTOM_ALLOC_POOL = PREDEF_POOL_COUNT // First custom allocation pool must be FIRST_CUSTOM_ALLOC_POOL
};

} // namespace DAVA

#endif // __DAVAENGINE_ALLOCPOOLS_H__
