#pragma once

#include "Reflection/Reflection.h"

namespace DAVA
{
class UIFlowContext;

/**
    Interface which describes UIFlowService.

    UIFlowServices creating and storing in UIFlowContext by activating
    Flow state with configured UIFlowStateComponent.
    While this state still activated all controllers can use instance of this
    service.
    After deactivation the state service will be destroyed too.
*/
class UIFlowService : public ReflectionBase
{
    DAVA_VIRTUAL_REFLECTION(UIFlowService, ReflectionBase);

public:
    /** Default destructor. */
    virtual ~UIFlowService() = default;

    /** Calls then instance of UIFlowService has been activated. */
    virtual void Activate(UIFlowContext* context)
    {
    }

    /** Calls then instance of UIFlowService will be deactivated. */
    virtual void Deactivate(UIFlowContext* context)
    {
    }
};
}
