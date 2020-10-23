#pragma once

#include "Reflection/Reflection.h"

namespace DAVA
{
class UIFlowContext;
class UIControl;

/** Interface of basic UI Flow controller. */
class UIFlowController : public ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION(UIFlowController, ReflectionBase);

public:
    /** Virtual destructor. */
    virtual ~UIFlowController();

    /** Calls when controller has been created in the begin of activation Flow state. */
    virtual void Init(UIFlowContext* context)
    {
    }

    /** Calls when controller will be destroyed in the end of deactivation Flow state. */
    virtual void Release(UIFlowContext* context)
    {
    }

    /** Calls when controller should be load some internal resources in the middle of activateion Flow state.
        This method can execute not in main thread. */
    virtual void LoadResources(UIFlowContext* context, UIControl* view)
    {
    }

    /** Calls when controller should be unload some internal resources in the middle of deactivateion Flow state.
        This method can execute not in main thread. */
    virtual void UnloadResources(UIFlowContext* context, UIControl* view)
    {
    }

    /** Calls when controller has been activated in the end of activation Flow state. */
    virtual void Activate(UIFlowContext* context, UIControl* view)
    {
    }

    /** Calls when controller has been activated in the begin of deactivation Flow state. */
    virtual void Deactivate(UIFlowContext* context, UIControl* view)
    {
    }

    /** Calls each frame if controller is activated. */
    virtual void Process(float32 elapsedTime)
    {
    }

    /** Calls when controller recieve Event from UI Flow.
        This method can return true if need discard current event. */
    virtual bool ProcessEvent(const FastName& eventName, const Vector<Any>& params = Vector<Any>())
    {
        return false;
    }
};
}