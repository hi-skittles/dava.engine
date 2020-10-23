#pragma once

#include "Reflection/Reflection.h"
#include "Base/BaseTypes.h"

namespace DAVA
{
class Engine;

/**
Interface for modules. 
Module lifecycle is Ctor->Init->Shutdown->Dtor.
*/
class IModule : public ReflectionBase
{
public:
    /** Create module. Called by ModuleManager for each module in unspecified order. */
    IModule(Engine* engine)
    {
    }

    /** Destroy module. Called by ModuleManager for each module in opposite to creation order. */
    virtual ~IModule() = default;

    /** Init module. Called by ModuleManager for each module after all Modules are created in the same order as modules were created. */
    virtual void Init()
    {
    }

    /** Shutdown module. Called by ModuleManager for each module in opposite to Init order. */
    virtual void Shutdown()
    {
    }
};
}