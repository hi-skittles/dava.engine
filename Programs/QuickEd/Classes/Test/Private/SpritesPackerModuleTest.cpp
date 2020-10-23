#include "Modules/SpritesPacker/SpritesPackerModule.h"
#include "Modules/SpritesPacker/SpritesPackerData.h"
#include "Modules/ProjectModule/ProjectModule.h"
#include "Modules/ProjectModule/ProjectData.h"
#include "Modules/DocumentsModule/DocumentsModule.h"
#include "Modules/DocumentsModule/DocumentData.h"

#include "Test/Private/TestHelpers.h"
#include "Test/Private/ProjectSettingsGuard.h"
#include "Test/Private/MockDocumentsModule.h"

#include <QtTools/ReloadSprites/DialogReloadSprites.h>

#include <TArc/Testing/TArcUnitTests.h>
#include <TArc/Testing/TArcTestClass.h>
#include <TArc/Core/ContextManager.h>

#include <FileSystem/FilePath.h>
#include <FileSystem/FileSystem.h>
#include <Render/Image/Image.h>
#include <Render/Image/ImageSystem.h>

DAVA_TARC_TESTCLASS(SpritesPackerModuleTest)
{
    BEGIN_TESTED_MODULES();
    DECLARE_TESTED_MODULE(TestHelpers::ProjectSettingsGuard);
    DECLARE_TESTED_MODULE(TestHelpers::MockDocumentsModule);
    DECLARE_TESTED_MODULE(ProjectModule);
    DECLARE_TESTED_MODULE(SpritesPackerModule);
    END_TESTED_MODULES();

    DAVA_TEST (ReloadSpritesTest)
    {
        using namespace DAVA;

        using namespace TestHelpers;

        CreateProject();

        SpritesPackerData* spritesPackerData = GetAccessor()->GetGlobalContext()->GetData<SpritesPackerData>();
        TEST_VERIFY(spritesPackerData != nullptr);
        SpritesPacker* spritesPacker = spritesPackerData->GetSpritesPacker();
        TEST_VERIFY(spritesPacker != nullptr);

        ProjectData* projectData = GetAccessor()->GetGlobalContext()->GetData<ProjectData>();
        TEST_VERIFY(projectData != nullptr);

        TEST_VERIFY(projectData->GetGfxDirectories().empty() == false);
        const ProjectData::GfxDir& gfxData = projectData->GetGfxDirectories().front();

        FilePath gfxPath = gfxData.directory.absolute;
        FilePath gfxOutPath = projectData->GetConvertedResourceDirectory().absolute + gfxData.directory.relative;

        CreateImage(gfxPath + "img.png");

        QDir gfxDirectory(QString::fromStdString(gfxPath.GetStringValue()));
        QDir gfxOutDirectory(QString::fromStdString(gfxOutPath.GetStringValue()));

        spritesPacker->AddTask(gfxDirectory, gfxOutDirectory);
        spritesPacker->ReloadSprites(true, true, DAVA::eGPUFamily::GPU_ORIGIN, DAVA::TextureConverter::eConvertQuality::ECQ_DEFAULT);

        FileSystem* fileSystem = GetAccessor()->GetEngineContext()->fileSystem;
        TEST_VERIFY(fileSystem->Exists(gfxOutPath + "img.txt") == true);
        TEST_VERIFY(fileSystem->Exists(gfxOutPath + "texture0.tex") == true);
        TEST_VERIFY(fileSystem->Exists(gfxOutPath + "texture0.png") == true);
    }

    void CreateProject()
    {
        DAVA::FilePath projectPath = TestHelpers::GetTestPath() + "ReloadSpritesModuleTest";

        TestHelpers::CreateProjectFolder(projectPath);

        DAVA::String projectPathStr = projectPath.GetAbsolutePathname();
        InvokeOperation(ProjectModuleTesting::CreateProjectOperation.ID, QString::fromStdString(projectPathStr));
    }

    void CreateImage(const DAVA::FilePath& imagePath)
    {
        using namespace DAVA;

        ScopedPtr<Image> image(Image::Create(256, 256, FORMAT_RGBA8888));
        image->MakePink(true);
        ImageSystem::Save(imagePath, image);
    }
};
