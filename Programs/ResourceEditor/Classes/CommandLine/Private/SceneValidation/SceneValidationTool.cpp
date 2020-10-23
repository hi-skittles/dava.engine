#include "Classes/CommandLine/SceneValidationTool.h"
#include "Classes/Qt/Scene/Validation/SceneValidation.h"
#include "Classes/Qt/Scene/Validation/ValidationProgressConsumer.h"

#include <REPlatform/CommandLine/OptionName.h>
#include <REPlatform/DataNodes/ProjectResources.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/Deprecated/EditorConfig.h>
#include <REPlatform/Scene/Components/CollisionTypeComponent.h>

#include <TArc/Core/ContextAccessor.h>
#include <TArc/Utils/ModuleCollection.h>

#include <Base/BaseTypes.h>
#include <Base/ScopedPtr.h>
#include <Entity/ComponentManager.h>
#include <Scene3D/Scene.h>
#include <Utils/StringUtils.h>
#include <FileSystem/File.h>
#include <Logger/Logger.h>

namespace SceneValidationToolDetails
{
using namespace DAVA;

Vector<FilePath> ReadScenesListFile(const FilePath& listFilePath)
{
    Vector<FilePath> scenes;
    ScopedPtr<File> listFile(File::Create(listFilePath, File::OPEN | File::READ));
    if (listFile)
    {
        while (!listFile->IsEof())
        {
            String str = StringUtils::Trim(listFile->ReadLine());
            if (!str.empty())
            {
                scenes.push_back(str);
            }
        }
    }
    else
    {
        Logger::Error("Can't open scenes listfile %s", listFilePath.GetAbsolutePathname().c_str());
    }

    return scenes;
}
}

SceneValidationTool::SceneValidationTool(const DAVA::Vector<DAVA::String>& commandLine)
    : CommandLineModule(commandLine, "-scenevalidation")
{
    using namespace DAVA;

    options.AddOption(OptionName::Scene, DAVA::VariantType(DAVA::String("")), "Path to validated scene");
    options.AddOption(OptionName::ProcessFileList, DAVA::VariantType(DAVA::String("")), "Path to file with the list of validated scenes");
    options.AddOption(OptionName::Validate, DAVA::VariantType(DAVA::String("all")), "Validation options: all, matrices, sameNames, collisionTypes, texturesRelevance, materialGroups", true);
}

void SceneValidationTool::EnableAllValidations()
{
    validateMatrices = true;
    validateSameNames = true;
    validateCollisionTypes = true;
    validateTexturesRelevance = true;
    validateMaterialGroups = true;
}

bool SceneValidationTool::PostInitInternal()
{
    using namespace DAVA;

    scenePath = options.GetOption(OptionName::Scene).AsString();
    scenesListPath = options.GetOption(OptionName::ProcessFileList).AsString();

    if (scenePath.IsEmpty() && scenesListPath.IsEmpty())
    {
        DAVA::Logger::Error("'%s' or '%s' param should be specified", OptionName::Scene.c_str(), OptionName::ProcessFileList.c_str());
        return false;
    }

    if (!scenePath.IsEmpty() && !scenesListPath.IsEmpty())
    {
        DAVA::Logger::Error("Both '%s' and '%s' params should not be specified", OptionName::Scene.c_str(), OptionName::ProcessFileList.c_str());
        return false;
    }

    DAVA::uint32 validationOptionsCount = options.GetOptionValuesCount(OptionName::Validate);
    if (validationOptionsCount == 0)
    {
        DAVA::Logger::Error("Any validation option should be specified");
        return false;
    }

    for (DAVA::uint32 n = 0; n < validationOptionsCount; ++n)
    {
        DAVA::String option = options.GetOption(OptionName::Validate, n).AsString();
        if (option == "all")
        {
            EnableAllValidations();
            break;
        }
        else if (option == "matrices")
        {
            validateMatrices = true;
        }
        else if (option == "sameNames")
        {
            validateSameNames = true;
        }
        else if (option == "collisionTypes")
        {
            validateCollisionTypes = true;
        }
        else if (option == "texturesRelevance")
        {
            validateTexturesRelevance = true;
        }
        else if (option == "materialGroups")
        {
            validateMaterialGroups = true;
        }
        else
        {
            DAVA::Logger::Error("Undefined validation option: '%s'", option.c_str());
            return false;
        }
    }

    return true;
}

void SceneValidationTool::UpdateResult(DAVA::Result newResult)
{
    if (newResult.type != DAVA::Result::RESULT_SUCCESS)
    {
        result = newResult;
    }
}

DAVA::ConsoleModule::eFrameResult SceneValidationTool::OnFrameInternal()
{
    using namespace DAVA;

    ComponentManager* cm = GetEngineContext()->componentManager;
    if (cm->IsRegisteredSceneComponent(Type::Instance<DAVA::CollisionTypeComponent>()) == false)
    {
        DAVA_REFLECTION_REGISTER_PERMANENT_NAME(DAVA::CollisionTypeComponent);
        cm->RegisterComponent(Type::Instance<DAVA::CollisionTypeComponent>());
    }

    Vector<FilePath> scenePathes;
    if (!scenesListPath.IsEmpty())
    {
        scenePathes = SceneValidationToolDetails::ReadScenesListFile(scenesListPath);
    }
    else
    {
        scenePathes.push_back(scenePath);
    }

    ProjectResources project(&GetAccessor());

    ValidationProgressToLog progressToLog;

    for (const FilePath& scenePath : scenePathes)
    {
        const FilePath projectPath = ProjectManagerData::CreateProjectPathFromPath(scenePath);
        project.LoadProject(projectPath);

        ScopedPtr<Scene> scene(new Scene);
        if (DAVA::SceneFileV2::ERROR_NO_ERROR == scene->LoadScene(scenePath))
        {
            Logger::Info("Validating scene '%s'", scenePath.GetAbsolutePathname().c_str());

            DAVA::ProjectManagerData* data = GetAccessor().GetGlobalContext()->GetData<DAVA::ProjectManagerData>();
            DVASSERT(data != nullptr);
            const std::unique_ptr<EditorConfig> config = std::make_unique<EditorConfig>();
            config->ParseConfig(projectPath + "EditorConfig.yaml");
            SceneValidation validation(data, config->GetCollisionTypeMap("CollisionType"));

            if (validateMatrices)
            {
                ValidationProgress validationProgress;
                validationProgress.SetProgressConsumer(&progressToLog);
                validation.ValidateMatrices(scene, validationProgress);
                UpdateResult(validationProgress.GetResult());
            }

            if (validateSameNames)
            {
                ValidationProgress validationProgress;
                validationProgress.SetProgressConsumer(&progressToLog);
                validation.ValidateSameNames(scene, validationProgress);
                UpdateResult(validationProgress.GetResult());
            }

            if (validateCollisionTypes)
            {
                ValidationProgress validationProgress;
                validationProgress.SetProgressConsumer(&progressToLog);
                validation.ValidateCollisionProperties(scene, validationProgress);
                UpdateResult(validationProgress.GetResult());
            }

            if (validateTexturesRelevance)
            {
                ValidationProgress validationProgress;
                validationProgress.SetProgressConsumer(&progressToLog);
                validation.ValidateTexturesRelevance(scene, validationProgress);
                UpdateResult(validationProgress.GetResult());
            }

            if (validateMaterialGroups)
            {
                ValidationProgress validationProgress;
                validationProgress.SetProgressConsumer(&progressToLog);
                validation.ValidateMaterialsGroups(scene, validationProgress);
                UpdateResult(validationProgress.GetResult());
            }
        }
        else
        {
            Logger::Error("Can't open scene '%s'", scenePath.GetAbsolutePathname().c_str());
        }
    }

    return eFrameResult::FINISHED;
}

DECL_TARC_MODULE(SceneValidationTool);
