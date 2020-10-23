#pragma once

#include <REPlatform/Global/CommandLineModule.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Render/RenderBase.h>

namespace DAVA
{
class Camera;
}

class SceneImageDump : public DAVA::CommandLineModule
{
public:
    SceneImageDump(const DAVA::Vector<DAVA::String>& commandLine);

protected:
    bool PostInitInternal() override;
    eFrameResult OnFrameInternal() override;
    void BeforeDestroyedInternal() override;
    void ShowHelpInternal() override;

    DAVA::Camera* FindCamera(DAVA::Entity* rootNode) const;

    DAVA::FilePath sceneFilePath;
    DAVA::FastName cameraName;
    DAVA::int32 width;
    DAVA::int32 height;
    DAVA::eGPUFamily gpuFamily = DAVA::GPU_ORIGIN;
    DAVA::FilePath outputFile;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SceneImageDump, DAVA::CommandLineModule)
    {
        DAVA::ReflectionRegistrator<SceneImageDump>::Begin()[DAVA::M::CommandName("-sceneimagedump")]
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};
