#include "SceneProcessor.h"

using namespace DAVA;

SceneProcessor::SceneProcessor(EntityProcessorBase* _entityProcessor /*= NULL*/)
    : entityProcessor(SafeRetain(_entityProcessor))
{
}

SceneProcessor::~SceneProcessor()
{
    SafeRelease(entityProcessor);
}

void SceneProcessor::SetEntityProcessor(EntityProcessorBase* _entityProcessor)
{
    SafeRelease(entityProcessor);

    entityProcessor = SafeRetain(_entityProcessor);
}

bool SceneProcessor::Execute(DAVA::Scene* currentScene)
{
    if (!entityProcessor)
    {
        Logger::Warning("%s need to set EntityProcessor", __FUNCTION__);
        return false;
    }

    entityProcessor->Init();

    int32 childrenCount = currentScene->GetChildrenCount();

    Set<String> refToOwnerSet;

    const bool needProcessExternal = entityProcessor->NeedProcessExternal();
    bool sceneModified = false;

    for (int32 index = 0; index < childrenCount; index++)
    {
        Entity* currentEntity = currentScene->GetChild(index);

        bool entityModified = entityProcessor->ProcessEntity(currentEntity, currentEntity->GetName(), false);
        sceneModified = sceneModified || entityModified;
        if (entityModified && needProcessExternal)
        {
            KeyedArchive* props = GetCustomPropertiesArchieve(currentEntity);

            if (!props)
            {
                Logger::Warning("%s %s custom properties not found", __FUNCTION__, currentEntity->GetName().c_str());
                continue;
            }

            if (!props->IsKeyExists("editor.referenceToOwner"))
            {
                Logger::Error("%s editor.referenceToOwner not found for %s", __FUNCTION__, currentEntity->GetName().c_str());
                continue;
            }

            const String referenceToOwner = props->GetString("editor.referenceToOwner");
            bool newItemInserted = refToOwnerSet.insert(referenceToOwner).second;
            if (newItemInserted)
            {
                Scene* newScene = new Scene();
                newScene->LoadScene(referenceToOwner);
                DVASSERT(newScene->GetChildrenCount() == 1);
                entityProcessor->ProcessEntity(newScene, currentEntity->GetName(), true);
                newScene->SaveScene(referenceToOwner);
                SafeRelease(newScene);
            }
        }
    }

    entityProcessor->Finalize();
    return sceneModified;
}
