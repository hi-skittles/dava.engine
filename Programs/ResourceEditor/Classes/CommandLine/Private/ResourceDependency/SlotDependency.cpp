#include "Classes/CommandLine/Private/ResourceDependency/SlotDependency.h"
#include "Classes/CommandLine/Private/ResourceDependency/ResourceDependencyConstants.h"

#include <Render/TextureDescriptor.h>
#include <Render/TextureDescriptorUtils.h>
#include <Scene3D/Systems/SlotSystem.h>

bool SlotDependency::GetDependencies(const DAVA::FilePath& slotConfigPath, DAVA::Set<DAVA::FilePath>& dependencies, DAVA::int32 requestedType)
{
    using namespace DAVA;

    if (requestedType == static_cast<eDependencyType>(eDependencyType::CONVERT))
    { // right now we don't have dependencies for convertion of slots
        return true;
    }
    else if (requestedType == static_cast<eDependencyType>(eDependencyType::DOWNLOAD))
    {
        SlotSystem::ItemsCache cache;
        Vector<SlotSystem::ItemsCache::Item> slotItems = cache.GetItems(slotConfigPath);
        for (const SlotSystem::ItemsCache::Item& item : slotItems)
        {
            dependencies.insert(item.scenePath);
        }
    }

    return false;
};
