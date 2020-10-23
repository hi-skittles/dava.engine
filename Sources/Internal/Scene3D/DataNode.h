#pragma once

#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Reflection/Reflection.h"
#include "Render/RenderBase.h"
#include "Scene3D/Entity.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
/**
    
 */
class DataNode : public BaseObject
{
public:
    static const uint64 INVALID_ID = 0;

public:
    DataNode();

    void SetScene(Scene* _scene);
    Scene* GetScene() const;

    DataNode* FindByName(const String& searchName);

    uint64 GetNodeID() const;
    void SetNodeID(uint64 id);

    void SetRuntime(bool isRuntime);
    bool IsRuntime() const;

    virtual void Save(KeyedArchive* archive, SerializationContext* serializationContext);
    virtual void Load(KeyedArchive* archive, SerializationContext* serializationContext);

protected:
    virtual ~DataNode();

    uint64 id;
    bool isRuntime;
    Scene* scene;

    DAVA_VIRTUAL_REFLECTION(DataNode, BaseObject);
};
}
