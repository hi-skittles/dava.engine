#include "UI/DataBinding/Private/UIDataBindingDependenciesManager.h"

#include "Debug/DVAssert.h"

namespace DAVA
{
UIDataBindingDependenciesManager::UIDataBindingDependenciesManager()
{
}

UIDataBindingDependenciesManager::~UIDataBindingDependenciesManager()
{
}

int32 UIDataBindingDependenciesManager::MakeDependency(int32 id, const Vector<void*>& data)
{
    if (id == UNKNOWN_DEPENDENCY)
    {
        id = nextId;
        nextId++;
    }
    else
    {
        ReleaseDepencency(id);
    }

    dirtyBindings[id] = false;
    AddDependencies(id, data);
    return id;
}

void UIDataBindingDependenciesManager::AddDependencies(int32 id, const Vector<void*>& data)
{
    DVASSERT(id != UNKNOWN_DEPENDENCY);
    DVASSERT(dirtyBindings.find(id) != dirtyBindings.end());

    for (void* d : data)
    {
        auto it = dirtyMap.find(d);
        if (it == dirtyMap.end())
        {
            dirtyMap[d] = Vector<int>();
            it = dirtyMap.find(d);
        }

        bool haveToAddId = std::find(it->second.begin(), it->second.end(), id) == it->second.end();
        if (haveToAddId)
        {
            it->second.push_back(id);
        }
    }
}

void UIDataBindingDependenciesManager::ReleaseDepencency(int32 index)
{
    for (auto mapIt = dirtyMap.begin(); mapIt != dirtyMap.end();)
    {
        Vector<int>& v = mapIt->second;
        auto it = std::find(v.begin(), v.end(), index);
        bool haveToErase = false;
        if (it != v.end())
        {
            v.erase(it);
            if (v.empty())
            {
                haveToErase = true;
            }
        }

        if (haveToErase)
        {
            mapIt = dirtyMap.erase(mapIt);
        }
        else
        {
            ++mapIt;
        }
    }

    auto it = dirtyBindings.find(index);
    if (it != dirtyBindings.end())
    {
        dirtyBindings.erase(it);
    }
}

void UIDataBindingDependenciesManager::SetDirty(void* data)
{
    auto it = dirtyMap.find(data);
    if (it != dirtyMap.end())
    {
        for (int32 id : it->second)
        {
            dirtyBindings[id] = true;
        }
    }
}

bool UIDataBindingDependenciesManager::IsDirty(int32 index) const
{
    auto it = dirtyBindings.find(index);
    return it != dirtyBindings.end() && it->second;
}

void UIDataBindingDependenciesManager::ResetDirties()
{
    for (auto& it : dirtyBindings)
    {
        it.second = false;
    }
}
}
