#include "Classes/CommandLine/StaticOcclusionTool.h"

#include <REPlatform/CommandLine/OptionName.h>
#include <REPlatform/CommandLine/SceneConsoleHelper.h>
#include <REPlatform/Scene/Utils/Utils.h>

#include <TArc/Utils/ModuleCollection.h>

#include <Entity/ComponentUtils.h>
#include <Logger/Logger.h>
#include <Math/Color.h>
#include <Render/Renderer.h>
#include <Render/RenderHelper.h>
#include <Render/RHI/rhi_Public.h>
#include <Scene3D/Scene.h>
#include <Scene3D/Systems/RenderUpdateSystem.h>
#include <Scene3D/Systems/StaticOcclusionBuildSystem.h>
#include <Scene3D/Components/StaticOcclusionComponent.h>
#include <Scene3D/Components/TransformComponent.h>

StaticOcclusionTool::StaticOcclusionTool(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "-staticocclusion")
    , scene(nullptr)
{
    using namespace DAVA;

    options.AddOption(OptionName::Build, VariantType(false), "Enables build of static occlusion");
    options.AddOption(OptionName::ProcessFile, VariantType(String("")), "Full pathname to scene file *.sc2");
    options.AddOption(OptionName::QualityConfig, VariantType(String("")), "Full path for quality.yaml file");
}

bool StaticOcclusionTool::PostInitInternal()
{
    using namespace DAVA;

    if (options.GetOption(OptionName::Build).AsBool())
    {
        commandAction = ACTION_BUILD;
    }
    else
    {
        Logger::Error("Wrong action was selected");
        return false;
    }

    scenePathname = options.GetOption(OptionName::ProcessFile).AsString();
    if (scenePathname.IsEmpty())
    {
        Logger::Error("Filename was not set");
        return false;
    }

    bool qualityInitialized = SceneConsoleHelper::InitializeQualitySystem(options, scenePathname);
    if (!qualityInitialized)
    {
        Logger::Error("Cannot create path to quality.yaml from %s", scenePathname.GetAbsolutePathname().c_str());
        return false;
    }

    if (commandAction == ACTION_BUILD)
    {
        scene.reset(new Scene());
        staticOcclusionBuildSystem = new StaticOcclusionBuildSystem(scene);
        scene->AddSystem(staticOcclusionBuildSystem, ComponentUtils::MakeMask<StaticOcclusionComponent>() | ComponentUtils::MakeMask<TransformComponent>(), Scene::SCENE_SYSTEM_REQUIRE_PROCESS, scene->renderUpdateSystem);

        if (scene->LoadScene(scenePathname) != SceneFileV2::eError::ERROR_NO_ERROR)
        {
            Logger::Error("Cannot load scene %s", scenePathname.GetAbsolutePathname().c_str());

            staticOcclusionBuildSystem = nullptr;
            scene.reset();
            return false;
        }

        ScopedPtr<Camera> lodSystemDummyCamera(new Camera());
        lodSystemDummyCamera->SetUp(Vector3(0.0f, 0.0f, 1.0f));
        lodSystemDummyCamera->SetPosition(Vector3(0.0f, 0.0f, 0.0f));
        lodSystemDummyCamera->SetTarget(Vector3(0.0f, 0.1f, 0.0f));
        lodSystemDummyCamera->SetupPerspective(90.f, 320.0f / 480.0f, 1.f, 5000.f);
        lodSystemDummyCamera->SetAspect(1.0f);

        scene->SetCurrentCamera(lodSystemDummyCamera);

        scene->Update(0.1f); // we need to call update to initialize (at least) QuadTree.
        staticOcclusionBuildSystem->Build();
        SceneConsoleHelper::FlushRHI();
    }

    return true;
}

DAVA::ConsoleModule::eFrameResult StaticOcclusionTool::OnFrameInternal()
{
    if (commandAction == ACTION_BUILD)
    {
        if (staticOcclusionBuildSystem != nullptr && staticOcclusionBuildSystem->IsInBuild())
        {
            const rhi::HTexture nullTexture;
            const rhi::Viewport nullViewport(0, 0, 1, 1);

            static DAVA::Color defShadowColor(1.f, 0.f, 0.f, 1.f);
            static DAVA::Color defWaterClearColor(0.f, 0.f, 0.f, 0.f);
            static DAVA::float32 sceneGlobalTime = 0.f;

            const DAVA::float32* shadowDataPtr = defShadowColor.color;
            const DAVA::float32* waterDataPtr = defWaterClearColor.color;

            DAVA::Renderer::GetDynamicBindings().SetDynamicParam(DAVA::DynamicBindings::PARAM_SHADOW_COLOR, shadowDataPtr, reinterpret_cast<DAVA::pointer_size>(shadowDataPtr));
            DAVA::Renderer::GetDynamicBindings().SetDynamicParam(DAVA::DynamicBindings::PARAM_WATER_CLEAR_COLOR, waterDataPtr, reinterpret_cast<DAVA::pointer_size>(waterDataPtr));
            DAVA::Renderer::GetDynamicBindings().SetDynamicParam(DAVA::DynamicBindings::PARAM_GLOBAL_TIME, &sceneGlobalTime, reinterpret_cast<DAVA::pointer_size>(&sceneGlobalTime));

            DAVA::Renderer::BeginFrame();
            DAVA::RenderHelper::CreateClearPass(nullTexture, nullTexture, 0, DAVA::Color::Clear, nullViewport);
            scene->Update(0.1f);
            DAVA::Renderer::EndFrame();

            return DAVA::ConsoleModule::eFrameResult::CONTINUE;
        }
    }

    return DAVA::ConsoleModule::eFrameResult::FINISHED;
}

void StaticOcclusionTool::BeforeDestroyedInternal()
{
    if (scene)
    {
        scene->SetCurrentCamera(nullptr);
        scene->SaveScene(scenePathname, true);
        staticOcclusionBuildSystem = nullptr;
        scene.reset();
    }

    DAVA::SceneConsoleHelper::FlushRHI();
}

void StaticOcclusionTool::ShowHelpInternal()
{
    CommandLineModule::ShowHelpInternal();

    DAVA::Logger::Info("Examples:");
    DAVA::Logger::Info("\t-staticocclusion -build -processfile /Users/Test/DataSource/3d/Maps/scene.sc2");
}

DECL_TARC_MODULE(StaticOcclusionTool);
