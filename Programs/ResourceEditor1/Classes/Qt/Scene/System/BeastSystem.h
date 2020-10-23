#ifndef __BEASTSYSTEM_H__
#define __BEASTSYSTEM_H__

#include "Base/BaseTypes.h"
#include "Entity/SceneSystem.h"

namespace DAVA
{
class Entity;
class Scene;
class KeyedArchive;
}

class BeastSystem : public DAVA::SceneSystem
{
public:
    BeastSystem(DAVA::Scene* scene);

    void static SetDefaultPropertyValues(DAVA::Entity* entity);

    void AddEntity(DAVA::Entity* entity) override;
    void PrepareForRemove() override;

    static DAVA::float32 DEFAULT_FALLOFFCUTOFF_VALUE;

private:
    static void SetBool(DAVA::KeyedArchive* propertyList, const DAVA::String& key, bool value);
    static void SetFloat(DAVA::KeyedArchive* propertyList, const DAVA::String& key, DAVA::float32 value);
    static void SetInt32(DAVA::KeyedArchive* propertyList, const DAVA::String& key, DAVA::int32 value);
};


#endif /* defined(__BEASTSYSTEM_H__) */
