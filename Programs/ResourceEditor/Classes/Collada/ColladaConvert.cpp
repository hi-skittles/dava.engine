#include "ColladaConvert.h"
#include "ColladaDocument.h"
#include "Classes/Collada/ColladaToSc2Importer/ColladaImporter.h"

eColladaErrorCodes ConvertDaeToSc2(const DAVA::FilePath& pathToFile)
{
    FCollada::Initialize();

    DAVA::ColladaDocument colladaDocument;

    eColladaErrorCodes code = colladaDocument.Open(pathToFile.GetAbsolutePathname().c_str());
    if (code != COLLADA_OK)
    {
        DAVA::Logger::Error("[ConvertDaeToSc2] Failed to read %s with error %d", pathToFile.GetAbsolutePathname().c_str(), (int32)code);
        return code;
    }

    DAVA::FilePath pathSc2 = DAVA::FilePath::CreateWithNewExtension(pathToFile, ".sc2");

    eColladaErrorCodes ret = colladaDocument.SaveSC2(pathSc2);
    colladaDocument.Close();

    FCollada::Release();
    return ret;
}

eColladaErrorCodes ConvertDaeToAnimations(const DAVA::FilePath& pathToFile)
{
    FCollada::Initialize();

    DAVA::ColladaDocument colladaDocument;

    eColladaErrorCodes code = colladaDocument.Open(pathToFile.GetAbsolutePathname().c_str(), true);
    if (code != COLLADA_OK)
    {
        DAVA::Logger::Error("[ConvertDaeToAnimations] Failed to read %s with error %d", pathToFile.GetAbsolutePathname().c_str(), (int32)code);
        return code;
    }

    DAVA::FilePath saveDirectory = pathToFile.GetDirectory();

    eColladaErrorCodes ret = colladaDocument.SaveAnimations(saveDirectory);
    colladaDocument.Close();

    FCollada::Release();

    return ret;
}
