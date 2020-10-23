#pragma once

#include <REPlatform/Global/CommandLineModule.h>

#include <Base/ScopedPtr.h>
#include <FileSystem/FilePath.h>
#include <Reflection/ReflectionRegistrator.h>

namespace DAVA
{
class Scene;
class StaticOcclusionBuildSystem;
}

class StaticOcclusionTool : public DAVA::CommandLineModule
{
public:
    StaticOcclusionTool(const DAVA::Vector<DAVA::String>& commandLine);

protected:
    bool PostInitInternal() override;
    eFrameResult OnFrameInternal() override;
    void BeforeDestroyedInternal() override;
    void ShowHelpInternal() override;

    DAVA::FilePath scenePathname;
    DAVA::ScopedPtr<DAVA::Scene> scene;
    DAVA::StaticOcclusionBuildSystem* staticOcclusionBuildSystem = nullptr;

    enum eAction : DAVA::int32
    {
        ACTION_NONE = -1,
        ACTION_BUILD,
    };
    eAction commandAction = ACTION_NONE;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(StaticOcclusionTool, DAVA::CommandLineModule)
    {
        DAVA::ReflectionRegistrator<StaticOcclusionTool>::Begin()[DAVA::M::CommandName("-staticocclusion")]
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};
