#ifndef __SCENE_PROCESSOR_H__
#define __SCENE_PROCESSOR_H__

#include "DAVAEngine.h"

class EntityProcessorBase : public DAVA::BaseObject
{
public:
    EntityProcessorBase()
    {
    }

    virtual void Init(){};
    virtual bool ProcessEntity(DAVA::Entity* entity, const DAVA::FastName& entityName, bool isExternal) = 0;
    virtual void Finalize(){};

    virtual bool NeedProcessExternal() const = 0;

protected:
    ~EntityProcessorBase(){};
};

class SceneProcessor
{
public:
    SceneProcessor(EntityProcessorBase* _processEntityItem = NULL);
    ~SceneProcessor();

    void SetEntityProcessor(EntityProcessorBase* _processEntityItem);
    bool Execute(DAVA::Scene* currentScene);

private:
    EntityProcessorBase* entityProcessor;
};

#endif