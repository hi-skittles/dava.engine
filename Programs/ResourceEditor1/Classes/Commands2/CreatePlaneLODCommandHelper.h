#ifndef __CREATE_PLANE_LOD_COOMAND_HELPER_H__
#define __CREATE_PLANE_LOD_COOMAND_HELPER_H__

#include "DAVAEngine.h"
#include "Base/TypeHolders.h"
#include "Scene3D/Lod/LodComponent.h"

namespace CreatePlaneLODCommandHelper
{
struct Request : public DAVA::RefCounter
{
    DAVA::LodComponent* lodComponent = nullptr;
    DAVA::RenderBatch* planeBatch = nullptr;
    DAVA::Image* planeImage = nullptr;
    DAVA::Texture* targetTexture = nullptr;
    DAVA::int32 fromLodLayer = 0;
    DAVA::int32 newLodIndex = 0;
    DAVA::uint32 textureSize = 0;
    DAVA::FilePath texturePath;
    DAVA::Atomic<bool> completed;
    rhi::HTexture depthTexture;

    Request();
    ~Request();
    void RegisterRenderCallback();
    void OnRenderCallback(rhi::HSyncObject object);
    void ReloadTexturesToGPU(DAVA::eGPUFamily);
};
using RequestPointer = DAVA::RefPtr<Request>;

RequestPointer RequestRenderToTexture(DAVA::LodComponent* lodComponent, DAVA::int32 fromLodLayer,
                                      DAVA::uint32 textureSize, const DAVA::FilePath& texturePath);
};

#endif // #ifndef __CREATE_PLANE_LOD_COOMAND_H__
