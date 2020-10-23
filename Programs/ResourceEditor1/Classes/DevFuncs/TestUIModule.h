#pragma once

#include "TArc/Core/ClientModule.h"
#include "TArc/Utils/QtConnections.h"

#include "Reflection/Reflection.h"

class TestUIModule : public DAVA::TArc::ClientModule
{
protected:
    void PostInit() override;

private:
    void ShowDialog();

    DAVA::TArc::QtConnections connections;

    DAVA_VIRTUAL_REFLECTION(TestUIModule, DAVA::TArc::ClientModule);
};
