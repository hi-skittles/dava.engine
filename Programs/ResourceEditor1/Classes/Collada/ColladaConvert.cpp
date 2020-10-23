#include "ColladaConvert.h"
#include "ColladaDocument.h"
#include "Collada/ColladaToSc2Importer/ColladaImporter.h"
#include "Classes/Collada/ImportParams.h"

eColladaErrorCodes ConvertDaeToSc2(const DAVA::FilePath& pathToFile, std::unique_ptr<DAEConverter::ImportParams>&& importParams)
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

    if (ret == COLLADA_OK)
    {
        using namespace DAVA;
        ScopedPtr<Scene> scene(new Scene());
        SceneFileV2::eError loadResult = scene->LoadScene(pathSc2);
        if (loadResult != SceneFileV2::ERROR_NO_ERROR)
        {
            DAVA::Logger::Error("[ConvertDaeToSc2] Failed to read scene %s {%u} for apply reimport params", pathToFile.GetAbsolutePathname().c_str(), loadResult);
        }

        DAEConverter::RestoreSceneParams(RefPtr<Scene>::ConstructWithRetain(scene), pathSc2, importParams.get());
        SceneFileV2::eError saveRes = scene->SaveScene(pathSc2, false);

        if (saveRes > SceneFileV2::eError::ERROR_NO_ERROR)
        {
            Logger::Error("[DAE to SC2] Cannot save SC2 after apply reimport params. Error %d", saveRes);
            ret = eColladaErrorCodes::COLLADA_ERROR;
        }
    }

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
