#include "Classes/Library/Private/DAEConverter.h"
#include "Classes/Collada/ColladaConvert.h"
#include "Classes/Collada/ImportParams.h"

#include <FileSystem/FileSystem.h>
#include <Logger/Logger.h>
#include <Engine/Engine.h>

namespace DAEConverter
{
bool Convert(const DAVA::FilePath& daePath)
{
    DAVA::FileSystem* fileSystem = DAVA::GetEngineContext()->fileSystem;
    if (fileSystem->Exists(daePath) && daePath.IsEqualToExtension(".dae"))
    {
        std::unique_ptr<ImportParams> importParams = std::make_unique<ImportParams>();
        DAVA::FilePath etalonScenePath = daePath;
        etalonScenePath.ReplaceExtension(".sc2");
        if (fileSystem->Exists(etalonScenePath))
        {
            DAVA::RefPtr<DAVA::Scene> scene;
            scene.ConstructInplace();
            DAVA::SceneFileV2::eError ret = scene->LoadScene(etalonScenePath);
            if (ret == DAVA::SceneFileV2::ERROR_NO_ERROR)
            {
                AccumulateImportParams(scene, etalonScenePath, importParams.get());
            }
        }

        eColladaErrorCodes code = ConvertDaeToSc2(daePath, std::move(importParams));
        if (code == COLLADA_OK)
        {
            return true;
        }
        else if (code == COLLADA_ERROR_OF_ROOT_NODE)
        {
            DAVA::Logger::Error("Can't convert from DAE. Looks like one of materials has same name as root node.");
        }
        else
        {
            DAVA::Logger::Error("[DAE to SC2] Can't convert from DAE.");
        }
    }
    else
    {
        DAVA::Logger::Error("[DAE to SC2] Wrong pathname: %s.", daePath.GetStringValue().c_str());
    }

    return false;
}

bool ConvertAnimations(const DAVA::FilePath& daePath)
{
    DAVA::FileSystem* fileSystem = DAVA::GetEngineContext()->fileSystem;
    if (fileSystem->Exists(daePath) && daePath.IsEqualToExtension(".dae"))
    {
        eColladaErrorCodes code = ConvertDaeToAnimations(daePath);
        if (code == COLLADA_OK)
        {
            return true;
        }
        else
        {
            DAVA::Logger::Error("[DAE to animations] Can't convert from DAE.");
        }
    }
    else
    {
        DAVA::Logger::Error("[DAE to animations] Wrong pathname: %s.", daePath.GetStringValue().c_str());
    }

    return false;
}
}
