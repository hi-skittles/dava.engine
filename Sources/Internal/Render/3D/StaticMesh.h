#ifndef __DAVAENGINE_STATICMESHGLES_H__
#define __DAVAENGINE_STATICMESHGLES_H__

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Render/3D/PolygonGroup.h"

namespace DAVA
{
class SceneFileV2;
class StaticMesh : public DataNode
{
protected:
    ~StaticMesh();

public:
    StaticMesh(Scene* _scene = 0);

    virtual void SetScene(Scene* _scene);
    virtual int32 Release();

    virtual void AddNode(DataNode* node);

    uint32 GetPolygonGroupCount();
    PolygonGroup* GetPolygonGroup(uint32 index);

protected:
    Vector<DataNode*> children;
};
};

#endif // __DAVAENGINE_STATICMESHGLES_H__
