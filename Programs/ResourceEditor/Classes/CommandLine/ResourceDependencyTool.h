#pragma once

#include <REPlatform/Global/CommandLineModule.h>
#include <REPlatform/Scene/Utils/SceneDumper.h>

#include <Base/BaseTypes.h>
#include <Reflection/ReflectionRegistrator.h>

class ResourceDependencyTool : public DAVA::CommandLineModule
{
public:
    ResourceDependencyTool(const DAVA::Vector<DAVA::String>& commandLine);

private:
    bool PostInitInternal() override;
    eFrameResult OnFrameInternal() override;
    void BeforeDestroyedInternal() override;
    void ShowHelpInternal() override;

    DAVA::Vector<DAVA::FilePath> resources;
    DAVA::FilePath outFile;

    DAVA::int32 mode = 0;

    enum class Action
    {
        PrintVersion,
        PrintDependencies
    };

    Action requiredAction = Action::PrintDependencies;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ResourceDependencyTool, DAVA::CommandLineModule)
    {
        DAVA::ReflectionRegistrator<ResourceDependencyTool>::Begin()[DAVA::M::CommandName("resourceDependency")]
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};
