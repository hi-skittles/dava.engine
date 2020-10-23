#ifndef __DAVAENGINE_RENDER_HIGHLEVEL_SHADOW_VOLUME_H__
#define __DAVAENGINE_RENDER_HIGHLEVEL_SHADOW_VOLUME_H__

#include "Render/3D/PolygonGroup.h"
#include "Render/Shader.h"
#include "Render/3D/EdgeAdjacency.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class PolygonGroup;
class Light;

class ShadowVolume : public RenderBatch
{
protected:
    virtual ~ShadowVolume();

public:
    ShadowVolume();
};
}

#endif //__DAVAENGINE_RENDER_HIGHLEVEL_SHADOW_VOLUME_H__
