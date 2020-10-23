#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Utils/QtDelayedExecutor.h>

class LaunchModule : public DAVA::ClientModule
{
public:
    ~LaunchModule();

protected:
    void PostInit() override;
    void UnpackHelpDoc();

private:
    class FirstSceneCreator;
    DAVA::QtDelayedExecutor delayedExecutor;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(LaunchModule, DAVA::ClientModule)
    {
        DAVA::ReflectionRegistrator<LaunchModule>::Begin()
        .ConstructorByPointer()
        .End();
    }
};