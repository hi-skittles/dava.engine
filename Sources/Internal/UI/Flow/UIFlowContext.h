#pragma once

#include "Base/BaseTypes.h"
#include "Base/FastName.h"
#include "Base/RefPtr.h"
#include "Base/UnordererMap.h"
#include "FileSystem/KeyedArchive.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class EngineContext;
class UIFlowService;

/** Contains information about current available UI services and shared data. */
class UIFlowContext final : public ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION(UIFlowContext, ReflectionBase);

public:
    /** Default constructor. */
    UIFlowContext();

    /** Return pointer to KeyedArchive with shared UI data. */
    KeyedArchive* GetData() const;
    /** Return pointer to activated UI service by specified alias name. */
    UIFlowService* GetService(const FastName& name) const;
    /** Return casted to specified type pointer to activated UI service by specified alias name. */
    template <class T>
    T* GetService(const FastName& name) const
    {
        return static_cast<T*>(GetService(name));
    }

    /** Initialize and activate UI service with specified alias and reflected name. */
    void InitServiceByType(const FastName& name, const String& typeName);
    /** Deactivate and release UI service by specified alias name. */
    void ReleaseService(const FastName& name);

private:
    struct ServiceLink
    {
        std::shared_ptr<UIFlowService> service;
        int32 initCount = 0;
    };

    UnorderedMap<FastName, ServiceLink> services;
    RefPtr<KeyedArchive> data;
};
}
