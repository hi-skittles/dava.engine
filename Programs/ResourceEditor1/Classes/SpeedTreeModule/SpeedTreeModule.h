#pragma once

#if defined(__DAVAENGINE_SPEEDTREE__)


#include "TArc/Core/ClientModule.h"
#include "TArc/Core/FieldBinder.h"
#include "TArc/Utils/QtConnections.h"

#include "Base/Any.h"
#include "Reflection/Reflection.h"

#include <memory>

class SpeedTreeModule : public DAVA::TArc::ClientModule
{
protected:
    void PostInit() override;

private:
    void OnImportSpeedTree();

    DAVA::TArc::QtConnections connections;
    DAVA_VIRTUAL_REFLECTION(SpeedTreeModule, DAVA::TArc::ClientModule);
};

#endif //#if defined (__DAVAENGINE_SPEEDTREE__)
