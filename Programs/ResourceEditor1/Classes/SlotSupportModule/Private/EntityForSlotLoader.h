#pragma once

#include <TArc/Core/ContextAccessor.h>
#include <Scene3D/Systems/SlotSystem.h>

namespace DAVA
{
class Entity;
class FilePath;
}

class EntityForSlotLoader : public DAVA::SlotSystem::ExternalEntityLoader
{
public:
    EntityForSlotLoader(DAVA::TArc::ContextAccessor* accessor);

    void Load(DAVA::RefPtr<DAVA::Entity> rootEntity, const DAVA::FilePath& path, const DAVA::Function<void(DAVA::String&&)>& finishCallback) override;
    void AddEntity(DAVA::Entity* parent, DAVA::Entity* child) override;
    void Process(DAVA::float32 delta) override;

protected:
    void Reset() override;

private:
    DAVA::TArc::ContextAccessor* accessor = nullptr;
    struct CallbackInfo
    {
        DAVA::Function<void(DAVA::String&&)> callback;
        DAVA::String message;
    };
    DAVA::Vector<CallbackInfo> callbacks;
};
