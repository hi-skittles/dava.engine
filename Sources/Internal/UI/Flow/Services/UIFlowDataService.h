#pragma once

#include "Reflection/Reflection.h"
#include "UI/Flow/UIFlowService.h"

namespace DAVA
{
class UIFlowContext;

class UIFlowDataService : public UIFlowService
{
    DAVA_VIRTUAL_REFLECTION(UIFlowDataService, UIFlowService);

public:
    void SetDataDirty(const Reflection& ref);
};
}