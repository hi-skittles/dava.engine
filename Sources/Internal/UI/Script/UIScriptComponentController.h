#pragma once

#include "Reflection/Reflection.h"

namespace DAVA
{
class UIContext;
class UIControl;
class UIScriptComponent;

/** Interface of basic UI Component controller. */
class UIScriptComponentController : public ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION(UIScriptComponentController, ReflectionBase);

public:
    virtual ~UIScriptComponentController();

    /** Calls when controller has been created in the begin of activation Flow state. */
    virtual void Init(UIScriptComponent* component)
    {
    }

    /** Calls when controller will be destroyed in the end of deactivation Flow state. */
    virtual void Release(UIScriptComponent* component)
    {
    }

    /** Calls when controller parameters will be changed. */
    virtual void ParametersChanged(UIScriptComponent* component)
    {
    }

    /** Calls each frame if controller is activated. */
    virtual void Process(UIScriptComponent* component, float32 elapsedTime)
    {
    }

    /** Calls when controller recieve Event from UI.
        This method can return true if need discard current event. */
    virtual bool ProcessEvent(UIScriptComponent* component, const FastName& eventName, const Vector<Any>& params = Vector<Any>())
    {
        return false;
    }
};
}