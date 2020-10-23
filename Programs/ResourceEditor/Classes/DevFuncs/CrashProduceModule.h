#pragma once

#include "TArc/Core/ClientModule.h"
#include "TArc/Utils/QtConnections.h"

#include "Reflection/Reflection.h"

class CrashProduceModule : public DAVA::ClientModule
{
protected:
    void PostInit() override;

private:
    DAVA::QtConnections connections;

    DAVA_VIRTUAL_REFLECTION(CrashProduceModule, DAVA::ClientModule);
};