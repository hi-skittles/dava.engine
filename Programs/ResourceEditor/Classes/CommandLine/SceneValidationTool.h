#pragma once

#include <REPlatform/Global/CommandLineModule.h>
#include "Classes/Qt/Scene/Validation/ValidationProgress.h"

#include <Reflection/ReflectionRegistrator.h>

namespace DAVA
{
class ProjectManagerData;
} // namespace DAVA

class SceneValidationTool : public DAVA::CommandLineModule
{
public:
    SceneValidationTool(const DAVA::Vector<DAVA::String>& commandLine);

    static const DAVA::String Key;

private:
    bool PostInitInternal() override;
    eFrameResult OnFrameInternal() override;

    DAVA::ProjectManagerData* GetProjectManagerData();

    void UpdateResult(DAVA::Result);
    void EnableAllValidations();

    DAVA::FilePath scenePath;
    DAVA::FilePath scenesListPath;

    bool validateMatrices = false;
    bool validateSameNames = false;
    bool validateCollisionTypes = false;
    bool validateTexturesRelevance = false;
    bool validateMaterialGroups = false;

    DAVA::FilePath qualityConfigPath;

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(SceneValidationTool, DAVA::CommandLineModule)
    {
        DAVA::ReflectionRegistrator<SceneValidationTool>::Begin()[DAVA::M::CommandName("-scenevalidation")]
        .ConstructorByPointer<DAVA::Vector<DAVA::String>>()
        .End();
    }
};
