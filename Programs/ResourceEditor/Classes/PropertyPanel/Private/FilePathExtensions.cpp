#include "Classes/PropertyPanel/FilePathExtensions.h"

#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/Global/GlobalOperations.h>
#include <REPlatform/Scene/Utils/RETextureDescriptorUtils.h>

#include <TArc/Core/FieldBinder.h>
#include <TArc/Core/Deprecated.h>
#include <TArc/Core/OperationInvoker.h>

#include <Base/Any.h>
#include <Base/FastName.h>
#include <Base/StaticSingleton.h>
#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <FileSystem/FileSystem.h>
#include <Reflection/ReflectedTypeDB.h>
#include <Render/Highlevel/Heightmap.h>
#include <Render/Image/ImageFormatInterface.h>
#include <Render/Image/ImageSystem.h>

namespace PathValidatorsDetail
{
class PathValidatorInfo : public DAVA::StaticSingleton<PathValidatorInfo>
{
public:
    void EnsureInited(DAVA::ContextAccessor* accessor)
    {
        if (binder == nullptr)
        {
            binder.reset(new DAVA::FieldBinder(accessor));

            {
                DAVA::FieldDescriptor descr;
                descr.fieldName = DAVA::FastName(DAVA::ProjectManagerData::ProjectPathProperty);
                descr.type = DAVA::ReflectedTypeDB::Get<DAVA::ProjectManagerData>();
                binder->BindField(descr, DAVA::MakeFunction(this, &PathValidatorInfo::OnProjectPathChanged));
            }

            {
                DAVA::FieldDescriptor descr;
                descr.fieldName = DAVA::FastName(DAVA::SceneData::scenePathPropertyName);
                descr.type = DAVA::ReflectedTypeDB::Get<DAVA::SceneData>();
                binder->BindField(descr, DAVA::MakeFunction(this, &PathValidatorInfo::OnScenePathChanged));
            }
            InitImageFilesExtensions();
        }
    }

    const DAVA::FilePath& GetProjectPath() const
    {
        return projectPath;
    }

    DAVA::FilePath GetDataSource3DPath() const
    {
        return DAVA::ProjectManagerData::GetDataSource3DPath(projectPath);
    }

    const DAVA::FilePath& GetScenePath() const
    {
        return scenePath;
    }

    const DAVA::String& GetSourceFileString() const
    {
        return sourceFileString;
    }

    const DAVA::String& GetSeparateSourceFileString() const
    {
        return separateSourceFileString;
    }

private:
    void OnProjectPathChanged(const DAVA::Any& value)
    {
        if (!value.IsEmpty())
        {
            projectPath = value.Cast<DAVA::FilePath>();
        }
        else
        {
            projectPath = DAVA::FilePath();
        }
    }

    void OnScenePathChanged(const DAVA::Any& value)
    {
        if (!value.IsEmpty())
        {
            scenePath = value.Cast<DAVA::FilePath>();
        }
        else
        {
            scenePath = DAVA::FilePath();
        }
    }

    void InitImageFilesExtensions()
    {
        for (DAVA::ImageFormat formatType : DAVA::TextureDescriptor::sourceTextureTypes)
        {
            DAVA::String fileTypeString;

            const DAVA::Vector<DAVA::String>& extensions = DAVA::ImageSystem::GetExtensionsFor(formatType);

            for (const DAVA::String& ex : extensions)
            {
                if (fileTypeString.empty())
                {
                    fileTypeString = DAVA::ImageSystem::GetImageFormatInterface(formatType)->GetName() + " (*";
                }
                else
                {
                    fileTypeString += " *";
                }
                fileTypeString += ex.c_str();

                if (sourceFileString.empty())
                {
                    sourceFileString = "*";
                }
                else
                {
                    sourceFileString += " *";
                }
                sourceFileString += ex.c_str();
            }

            fileTypeString += ")";

            if (!separateSourceFileString.empty())
            {
                separateSourceFileString += ";;";
            }
            separateSourceFileString += fileTypeString;
        }
    }

private:
    std::unique_ptr<DAVA::FieldBinder> binder;
    DAVA::FilePath projectPath;
    DAVA::FilePath scenePath;

    DAVA::String sourceFileString;
    DAVA::String separateSourceFileString;
};

DAVA::FilePath GetValidDir(bool withScene)
{
    PathValidatorInfo* info = PathValidatorInfo::Instance();
    DAVA::FilePath resultPath = info->GetProjectPath();
    DAVA::FilePath dataSourcePath = info->GetDataSource3DPath();

    const DAVA::EngineContext* ctx = DAVA::GetEngineContext();

    if (ctx->fileSystem->Exists(dataSourcePath))
    {
        resultPath = dataSourcePath;
    }

    DAVA::FilePath scenePath = info->GetScenePath();
    if (withScene && scenePath.IsEmpty() == false && ctx->fileSystem->Exists(scenePath))
    {
        if (DAVA::FilePath::ContainPath(scenePath, dataSourcePath.GetAbsolutePathname().c_str()))
        {
            resultPath = scenePath.GetDirectory();
        }
    }

    return resultPath;
}

DAVA::M::ValidationResult ValidateHeightMap(const DAVA::Any& value, const DAVA::Any& oldValue)
{
    using namespace DAVA;
    M::ValidationResult result;

    FilePath heightMapPath = value.Cast<FilePath>();
    if (heightMapPath.IsEmpty() == false)
    {
        FilePath validDir = GetValidDir(true);
        if (FilePath::ContainPath(heightMapPath, validDir) == false)
        {
            result.state = M::ValidationResult::eState::Invalid;
            result.message = Format("\"%s\" is wrong. It's allowed to select only from %s", heightMapPath.GetAbsolutePathname().c_str(), validDir.GetAbsolutePathname().c_str());
            return result;
        }

        if (heightMapPath.IsEqualToExtension(".heightmap") == false)
        {
            auto extension = heightMapPath.GetExtension();
            auto imageFormat = ImageSystem::GetImageFormatForExtension(extension);

            if (IMAGE_FORMAT_UNKNOWN != imageFormat)
            {
                auto imgSystem = ImageSystem::GetImageFormatInterface(imageFormat);
                Size2i size = imgSystem->GetImageInfo(heightMapPath).GetImageSize();
                if (size.dx != size.dy)
                {
                    result.state = M::ValidationResult::eState::Invalid;
                    result.message = Format("\"%s\" has wrong size: landscape requires square heightmap.", heightMapPath.GetAbsolutePathname().c_str());
                    return result;
                }

                if (!IsPowerOf2(size.dx))
                {
                    result.state = M::ValidationResult::eState::Invalid;
                    result.message = Format("\"%s\" has wrong size: landscape requires square heightmap with size 2^n.", heightMapPath.GetAbsolutePathname().c_str());
                    return result;
                }

                Vector<Image*> imageVector;
                ImageSystem::Load(heightMapPath, imageVector);
                DVASSERT(imageVector.size());

                PixelFormat format = imageVector[0]->GetPixelFormat();

                for_each(imageVector.begin(), imageVector.end(), SafeRelease<Image>);
                if (format != FORMAT_A8 && format != FORMAT_A16)
                {
                    result.state = M::ValidationResult::eState::Invalid;
                    result.message = Format("\"%s\" is wrong: png file should be in format A8 or A16.", heightMapPath.GetAbsolutePathname().c_str());
                    return result;
                }
            }
            else
            {
                result.state = M::ValidationResult::eState::Invalid;
                result.message = Format("\"%s\" is wrong: should be *.png, *.tga, *.jpeg or *.heightmap.", heightMapPath.GetAbsolutePathname().c_str());
                return result;
            }
        }
    }

    result.state = M::ValidationResult::eState::Valid;
    return result;
}

DAVA::M::ValidationResult ValidateTexture(const DAVA::Any& value, const DAVA::Any& oldValue, bool withScene)
{
    using namespace DAVA;
    M::ValidationResult result;

    FilePath texturePath = value.Cast<FilePath>();
    if (texturePath.IsEmpty() == false)
    {
        FilePath validDir = GetValidDir(withScene);
        if (FilePath::ContainPath(texturePath, validDir) == false)
        {
            result.state = M::ValidationResult::eState::Invalid;
            result.message = Format("\"%s\" is wrong. It's allowed to select only from %s", texturePath.GetAbsolutePathname().c_str(), validDir.GetAbsolutePathname().c_str());
            return result;
        }

        if (texturePath.GetExtension() != TextureDescriptor::GetDescriptorExtension())
        {
            result.state = M::ValidationResult::eState::Invalid;
            const EngineContext* ctx = GetEngineContext();
            if (ctx->fileSystem->Exists(texturePath) && RETextureDescriptorUtils::CreateOrUpdateDescriptor(texturePath))
            {
                FilePath descriptorPath = TextureDescriptor::GetDescriptorPathname(texturePath);

                const TexturesMap& texturesMap = Texture::GetTextureMap();
                auto found = texturesMap.find(FILEPATH_MAP_KEY(descriptorPath));
                if (found != texturesMap.end())
                {
                    DAVA::Vector<DAVA::Texture*> reloadTextures;
                    reloadTextures.push_back(found->second);

                    DAVA::Deprecated::GetInvoker()->Invoke(DAVA::ReloadTextures.ID, reloadTextures);
                }

                result.fixedValue = FilePath(descriptorPath);
            }
            return result;
        }
    }

    result.state = M::ValidationResult::eState::Valid;
    return result;
}

DAVA::M::ValidationResult ValidateTextureWithScene(const DAVA::Any& value, const DAVA::Any& oldValue)
{
    return ValidateTexture(value, oldValue, true);
}

DAVA::M::ValidationResult ValidateTextureWithOutScene(const DAVA::Any& value, const DAVA::Any& oldValue)
{
    return ValidateTexture(value, oldValue, false);
}

DAVA::M::ValidationResult ValidateImage(const DAVA::Any& value, const DAVA::Any& oldValue)
{
    using namespace DAVA;

    M::ValidationResult result;

    FilePath imagePath = value.Cast<FilePath>();
    FilePath validDir = GetValidDir(true);
    if (FilePath::ContainPath(imagePath, validDir) == false)
    {
        result.state = M::ValidationResult::eState::Invalid;
        result.message = Format("\"%s\" is wrong. It's allowed to select only from %s", imagePath.GetAbsolutePathname().c_str(), validDir.GetAbsolutePathname().c_str());
        return result;
    }

    result.state = M::ValidationResult::eState::Valid;
    return result;
}

DAVA::M::ValidationResult ValidateScene(const DAVA::Any& value, const DAVA::Any& oldValue)
{
    using namespace DAVA;

    M::ValidationResult result;

    FilePath scenePath = value.Cast<FilePath>();
    FilePath validDir = GetValidDir(true);
    FilePath validProjectDir = GetValidDir(false);
    if ((FilePath::ContainPath(scenePath, validDir) == false &&
         FilePath::ContainPath(scenePath, validProjectDir) == false))
    {
        result.state = M::ValidationResult::eState::Invalid;
        result.message = Format("\"%s\" is wrong. It's allowed to select only from %s", scenePath.GetAbsolutePathname().c_str(), validDir.GetAbsolutePathname().c_str());
        return result;
    }

    if (CompareCaseInsensitive(scenePath.GetExtension(), ".sc2") != 0)
    {
        result.state = M::ValidationResult::eState::Invalid;
        result.message = Format("\"%s\" is not a Scene file", scenePath.GetAbsolutePathname().c_str());
        return result;
    }

    result.state = M::ValidationResult::eState::Valid;
    return result;
}

DAVA::M::ValidationResult ValidateExistsFileInProject(const DAVA::Any& value, const DAVA::Any& oldValue)
{
    using namespace DAVA;

    M::ValidationResult result;

    FilePath filePath = value.Cast<FilePath>();
    if (filePath.IsEmpty())
    {
        result.state = M::ValidationResult::eState::Valid;
        return result;
    }

    FilePath validProjectDir = GetValidDir(false);

    if (filePath.IsDirectoryPathname() == true)
    {
        result.state = M::ValidationResult::eState::Invalid;
        result.message = Format("\"%s\" is wrong. Should be file", filePath.GetAbsolutePathname().c_str());
        return result;
    }

    const DAVA::EngineContext* ctx = DAVA::GetEngineContext();

    if (ctx->fileSystem->Exists(filePath) == false)
    {
        result.state = M::ValidationResult::eState::Invalid;
        result.message = Format("\"%s\" is wrong. File doesn't exists", filePath.GetAbsolutePathname().c_str());
        return result;
    }

    if (FilePath::ContainPath(filePath, validProjectDir) == false)
    {
        result.state = M::ValidationResult::eState::Invalid;
        result.message = Format("\"%s\" is wrong. It's allowed to select only from %s", filePath.GetAbsolutePathname().c_str(), validProjectDir.GetAbsolutePathname().c_str());
        return result;
    }

    result.state = M::ValidationResult::eState::Valid;
    return result;
}

} // namespace PathValidatorsDetail

void InitFilePathExtensions(DAVA::ContextAccessor* accessor)
{
    PathValidatorsDetail::PathValidatorInfo::Instance()->EnsureInited(accessor);
}

DAVA::M::Validator CreateHeightMapValidator()
{
    return DAVA::M::Validator(PathValidatorsDetail::ValidateHeightMap);
}

DAVA::M::Validator CreateTextureValidator(bool bindToScenePath)
{
    return DAVA::M::Validator(bindToScenePath == true ? PathValidatorsDetail::ValidateTextureWithScene
                                                        :
                                                        PathValidatorsDetail::ValidateTextureWithOutScene);
}

DAVA::M::Validator CreateImageValidator()
{
    return DAVA::M::Validator(PathValidatorsDetail::ValidateImage);
}

DAVA::M::Validator CreateSceneValidator()
{
    return DAVA::M::Validator(PathValidatorsDetail::ValidateScene);
}

DAVA::M::Validator CreateExistsFile()
{
    return DAVA::M::Validator(PathValidatorsDetail::ValidateExistsFileInProject);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

REFileMeta::REFileMeta(const DAVA::String& filters, const DAVA::String& dlgTitle)
    : DAVA::Metas::File(filters, dlgTitle)
{
}

DAVA::String REFileMeta::GetDefaultPath() const
{
    return PathValidatorsDetail::GetValidDir(true).GetAbsolutePathname();
}

DAVA::String REFileMeta::GetRootDirectory() const
{
    return PathValidatorsDetail::GetValidDir(true).GetAbsolutePathname();
}

HeightMapFileMeta::HeightMapFileMeta(const DAVA::String& filters)
    : REFileMeta(filters, "Open Heightmap")
{
}

TextureFileMeta::TextureFileMeta(const DAVA::String& filters)
    : REFileMeta(filters, "Open Texture")
{
}

ImageFileMeta::ImageFileMeta(const DAVA::String& filters)
    : REFileMeta(filters, "Open Image")
{
}

SceneFileMeta::SceneFileMeta(const DAVA::String& filters)
    : REFileMeta(filters, "Open Scene")
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

GenericFileMeta<HeightMapFileMeta> CreateHeightMapFileMeta()
{
    using namespace PathValidatorsDetail;
    PathValidatorInfo* info = PathValidatorInfo::Instance();

    DAVA::String heightMapExt = DAVA::Heightmap::FileExtension();
    DAVA::String filters = DAVA::Format("All (*%s %s);;Heightmap (*%s);;%s", heightMapExt.c_str(), info->GetSourceFileString().c_str(),
                                        heightMapExt.c_str(), info->GetSeparateSourceFileString().c_str());
    return DAVA::Meta<HeightMapFileMeta, DAVA::Metas::File>(filters);
}

GenericFileMeta<TextureFileMeta> CreateTextureFileMeta()
{
    using namespace PathValidatorsDetail;
    PathValidatorInfo* info = PathValidatorInfo::Instance();

    DAVA::String textureExt = DAVA::TextureDescriptor::GetDescriptorExtension();
    DAVA::String filters = DAVA::Format("All (*%s %s);;TEX (*%s);;%s", textureExt.c_str(), info->GetSourceFileString().c_str(),
                                        textureExt.c_str(), info->GetSeparateSourceFileString().c_str());
    return DAVA::Meta<TextureFileMeta, DAVA::Metas::File>(filters);
}

GenericFileMeta<ImageFileMeta> CreateImageFileMeta()
{
    using namespace PathValidatorsDetail;
    PathValidatorInfo* info = PathValidatorInfo::Instance();

    DAVA::String filters = DAVA::Format("All (%s);;%s", info->GetSourceFileString().c_str(), info->GetSeparateSourceFileString().c_str());
    return DAVA::Meta<ImageFileMeta, DAVA::Metas::File>(filters);
}

GenericFileMeta<SceneFileMeta> CreateSceneFileMeta()
{
    return DAVA::Meta<SceneFileMeta, DAVA::Metas::File>("All(*.sc2);; SC2(*.sc2);");
}
