#pragma once

#include "TArc/Core/ClientModule.h"
#include "TArc/Utils/QtConnections.h"
#include "Reflection/ReflectionRegistrator.h"

class MitsubaExporter : public DAVA::ClientModule
{
protected:
    void PostInit() override;
    void Export();

private:
    DAVA::QtConnections connections;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(MitsubaExporter, DAVA::ClientModule)
    {
        DAVA::ReflectionRegistrator<MitsubaExporter>::Begin()
        .ConstructorByPointer()
        .End();
    }
};
