#pragma once

#if defined(__DAVAENGINE_BEAST__)

#include <TArc/WindowSubSystem/UI.h>

#include <Beast/BeastConstants.h>
#include <Beast/BeastManager.h>

#include <FileSystem/FilePath.h>

#include <memory>

namespace DAVA
{
class Scene;

namespace TArc
{
class WaitHandle;
}
}

namespace Beast
{
class BeastManager;
}

class BeastRunner final
{
public:
    BeastRunner(DAVA::Scene* scene, const DAVA::FilePath& scenePath, const DAVA::FilePath& outputPath, Beast::eBeastMode mode, std::unique_ptr<DAVA::TArc::WaitHandle> waitHandle);

    void RunUIMode();

    void Start();
    bool Process();
    void Finish(bool canceled);

private:
    void PackLightmaps();
    DAVA::FilePath GetLightmapDirectoryPath() const;

private:
    std::unique_ptr<Beast::BeastManager> beastManager;

    std::unique_ptr<DAVA::TArc::WaitHandle> waitHandle;

    DAVA::Scene* workingScene = nullptr;
    const DAVA::FilePath scenePath;
    DAVA::FilePath outputPath;
    DAVA::uint64 startTime = 0;
    Beast::eBeastMode beastMode = Beast::eBeastMode::MODE_LIGHTMAPS;

    bool cancelledManually = false;
};

#endif //#if defined (__DAVAENGINE_BEAST__)
