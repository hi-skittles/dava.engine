#include "ModuleManager/ModuleManager.h"
#include "ModuleManager/IModule.h"
#include "Reflection/ReflectedTypeDB.h"

namespace DAVA
{
extern Vector<IModule*> CreateModuleInstances(Engine* engine);

ModuleManager::ModuleManager(Engine* engine)
{
    modules = CreateModuleInstances(engine);
    for (IModule* module : modules)
    {
        const ReflectedType* type = ReflectedTypeDB::GetByPointer(module);
        if (type != nullptr)
        {
            searchIndex.emplace(type->GetType(), module);
        }
    }
}

ModuleManager::~ModuleManager()
{
    DVASSERT(modules.empty());
    DVASSERT(searchIndex.empty());
}

void ModuleManager::InitModules()
{
    for (IModule* module : modules)
    {
        module->Init();
    }
}

void ModuleManager::ShutdownModules()
{
    for (auto it = rbegin(modules); it != rend(modules); ++it)
    {
        (*it)->Shutdown();
    }

    for (auto it = rbegin(modules); it != rend(modules); ++it)
    {
        delete *it;
    }
    modules.clear();
    searchIndex.clear();
}

DAVA::IModule* ModuleManager::GetModule(const String& permanentName) const
{
    for (auto& node : searchIndex)
    {
        const ReflectedType* refType = ReflectedTypeDB::GetByType(node.first);
        if (refType->GetPermanentName() == permanentName)
        {
            return node.second;
        }
    }

    return nullptr;
}
}
