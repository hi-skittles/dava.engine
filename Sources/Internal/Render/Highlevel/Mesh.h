#ifndef __DAVAENGINE_SCENE3D_MESH_H__
#define __DAVAENGINE_SCENE3D_MESH_H__

#include "Base/BaseTypes.h"
#include "Animation/AnimatedObject.h"
#include "Base/BaseMath.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Render/Highlevel/RenderObject.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class PolygonGroup;
class RenderBatch;
class NMaterial;

class Mesh : public RenderObject
{
public:
    Mesh();
    virtual ~Mesh();

    void AddPolygonGroup(PolygonGroup* polygonGroup, NMaterial* material);

    uint32 GetPolygonGroupCount();
    PolygonGroup* GetPolygonGroup(uint32 index);

    virtual RenderObject* Clone(RenderObject* newObject);

    virtual void Save(KeyedArchive* archive, SerializationContext* serializationContext);
    virtual void Load(KeyedArchive* archive, SerializationContext* serializationContext);

    virtual void BakeGeometry(const Matrix4& transform);

protected:
};

} // ns

#endif /* __DAVAENGINE_SCENE3D_RENDEROBJECT_H__ */