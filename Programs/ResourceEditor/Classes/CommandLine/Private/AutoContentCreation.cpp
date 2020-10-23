#include "Classes/CommandLine/AutoContentCreation.h"

#include <REPlatform/CommandLine/OptionName.h>
#include <REPlatform/CommandLine/SceneConsoleHelper.h>
#include <REPlatform/Scene/Utils/SceneSaver.h>

#include <TArc/Utils/ModuleCollection.h>

#include <Base/RefPtr.h>
#include <Base/ScopedPtr.h>
#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <FileSystem/File.h>
#include <FileSystem/FileList.h>
#include <FileSystem/FileSystem.h>
#include <Logger/Logger.h>
#include <Scene3D/Scene.h>
#include <Scene3D/SceneFileV2.h>
#include <Utils/StringFormat.h>
#include <Math/Transform.h>
#include <Scene3D/Components/TransformComponent.h>

DuplicateObjectTool::DuplicateObjectTool(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "-duplicate")
{
    using namespace DAVA;

    options.AddOption(OptionName::File, VariantType(String("")), "Filename from DataSource/3d/ for duplicating");
    options.AddOption(OptionName::OutDir, VariantType(String("")), "Full path to folder to write duplicates");
    options.AddOption(OptionName::Count, VariantType(1), "Count of duplicated objects");
}

bool DuplicateObjectTool::PostInitInternal()
{
    using namespace DAVA;

    filePath = options.GetOption(OptionName::File).AsString();
    if (filePath.IsEmpty())
    {
        DAVA::Logger::Error("[DuplicateObjectTool] File for duplicating was not specified");
        return false;
    }

    outDirPath = options.GetOption(OptionName::OutDir).AsString();
    if (outDirPath.IsEmpty())
    {
        DAVA::Logger::Error("[DuplicateObjectTool] Output folder was not specified");
        return false;
    }
    outDirPath.MakeDirectoryPathname();
    const DAVA::EngineContext* engineCtx = DAVA::GetEngineContext();
    if (engineCtx->fileSystem->Exists(outDirPath))
    {
        if (engineCtx->fileSystem->DeleteDirectory(outDirPath, true) == false)
        {
            DAVA::Logger::Error("[DuplicateObjectTool] Can't clear output folder");
            return false;
        }
    }

    if (engineCtx->fileSystem->CreateDirectory(outDirPath, true) == false)
    {
        DAVA::Logger::Error("[DuplicateObjectTool] Can't create output folder");
        return false;
    }

    count = options.GetOption(OptionName::Count).AsInt32();
    return true;
}

DAVA::ConsoleModule::eFrameResult DuplicateObjectTool::OnFrameInternal()
{
    DAVA::RefPtr<DAVA::Scene> scene(new DAVA::Scene());
    DAVA::SceneFileV2::eError result = scene->LoadScene(filePath);
    if (result != DAVA::SceneFileV2::ERROR_NO_ERROR)
    {
        DAVA::Logger::Error("[DuplicateObjectTool] Can't open scene %s. Error code: %d", filePath.GetAbsolutePathname().c_str(), result);
        return DAVA::ConsoleModule::eFrameResult::FINISHED;
    }

    for (DAVA::uint32 i = 0; i < count; ++i)
    {
        DAVA::FilePath duplicatePath = outDirPath + DAVA::Format("/duplicate_%u/", i);

        DAVA::SceneSaver sceneSaver;
        sceneSaver.SetInFolder(filePath.GetDirectory());
        sceneSaver.SetOutFolder(duplicatePath);
        sceneSaver.EnableCopyConverted(true);

        sceneSaver.SaveScene(scene.Get(), filePath);
    }

    return DAVA::ConsoleModule::eFrameResult::FINISHED;
}

void DuplicateObjectTool::BeforeDestroyedInternal()
{
    DAVA::SceneConsoleHelper::FlushRHI();
}

void DuplicateObjectTool::ShowHelpInternal()
{
    CommandLineModule::ShowHelpInternal();

    DAVA::Logger::Info("Examples:");
    DAVA::Logger::Info("\t-duplicate -file /Users/SmokeTest/DataSource/3d/Map/Box.sc2 -outdir /Users/SmokeTest/DataSource/3d/Boxes/ -count 140");
}

RandomPlaceHingedEquipment::RandomPlaceHingedEquipment(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "-placehingedequip")
{
    using namespace DAVA;

    options.AddOption(OptionName::InDir, VariantType(String("")), "Path for Project/DataSource/3d/ folder");
    options.AddOption(OptionName::ProcessFileList, VariantType(String("")), "File that contains list of object to place hinged equip on");
    options.AddOption(OptionName::ProcessDir, VariantType(String("")), "Library of hinged equip path. Each hinged equip object in separate folder");
}

bool RandomPlaceHingedEquipment::PostInitInternal()
{
    using namespace DAVA;

    const DAVA::EngineContext* engineCtx = DAVA::GetEngineContext();

    projectRootFolder = options.GetOption(OptionName::InDir).AsString();
    if (projectRootFolder.IsEmpty())
    {
        DAVA::Logger::Error("[RandomPlaceHingedEquipment] Project folder was not specified");
        return false;
    }

    if (engineCtx->fileSystem->Exists(projectRootFolder) == false)
    {
        DAVA::Logger::Error("[RandomPlaceHingedEquipment] Project folder does not exist");
        return false;
    }
    projectRootFolder.MakeDirectoryPathname();

    // Accumulate scenes
    DAVA::FilePath processFileList(options.GetOption(OptionName::ProcessFileList).AsString());
    if (processFileList.IsEmpty())
    {
        DAVA::Logger::Error("[RandomPlaceHingedEquipment] File with list of object to place hinged equip was not specified");
        return false;
    }

    if (engineCtx->fileSystem->Exists(processFileList) == false)
    {
        DAVA::Logger::Error("[RandomPlaceHingedEquipment] File with list of object to place hinged equip does not exist");
        return false;
    }

    DAVA::ScopedPtr<DAVA::File> scenesFile(DAVA::File::Create(processFileList, DAVA::File::OPEN | DAVA::File::READ));
    if (!scenesFile)
    {
        DAVA::Logger::Error("[RandomPlaceHingedEquipment] cannot open file with links %s", processFileList.GetAbsolutePathname().c_str());
        return false;
    }

    do
    {
        DAVA::String scenePath = scenesFile->ReadLine();
        if (scenePath.empty())
        {
            DAVA::Logger::Warning("[RandomPlaceHingedEquipment] found empty string in file %s", processFileList.GetAbsolutePathname().c_str());
            continue;
        }

        DAVA::FilePath fullScenePath = projectRootFolder + scenePath;
        if (engineCtx->fileSystem->Exists(fullScenePath) == false)
        {
            DAVA::Logger::Error("[RandomPlaceHingedEquipment] Scene %s does not exist", fullScenePath.GetAbsolutePathname().c_str());
            continue;
        }
        scenesPath.emplace_back(fullScenePath);
    } while (!scenesFile->IsEof());

    hindedEquipLibrary = options.GetOption(OptionName::ProcessDir).AsString();
    if (hindedEquipLibrary.IsEmpty())
    {
        DAVA::Logger::Error("[RandomPlaceHingedEquipment] Hinged equip library was not specified");
        return false;
    }
    hindedEquipLibrary.MakeDirectoryPathname();

    return true;
}

DAVA::ConsoleModule::eFrameResult RandomPlaceHingedEquipment::OnFrameInternal()
{
    DAVA::Vector<DAVA::FilePath> hingedEquipPathes;
    DAVA::ScopedPtr<DAVA::FileList> hingedEquipFolders(new DAVA::FileList(hindedEquipLibrary, false));
    for (DAVA::uint32 dirIndex = 0; dirIndex < hingedEquipFolders->GetCount(); ++dirIndex)
    {
        if (hingedEquipFolders->IsNavigationDirectory(dirIndex))
        {
            continue;
        }

        DAVA::FilePath filePath = hingedEquipFolders->GetPathname(dirIndex);
        if (filePath.IsDirectoryPathname())
        {
            DAVA::ScopedPtr<DAVA::FileList> hingedEquipObject(new DAVA::FileList(filePath, false));
            for (DAVA::uint32 fileIndex = 0; fileIndex < hingedEquipObject->GetCount(); ++fileIndex)
            {
                DAVA::FilePath objectPath = hingedEquipObject->GetPathname(fileIndex);
                if (objectPath.IsEqualToExtension(".sc2") == true)
                {
                    hingedEquipPathes.push_back(objectPath);
                }
            }
        }
    }

    DAVA::uint32 hingedCountOnObject = static_cast<DAVA::uint32>((hingedEquipPathes.size() + scenesPath.size() - 1) / scenesPath.size());
    for (const DAVA::FilePath& scenePath : scenesPath)
    {
        DAVA::ScopedPtr<DAVA::Scene> scene(new DAVA::Scene());
        DAVA::SceneFileV2::eError loadResult = scene->LoadScene(scenePath);
        if (loadResult != DAVA::SceneFileV2::ERROR_NO_ERROR)
        {
            DAVA::Logger::Error("[RandomPlaceHingedEquipment] Can't load scene %s", scenePath.GetAbsolutePathname().c_str());
            continue;
        }

        if (scene->GetChildrenCount() != 1)
        {
            DAVA::Logger::Error("[RandomPlaceHingedEquipment] Scene %s doesn't meet requirements. Scene should has only one root entity", scenePath.GetAbsolutePathname().c_str());
            continue;
        }

        DAVA::Entity* rootEntity = scene->GetChild(0);
        DAVA::TransformComponent* rootTC = rootEntity->GetComponent<DAVA::TransformComponent>();
        DAVA::Matrix4 invertParentTranfsorm = rootTC->GetWorldMatrix();
        invertParentTranfsorm.Inverse();
        DAVA::AABBox3 bbox = rootEntity->GetWTMaximumBoundingBoxSlow();

        DAVA::float32 zCoord = bbox.max.z;
        DAVA::float32 minX = bbox.min.x;
        DAVA::float32 maxX = bbox.max.x;
        DAVA::float32 minY = bbox.min.y;
        DAVA::float32 maxY = bbox.max.y;

        DAVA::uint32 placedCount = 0;
        while (placedCount < hingedCountOnObject)
        {
            DAVA::float32 maxZSize = 0.0f;
            DAVA::float32 currentY = minY;
            while (placedCount < hingedCountOnObject && currentY < maxY)
            {
                DAVA::float32 maxYSize = 0.0f;
                DAVA::float32 currentX = minX;
                while (placedCount < hingedCountOnObject && currentX < maxX)
                {
                    if (hingedEquipPathes.empty())
                    {
                        currentX = maxX;
                        currentY = maxY;
                        placedCount = hingedCountOnObject;
                        break;
                    }

                    DAVA::FilePath hingedEquipPath = hingedEquipPathes.back();
                    hingedEquipPathes.pop_back();

                    DAVA::Entity* hingedEntity = scene->cache.GetClone(hingedEquipPath);
                    if (hingedEntity == nullptr)
                    {
                        DAVA::Logger::Error("[RandomPlaceHingedEquipment] Can't load hinged equip %s", hingedEquipPath.GetAbsolutePathname().c_str());
                        continue;
                    }

                    DAVA::AABBox3 hingedBox = hingedEntity->GetWTMaximumBoundingBoxSlow();
                    DAVA::Vector3 hingedSize = hingedBox.GetSize();

                    DAVA::Vector3 position = DAVA::Vector3(currentX, currentY, zCoord);
                    DAVA::Transform hingedTransform;
                    hingedTransform.SetTranslation(position);
                    hingedTransform = DAVA::Transform(invertParentTranfsorm) * hingedTransform;

                    DAVA::TransformComponent* hingedTC = hingedEntity->GetComponent<DAVA::TransformComponent>();
                    hingedTC->SetLocalTransform(hingedTransform);
                    rootEntity->AddNode(hingedEntity);

                    currentX += hingedSize.x;
                    maxYSize = std::max(maxYSize, hingedSize.y);
                    maxZSize = std::max(maxZSize, hingedSize.z);
                    ++placedCount;
                }

                currentY += maxYSize;
            }

            zCoord += maxZSize;
        }

        scene->SaveScene(scenePath);
    }

    return DAVA::ConsoleModule::eFrameResult::FINISHED;
}

void RandomPlaceHingedEquipment::BeforeDestroyedInternal()
{
    DAVA::SceneConsoleHelper::FlushRHI();
}

void RandomPlaceHingedEquipment::ShowHelpInternal()
{
    CommandLineModule::ShowHelpInternal();

    DAVA::Logger::Info("Examples:");
    DAVA::Logger::Info("\t-placehingedequip -indir /Users/SmokeTest/DataSource/3d/ -processfilelist /Users/SmokeTest/objectList.txt -processdir /Users/SmokeTest/DataSource/3d/HingedEquip/");
}

DECL_TARC_MODULE(DuplicateObjectTool);
DECL_TARC_MODULE(RandomPlaceHingedEquipment);
