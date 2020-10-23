#if defined(__DAVAENGINE_BEAST__)

#include "Classes/Beast/Private/BeastRunner.h"
#include "Classes/Beast/Private/LightmapsPacker.h"

#include "Utils/SceneUtils/SceneUtils.h"

#include <Beast/SceneParser.h>

#include <Time/SystemTimer.h>
#include <Scene3D/Scene.h>
#include <FileSystem/FileSystem.h>

//Beast
BeastRunner::BeastRunner(DAVA::Scene* scene, const DAVA::FilePath& scenePath_, const DAVA::FilePath& outputPath_, Beast::eBeastMode mode, std::unique_ptr<DAVA::TArc::WaitHandle> waitHandle_)
    : beastManager(new Beast::BeastManager())
    , waitHandle(std::move(waitHandle_))
    , workingScene(scene)
    , scenePath(scenePath_)
    , outputPath(outputPath_)
    , beastMode(mode)
{
    outputPath.MakeDirectoryPathname();
    beastManager->SetMode(beastMode);
}

void BeastRunner::RunUIMode()
{
    Start();

    while (Process() == false)
    {
        if (cancelledManually)
        {
            beastManager->Cancel();
            break;
        }
        else if (waitHandle)
        {
            waitHandle->SetProgressValue(beastManager->GetCurTaskProcess());
            waitHandle->SetMessage(beastManager->GetCurTaskName().c_str());
            cancelledManually |= waitHandle->WasCanceled();
        }

        Sleep(15);
    }

    Finish(cancelledManually || beastManager->WasCancelled());

    if (waitHandle)
    {
        waitHandle.reset();
    }
}

void BeastRunner::Start()
{
    startTime = DAVA::SystemTimer::GetMs();

    DAVA::FilePath path = GetLightmapDirectoryPath();
    if (beastMode == Beast::eBeastMode::MODE_LIGHTMAPS)
    {
        DAVA::FileSystem::Instance()->CreateDirectory(path, false);
        DAVA::FileSystem::Instance()->CreateDirectory(outputPath, true);
    }

    beastManager->SetLightmapsDirectory(path.GetAbsolutePathname());
    beastManager->Run(workingScene);
}

bool BeastRunner::Process()
{
    beastManager->Update();
    return beastManager->IsJobDone();
}

void BeastRunner::Finish(bool canceled)
{
    if (!canceled && beastMode == Beast::eBeastMode::MODE_LIGHTMAPS)
    {
        PackLightmaps();
    }

    DAVA::FileSystem::Instance()->DeleteDirectory(Beast::SceneParser::GetTemporaryFolder());
    if (beastMode == Beast::eBeastMode::MODE_LIGHTMAPS)
    {
        DAVA::FileSystem::Instance()->DeleteDirectory(GetLightmapDirectoryPath());
    }
}

void BeastRunner::PackLightmaps()
{
    DAVA::FilePath inputDir = GetLightmapDirectoryPath();
    DAVA::FilePath outputDir = outputPath;

    DAVA::FileSystem::Instance()->MoveFile(inputDir + "landscape.png", scenePath.GetDirectory() + "temp_landscape_lightmap.png", true);

    LightmapsPacker packer;
    packer.SetInputDir(inputDir);

    packer.SetOutputDir(outputDir);
    packer.PackLightmaps(DAVA::GPU_ORIGIN);
    packer.CreateDescriptors();
    packer.ParseSpriteDescriptors();

    beastManager->UpdateAtlas(packer.GetAtlasingData());

    DAVA::FileSystem::Instance()->MoveFile(scenePath.GetDirectory() + "temp_landscape_lightmap.png", outputDir + "landscape.png", true);
    DAVA::FileSystem::Instance()->DeleteDirectory(scenePath.GetDirectory() + "$process/");
}

DAVA::FilePath BeastRunner::GetLightmapDirectoryPath() const
{
    return scenePath + "_beast/";
}


#endif //#if defined (__DAVAENGINE_BEAST__)
