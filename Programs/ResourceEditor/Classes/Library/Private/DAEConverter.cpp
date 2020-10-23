#include "Classes/Library/Private/DAEConverter.h"
#include "Classes/Collada/ColladaConvert.h"

#include <FileSystem/FileSystem.h>
#include <Logger/Logger.h>
#include <Engine/Engine.h>

bool DAEConverter::Convert(const DAVA::FilePath& daePath)
{
    DAVA::FileSystem* fileSystem = DAVA::GetEngineContext()->fileSystem;
    if (fileSystem->Exists(daePath) && daePath.IsEqualToExtension(".dae"))
    {
        eColladaErrorCodes code = ConvertDaeToSc2(daePath);
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

bool DAEConverter::ConvertAnimations(const DAVA::FilePath& daePath)
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
        DAVA::Logger::Error("[DAE to animations] Wrong pathname: %s", daePath.GetStringValue().c_str());
    }

    return false;
}
