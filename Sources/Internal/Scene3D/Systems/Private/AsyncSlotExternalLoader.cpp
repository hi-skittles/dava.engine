#include "Scene3D/Systems/Private/AsyncSlotExternalLoader.h"

#include "Engine/Engine.h"
#include "Engine/EngineContext.h"

#include "Scene3D/Scene.h"
#include "Scene3D/Entity.h"
#include "Concurrency/LockGuard.h"
#include "Job/JobManager.h"
#include "Logger/Logger.h"
#include "Base/FastName.h"

namespace DAVA
{
void AsyncSlotExternalLoader::Load(RefPtr<Entity> rootEntity, const FilePath& path, const DAVA::Function<void(String&&)>& finishCallback)
{
    {
        LockGuard<Mutex> guard(jobsMutex);
        LoadingResult result;
        result.finishCallback = finishCallback;
        jobsMap.emplace(rootEntity, result);
    }

    {
        LockGuard<Mutex> guard(queueMutes);
        loadingQueue.push_back(LoadTask());
        LoadTask& task = loadingQueue.back();
        task.filePath = path;
        task.rootEntity = rootEntity;
    }

    ApplyNextJob();
}

void AsyncSlotExternalLoader::Process(float32 delta)
{
    LockGuard<Mutex> guard(jobsMutex);
    auto currentIter = jobsMap.begin();
    auto endIter = jobsMap.end();
    while (currentIter != endIter)
    {
        RefPtr<Entity> rootEntity = currentIter->first;
        LoadingResult result = currentIter->second;
        if (result.scene.Get() != nullptr)
        {
            int32 childCount = result.scene->GetChildrenCount();
            while (childCount > 0)
            {
                rootEntity->AddNode(result.scene->GetChild(childCount - 1));
                childCount = result.scene->GetChildrenCount();
            }

            currentIter = jobsMap.erase(currentIter);
            if (result.finishCallback)
            {
                result.finishCallback(std::move(result.error));
            }
        }
        else
        {
            ++currentIter;
        }
    }
}

void AsyncSlotExternalLoader::LoadImpl(RefPtr<Entity> rootEntity, const FilePath& path)
{
#if defined(__DAVAENGINE_DEBUG__)
    {
        LockGuard<Mutex> guard(jobsMutex);
        DVASSERT(jobsMap.count(rootEntity) > 0);
    }
#endif // DEBUG

    RefPtr<Scene> scene;
    scene.ConstructInplace();
    SceneFileV2::eError sceneLoadResult = scene->LoadScene(path);
    {
        LockGuard<Mutex> guard(jobsMutex);
        LoadingResult& result = jobsMap[rootEntity];
        result.scene = scene;
        if (sceneLoadResult != SceneFileV2::ERROR_NO_ERROR)
        {
            result.error = Format("[AsyncSlotExternalLoader] Couldn't load scene %s with code %d", path.GetAbsolutePathname().c_str(), sceneLoadResult);
        }
    }

    {
        LockGuard<Mutex> loadingGuard(queueMutes);
        isLoadingActive = false;
    }

    ApplyNextJob();
}

void AsyncSlotExternalLoader::ApplyNextJob()
{
    {
        LockGuard<Mutex> loadingGuard(queueMutes);
        if (isLoadingActive == true)
        {
            return;
        }

        if (loadingQueue.empty() == true)
        {
            return;
        }

        isLoadingActive = true;
    }

    JobManager* jobMng = GetEngineContext()->jobManager;
    std::shared_ptr<AsyncSlotExternalLoader> loaderRef = std::static_pointer_cast<AsyncSlotExternalLoader>(shared_from_this());
    LoadTask task = loadingQueue.front();
    loadingQueue.pop_front();
    jobMng->CreateWorkerJob([loaderRef, task]() {
        loaderRef->LoadImpl(task.rootEntity, task.filePath);
    });
}

void AsyncSlotExternalLoader::Reset()
{
    {
        LockGuard<Mutex> loadingGuard(queueMutes);
        loadingQueue.clear();
    }

    {
        LockGuard<Mutex> jobGuard(jobsMutex);
        for (auto& node : jobsMap)
        {
            node.second.finishCallback = Function<void(String && )>();
        }
    }
}

} // namespace DAVA
