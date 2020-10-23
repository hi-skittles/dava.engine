#include "Classes/CommandLine/SceneImageDump.h"

#include <REPlatform/CommandLine/CommandLineModuleTestUtils.h>

#include <TArc/Testing/ConsoleModuleTestExecution.h>
#include <TArc/Testing/TArcUnitTests.h>

#include <Base/ScopedPtr.h>
#include <FileSystem/FileSystem.h>
#include <Render/Highlevel/Landscape.h>
#include <Render/Highlevel/RenderObject.h>
#include <Render/Highlevel/Vegetation/VegetationRenderObject.h>
#include <Render/Image/ImageSystem.h>
#include <Scene3D/Entity.h>
#include <Scene3D/Scene.h>

#include <memory>

namespace SIDTestDetail
{
const DAVA::String projectStr = "~doc:/Test/SceneImageDump/";
const DAVA::String scenePathnameStr = projectStr + "DataSource/3d/Scene/testScene.sc2";
const DAVA::String outPathnameStr = projectStr + "screenshot.png";
}

DAVA_TARC_TESTCLASS(SceneImageDumpTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(TArc)
    DECLARE_COVERED_FILES("SceneImageDump.cpp")
    END_FILES_COVERED_BY_TESTS();

    DAVA_TEST (ImageDump)
    {
        using namespace DAVA;

        std::unique_ptr<CommandLineModuleTestUtils::TextureLoadingGuard> guard = CommandLineModuleTestUtils::CreateTextureGuard({ eGPUFamily::GPU_ORIGIN });
        CommandLineModuleTestUtils::CreateProjectInfrastructure(SIDTestDetail::projectStr);
        CommandLineModuleTestUtils::SceneBuilder::CreateFullScene(SIDTestDetail::scenePathnameStr, SIDTestDetail::projectStr);

        Vector<String> cmdLine =
        {
          "ResourceEditor",
          "-sceneimagedump",
          "-processfile",
          FilePath(SIDTestDetail::scenePathnameStr).GetAbsolutePathname(),
          "-camera",
          "camera",
          "-width",
          "64",
          "-height",
          "64",
          "-gpu",
          "origin",
          "-outfile",
          FilePath(SIDTestDetail::outPathnameStr).GetAbsolutePathname()
        };

        std::unique_ptr<CommandLineModule> tool = std::make_unique<SceneImageDump>(cmdLine);
        DAVA::ConsoleModuleTestExecution::ExecuteModule(tool.get());

        ImageInfo info = ImageSystem::GetImageInfo(SIDTestDetail::outPathnameStr);
        TEST_VERIFY(info.IsEmpty() == false);
        TEST_VERIFY(info.width == 64);
        TEST_VERIFY(info.height == 64);

        CommandLineModuleTestUtils::ClearTestFolder(SIDTestDetail::projectStr);
    }
};
