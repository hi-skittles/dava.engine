#pragma once

#include "Scene3D/Systems/SlotSystem.h"

#include "Concurrency/Mutex.h"
#include "Base/RefPtr.h"
#include "Base/Hash.h"

namespace DAVA
{
class AsyncSlotExternalLoader final : public SlotSystem::ExternalEntityLoader
{
public:
    void Load(RefPtr<Entity> rootEntity, const FilePath& path, const DAVA::Function<void(String&&)>& finishCallback) override;
    void Process(float32 delta) override;
    void Reset() override;

    void LoadImpl(RefPtr<Entity> rootEntity, const FilePath& path);

private:
    void ApplyNextJob();
    struct HashRefPtrEntity
    {
        size_t operator()(const RefPtr<Entity>& pointer) const;
    };

    struct LoadingResult
    {
        RefPtr<Scene> scene;
        String error;
        Function<void(String&&)> finishCallback;
    };
    UnorderedMap<RefPtr<Entity>, LoadingResult, HashRefPtrEntity> jobsMap;
    Mutex jobsMutex;

    Mutex queueMutes;
    struct LoadTask
    {
        RefPtr<Entity> rootEntity;
        FilePath filePath;
    };
    List<LoadTask> loadingQueue;
    bool isLoadingActive = false;
};

inline size_t AsyncSlotExternalLoader::HashRefPtrEntity::operator()(const RefPtr<Entity>& pointer) const
{
    return reinterpret_cast<size_t>(pointer.Get());
}

} // namespace DAVA
