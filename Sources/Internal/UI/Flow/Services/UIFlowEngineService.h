#pragma once

#include "Reflection/Reflection.h"
#include "UI/Flow/UIFlowService.h"

namespace DAVA
{
class Engine;
class EngineContext;

class UIFlowEngineService : public UIFlowService
{
    DAVA_VIRTUAL_REFLECTION(UIFlowEngineService, UIFlowService);

public:
    const Engine* GetEngine() const;
    const EngineContext* GetEngineContext() const;
};
}