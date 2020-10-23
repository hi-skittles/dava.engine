#pragma once

#include "Base/Vector.h"
#include "Base/UnordererMap.h"
#include "Base/Type.h"
#include "Base/Any.h"

namespace DAVA
{
class IModule;
class Engine;

class ModuleManager final
{
public:
    ModuleManager(Engine* engine);
    ~ModuleManager();

    template <typename T>
    T* GetModule() const;

    IModule* GetModule(const String& permanentName) const;

    void InitModules();
    void ShutdownModules();

private:
    Vector<IModule*> modules;
    UnorderedMap<const Type*, IModule*> searchIndex;
};

template <typename T>
T* ModuleManager::GetModule() const
{
    const Type* requestType = Type::Instance<T>();
    auto iter = searchIndex.find(requestType);
    DVASSERT(iter != searchIndex.end());
    return static_cast<T*>(iter->second);
}
}
