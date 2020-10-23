#include "CommandLine/SceneImageDump.h"

#include "CommandLine/Private/OptionName.h"
#include "CommandLine/Private/SceneConsoleHelper.h"
#include "Scene/SceneImageGraber.h"

#include "TArc/Utils/ModuleCollection.h"

#include "Logger/Logger.h"
#include "Base/ScopedPtr.h"
#include "Base/BaseTypes.h"
#include "Render/GPUFamilyDescriptor.h"
#include "Render/Texture.h"
#include "Render/RHI/rhi_Public.h"
#include "Render/Renderer.h"
#include "Render/RenderHelper.h"
#include "Render/GPUFamilyDescriptor.h"
#include "Render/Texture.h"
#include "Scene3D/Components/ComponentHelpers.h"
#include "Scene3D/Scene.h"

SceneImageDump::SceneImageDump(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "-sceneimagedump")
{
    using namespace DAVA;
    options.AddOption(OptionName::ProcessFile, VariantType(String("")), "Full pathname to scene file *.sc2");
    options.AddOption(OptionName::Camera, VariantType(String("")), "Camera name for draw");
    options.AddOption(OptionName::Width, VariantType(int32(0)), "Result image width");
    options.AddOption(OptionName::Height, VariantType(int32(0)), "Result image height");
    options.AddOption(OptionName::GPU, VariantType(String("origin")), "GPU family: PowerVR_iOS, PowerVR_Android, tegra, mali, adreno, origin, dx11");
    options.AddOption(OptionName::OutFile, VariantType(String("")), "Path to output file");
    options.AddOption(OptionName::QualityConfig, VariantType(String("")), "Full path for quality.yaml file");
}

bool SceneImageDump::PostInitInternal()
{
    sceneFilePath = options.GetOption(OptionName::ProcessFile).AsString();
    if (sceneFilePath.IsEmpty() || !sceneFilePath.Exists())
    {
        DAVA::Logger::Error("Path to scene is incorrect");
        return false;
    }

    cameraName = DAVA::FastName(options.GetOption(OptionName::Camera).AsString());
    if (!cameraName.IsValid())
    {
        DAVA::Logger::Error("Camera name is not specified");
        return false;
    }

    width = options.GetOption(OptionName::Width).AsInt32();
    height = options.GetOption(OptionName::Height).AsInt32();
    if (width <= 0 || height <= 0)
    {
        DAVA::Logger::Error("Incorrect size for output image");
        return false;
    }

    DAVA::String gpuName = options.GetOption(OptionName::GPU).AsString();
    gpuFamily = DAVA::GPUFamilyDescriptor::GetGPUByName(gpuName);
    outputFile = options.GetOption(OptionName::OutFile).AsString();

    if (outputFile.IsEmpty())
    {
        outputFile = sceneFilePath.GetDirectory();
    }

    bool qualityInitialized = SceneConsoleHelper::InitializeQualitySystem(options, sceneFilePath);
    if (!qualityInitialized)
    {
        DAVA::Logger::Error("Cannot create path to quality.yaml from %s", sceneFilePath.GetAbsolutePathname().c_str());
        return false;
    }

    return true;
}

DAVA::TArc::ConsoleModule::eFrameResult SceneImageDump::OnFrameInternal()
{
    using namespace DAVA;
    const rhi::HTexture nullTexture;
    const rhi::Viewport nullViewport(0, 0, 1, 1);

    Vector<eGPUFamily> textureLoadingOrder = Texture::GetGPULoadingOrder();
    Texture::SetGPULoadingOrder({ gpuFamily });

    ScopedPtr<Scene> scene(new Scene());
    if (scene->LoadScene(sceneFilePath) == SceneFileV2::eError::ERROR_NO_ERROR)
    {
        Camera* camera = FindCamera(scene);
        if (camera == nullptr)
        {
            Logger::Error("Camera %s not found in scene %s", cameraName.c_str(), sceneFilePath.GetAbsolutePathname().c_str());
            return TArc::ConsoleModule::eFrameResult::FINISHED;
        }

        bool grabFinished = false;

        SceneImageGrabber::Params params;
        params.scene = scene;
        params.cameraToGrab = camera;
        params.imageSize = Size2i(width, height);
        params.outputFile = FilePath(outputFile);
        params.processInDAVAFrame = false;
        params.readyCallback = [&grabFinished]()
        {
            grabFinished = true;
        };

        scene->Update(0.1f);
        Renderer::BeginFrame();
        RenderHelper::CreateClearPass(nullTexture, nullTexture, 0, Color::Clear, nullViewport);
        SceneImageGrabber::GrabImage(params);
        Renderer::EndFrame();

        while (grabFinished == false)
        {
            Renderer::BeginFrame();
            RenderHelper::CreateClearPass(nullTexture, nullTexture, 0, Color::Clear, nullViewport);
            scene->Update(0.1f);
            Renderer::EndFrame();
        }
    }

    Texture::SetGPULoadingOrder(textureLoadingOrder);

    return TArc::ConsoleModule::eFrameResult::FINISHED;
}

DAVA::Camera* SceneImageDump::FindCamera(DAVA::Entity* rootNode) const
{
    for (DAVA::Entity* entity : rootNode->children)
    {
        if (entity->GetName() == cameraName)
        {
            DAVA::Camera* camera = DAVA::GetCamera(entity);
            if (camera != nullptr)
            {
                return camera;
            }
        }
    }

    return nullptr;
}

void SceneImageDump::BeforeDestroyedInternal()
{
    SceneConsoleHelper::FlushRHI();
}

void SceneImageDump::ShowHelpInternal()
{
    CommandLineModule::ShowHelpInternal();

    DAVA::Logger::Info("Examples:");
    DAVA::Logger::Info("\t-sceneimagedump -processfile /Users/SmokeTest/DataSource/3d/Maps/11-grass/test_scene.sc2 -camera TestCamera -width 1024 -height 1024 -gpu adreno -outfile /Users/screenshot.png");
}

DECL_CONSOLE_MODULE(SceneImageDump, "-sceneimagedump");
