#include "Classes/Utils/SceneSaver/SceneSaver.h"
#include "Classes/Qt/Main/QtUtils.h"
#include "Classes/Qt/Scene/SceneHelper.h"
#include "Classes/Project/ProjectManagerData.h"
#include "Classes/Deprecated/SceneValidator.h"
#include "Classes/StringConstants.h"

#include <Scene3D/Components/CustomPropertiesComponent.h>
#include <Scene3D/Components/MotionComponent.h>
#include <Scene3D/Components/SlotComponent.h>
#include <Scene3D/Systems/SlotSystem.h>
#include <FileSystem/FileList.h>
#include <FileSystem/FileSystem.h>
#include <Engine/Engine.h>
#include <Time/SystemTimer.h>

SceneSaver::SceneSaver()
{
    tags.push_back(""); // we have "" in tags for default behavior
}

SceneSaver::~SceneSaver()
{
    ReleaseTextures();
}

void SceneSaver::SetInFolder(const DAVA::FilePath& folderPathname)
{
    sceneUtils.SetInFolder(folderPathname);
}

void SceneSaver::SetOutFolder(const DAVA::FilePath& folderPathname)
{
    sceneUtils.SetOutFolder(folderPathname);
}

void SceneSaver::EnableCopyConverted(bool enabled)
{
    copyConverted = enabled;
}

void SceneSaver::SetTags(const DAVA::Vector<DAVA::String>& tags_)
{
    DVASSERT(tags_.empty() == false && tags_[0].empty() == true); // mean that we have "" in tags for default behavior
    tags = tags_;
}

void SceneSaver::SaveFile(const DAVA::String& fileName)
{
    using namespace DAVA;

    Logger::FrameworkDebug("[SceneSaver::SaveFile] %s", fileName.c_str());

    FilePath filePath = sceneUtils.dataSourceFolder + fileName;

    //Load scene with *.sc2
    Scene* scene = new Scene();
    if (SceneFileV2::ERROR_NO_ERROR == scene->LoadScene(filePath))
    {
        SaveScene(scene, filePath);
    }
    else
    {
        Logger::Error("[SceneSaver::SaveFile] Can't open file %s", fileName.c_str());
    }

    SafeRelease(scene);
}

void SceneSaver::ResaveFile(const DAVA::String& fileName)
{
    using namespace DAVA;

    Logger::FrameworkDebug("[SceneSaver::ResaveFile] %s", fileName.c_str());

    FilePath sc2Filename = sceneUtils.dataSourceFolder + fileName;

    //Load scene with *.sc2
    Scene* scene = new Scene();
    if (SceneFileV2::ERROR_NO_ERROR == scene->LoadScene(sc2Filename))
    {
        scene->SaveScene(sc2Filename, false);
    }
    else
    {
        Logger::Error("[SceneSaver::ResaveFile] Can't open file %s", fileName.c_str());
    }

    SafeRelease(scene);
}

void SceneSaver::SaveScene(DAVA::Scene* scene, const DAVA::FilePath& fileName)
{
    using namespace DAVA;

    String absScenePath = fileName.GetAbsolutePathname();
    if (savedExternalScenes.count(absScenePath) > 0)
    {
        return;
    }

    savedExternalScenes.insert(absScenePath);
    uint64 startTime = SystemTimer::GetMs();

    DVASSERT(0 == texturesForSave.size());

    String relativeFilename = fileName.GetRelativePathname(sceneUtils.dataSourceFolder);
    sceneUtils.workingFolder = fileName.GetDirectory().GetRelativePathname(sceneUtils.dataSourceFolder);

    FileSystem::Instance()->CreateDirectory(sceneUtils.dataFolder + sceneUtils.workingFolder, true);

    //scene->Update(0.1f);

    SceneValidator validator;
    validator.SetPathForChecking(sceneUtils.dataSourceFolder);
    validator.ValidateScene(scene, fileName);

    {
        SceneHelper::TextureCollector collector(SceneHelper::TextureCollector::IncludeNullTextures);
        SceneHelper::EnumerateSceneTextures(scene, collector);
        texturesForSave = std::move(collector.GetTextures());
    }

    CopyTextures(scene);
    ReleaseTextures();

    Landscape* landscape = FindLandscape(scene);
    if (landscape)
    {
        sceneUtils.AddFile(landscape->GetHeightmapPathname());
    }

    VegetationRenderObject* vegetation = FindVegetation(scene);
    if (vegetation)
    {
        const FilePath vegetationCustomGeometry = vegetation->GetCustomGeometryPath();
        if (!vegetationCustomGeometry.IsEmpty())
        {
            sceneUtils.AddFile(vegetationCustomGeometry);
        }
    }

    CopyReferencedObject(scene);
    CopyAnimationClips(scene);
    CopyEffects(scene);
    CopyCustomColorTexture(scene, fileName.GetDirectory());

    Set<FilePath> externalScenes;
    CopySlots(scene, externalScenes);

    //save scene to new place
    FilePath tempSceneName = sceneUtils.dataSourceFolder + relativeFilename;
    tempSceneName.ReplaceExtension(".saved.sc2");

    sceneUtils.CopyFiles();
    scene->SaveScene(tempSceneName, false);

    bool moved = FileSystem::Instance()->MoveFile(tempSceneName, sceneUtils.dataFolder + relativeFilename, true);
    if (!moved)
    {
        Logger::Error("Can't move file %s", fileName.GetAbsolutePathname().c_str());
    }

    uint64 saveTime = SystemTimer::GetMs() - startTime;
    Logger::FrameworkDebug("Save of %s to folder was done for %ldms", fileName.GetStringValue().c_str(), saveTime);

    for (const FilePath& externalScene : externalScenes)
    {
        ScopedPtr<Scene> scene(new Scene());
        if (SceneFileV2::ERROR_NO_ERROR == scene->LoadScene(externalScene))
        {
            SaveScene(scene, externalScene.GetAbsolutePathname());
        }
        else
        {
            Logger::Error("[SceneSaver::ResaveFile] Can't open file %s", externalScene.GetAbsolutePathname().c_str());
        }
    }
}

void SceneSaver::CopyTextures(DAVA::Scene* scene)
{
    using namespace DAVA;

    for (const auto& it : texturesForSave)
    {
        if (it.first.GetType() == DAVA::FilePath::PATH_IN_MEMORY)
        {
            continue;
        }

        FileSystem* fs = GetEngineContext()->fileSystem;
        for (const DAVA::String& tag : tags)
        {
            FilePath path = it.first;
            path.ReplaceBasename(path.GetBasename() + tag);
            if (fs->Exists(path) == true)
            {
                CopyTexture(path);
            }
        }
    }
}

void SceneSaver::ReleaseTextures()
{
    texturesForSave.clear();
}

void SceneSaver::CopyTexture(const DAVA::FilePath& texturePathname)
{
    using namespace DAVA;

    FilePath descriptorPathname = TextureDescriptor::GetDescriptorPathname(texturePathname);

    TextureDescriptor* desc = TextureDescriptor::CreateFromFile(descriptorPathname);
    if (!desc)
    {
        Logger::Error("Can't open file %s", descriptorPathname.GetAbsolutePathname().c_str());
        return;
    }

    //copy descriptor
    sceneUtils.AddFile(descriptorPathname);

    //copy source textures
    if (desc->IsCubeMap())
    {
        Vector<FilePath> faceNames;

        desc->GetFacePathnames(faceNames);
        for (const FilePath& faceName : faceNames)
        {
            if (!faceName.IsEmpty())
                sceneUtils.AddFile(faceName);
        }
    }
    else
    {
        sceneUtils.AddFile(desc->GetSourceTexturePathname());
    }

    //copy converted textures (*.pvr and *.dds)
    if (copyConverted)
    {
        for (int32 i = 0; i < GPU_DEVICE_COUNT; ++i)
        {
            eGPUFamily gpu = (eGPUFamily)i;

            PixelFormat format = desc->GetPixelFormatForGPU(gpu);
            if (format == FORMAT_INVALID)
            {
                continue;
            }

            Vector<FilePath> imagePathnames;
            desc->CreateLoadPathnamesForGPU(gpu, imagePathnames);
            for (const FilePath& path : imagePathnames)
            {
                sceneUtils.AddFile(path);
            }
        }
    }

    delete desc;
}

void SceneSaver::CopyReferencedObject(DAVA::Entity* node)
{
    using namespace DAVA;

    KeyedArchive* customProperties = GetCustomPropertiesArchieve(node);
    if (customProperties && customProperties->IsKeyExists(ResourceEditor::EDITOR_REFERENCE_TO_OWNER))
    {
        String path = customProperties->GetString(ResourceEditor::EDITOR_REFERENCE_TO_OWNER);
        sceneUtils.AddFile(path);
    }

    for (int32 i = 0; i < node->GetChildrenCount(); i++)
    {
        CopyReferencedObject(node->GetChild(i));
    }
}

void SceneSaver::CopyAnimationClips(DAVA::Entity* node)
{
    for (DAVA::uint32 i = 0; i < node->GetComponentCount<DAVA::MotionComponent>(); ++i)
    {
        DAVA::MotionComponent* component = node->GetComponent<DAVA::MotionComponent>(i);
        DAVA::Vector<DAVA::FilePath> dependencies = component->GetDependencies();
        for (const DAVA::FilePath& fp : dependencies)
            sceneUtils.AddFile(fp);
    }

    for (DAVA::int32 i = 0; i < node->GetChildrenCount(); i++)
    {
        CopyAnimationClips(node->GetChild(i));
    }
}

void SceneSaver::CopySlots(DAVA::Entity* node, DAVA::Set<DAVA::FilePath>& externalScenes)
{
    using namespace DAVA;

    auto processConfig = [&](Scene* scene, const FilePath& configPath)
    {
        sceneUtils.AddFile(configPath);

        Vector<SlotSystem::ItemsCache::Item> items = scene->slotSystem->GetItems(configPath);
        for (const SlotSystem::ItemsCache::Item& item : items)
        {
            if (savedExternalScenes.count(item.scenePath) == 0)
            {
                sceneUtils.AddFile(item.scenePath);
                externalScenes.insert(item.scenePath);
            }
        }
    };

    FileSystem* fs = GetEngineContext()->fileSystem;
    for (uint32 i = 0; i < node->GetComponentCount<SlotComponent>(); ++i)
    {
        Scene* scene = node->GetScene();
        if (scene == nullptr)
        {
            scene = dynamic_cast<Scene*>(node);
            DVASSERT(scene != nullptr);
        }

        SlotComponent* component = node->GetComponent<SlotComponent>(i);
        FilePath originalConfigPath = component->GetConfigFilePath();

        for (const DAVA::String& tag : tags)
        {
            FilePath path = originalConfigPath;
            path.ReplaceBasename(path.GetBasename() + tag);
            if (fs->Exists(path) == true)
            {
                processConfig(scene, path);
            }
        }
    }

    for (int32 i = 0; i < node->GetChildrenCount(); i++)
    {
        CopySlots(node->GetChild(i), externalScenes);
    }
}

void SceneSaver::CopyEffects(DAVA::Entity* node)
{
    using namespace DAVA;

    ParticleEffectComponent* effect = GetEffectComponent(node);
    if (effect)
    {
        for (int32 i = 0, sz = effect->GetEmittersCount(); i < sz; ++i)
        {
            CopyAllParticlesEmitters(effect->GetEmitterInstance(i));
        }
    }

    for (int i = 0; i < node->GetChildrenCount(); ++i)
    {
        CopyEffects(node->GetChild(i));
    }

    for (auto it = effectFolders.begin(), endIt = effectFolders.end(); it != endIt; ++it)
    {
        FilePath flagsTXT = *it + "flags.txt";

        if (FileSystem::Instance()->Exists(flagsTXT))
        {
            sceneUtils.AddFile(flagsTXT);
        }
    }

    effectFolders.clear();
}

void SceneSaver::CopyAllParticlesEmitters(DAVA::ParticleEmitterInstance* instance)
{
    using namespace DAVA;

    const Set<FilePath>& paths = EnumAlternativeEmittersFilepaths(instance->GetFilePath());
    for (const FilePath& alternativeFilepath : paths)
    {
        ParticleEmitter* emitter = instance->GetEmitter();
        if (alternativeFilepath == emitter->configPath)
        {
            CopyEmitter(emitter);
        }
        else
        {
            CopyEmitterByPath(alternativeFilepath);
        }
    }
}

void SceneSaver::CopyEmitterByPath(const DAVA::FilePath& emitterConfigPath)
{
    DAVA::RefPtr<DAVA::ParticleEmitter> emitter(DAVA::ParticleEmitter::LoadEmitter(emitterConfigPath));
    CopyEmitter(emitter.Get());
}

void SceneSaver::CopyEmitter(DAVA::ParticleEmitter* emitter)
{
    using namespace DAVA;

    if (emitter->configPath.IsEmpty() == false)
    {
        sceneUtils.AddFile(emitter->configPath);
    }
    else
    {
        Logger::Warning("[SceneSaver::CopyEmitter] empty config path for emitter %s", emitter->name.c_str());
    }

    const Vector<ParticleLayer*>& layers = emitter->layers;

    uint32 count = (uint32)layers.size();
    for (uint32 i = 0; i < count; ++i)
    {
        if (layers[i]->type == ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
        {
            CopyEmitter(layers[i]->innerEmitter->GetEmitter());
        }
        else
        {
            ProcessSprite(layers[i]->sprite);
            ProcessSprite(layers[i]->flowmap);
            ProcessSprite(layers[i]->noise);
            ProcessSprite(layers[i]->alphaRemapSprite);
        }
    }
}

void SceneSaver::ProcessSprite(DAVA::Sprite* sprite)
{
    using namespace DAVA;

    if (sprite == nullptr)
        return;

    FilePath psdPath = ReplaceInString(sprite->GetRelativePathname().GetAbsolutePathname(), "/Data/", "/DataSource/");
    psdPath.ReplaceExtension(".psd");
    sceneUtils.AddFile(psdPath);

    effectFolders.insert(psdPath.GetDirectory());
}

DAVA::Set<DAVA::FilePath> SceneSaver::EnumAlternativeEmittersFilepaths(const DAVA::FilePath& originalFilepath) const
{
    using namespace DAVA;

    Set<FilePath> qualityFilepaths;
    const ParticlesQualitySettings& particlesSettings = QualitySettingsSystem::Instance()->GetParticlesQualitySettings();

    for (const ParticlesQualitySettings::QualitySheet& qualitySheet : particlesSettings.GetQualitySheets())
    {
        FilePath alternativeFilepath;
        if (qualitySheet.Apply(originalFilepath, alternativeFilepath))
        {
            if (FileSystem::Instance()->Exists(alternativeFilepath))
            {
                qualityFilepaths.insert(alternativeFilepath);
            }
        }
    }

    qualityFilepaths.insert(originalFilepath);

    return qualityFilepaths;
}

void SceneSaver::CopyCustomColorTexture(DAVA::Scene* scene, const DAVA::FilePath& sceneFolder)
{
    using namespace DAVA;

    Entity* land = FindLandscapeEntity(scene);
    if (!land)
        return;

    KeyedArchive* customProps = GetCustomPropertiesArchieve(land);
    if (!customProps)
        return;

    String pathname = customProps->GetString(ResourceEditor::CUSTOM_COLOR_TEXTURE_PROP);
    if (pathname.empty())
        return;

    FilePath projectPath = ProjectManagerData::CreateProjectPathFromPath(sceneFolder);
    if (projectPath.IsEmpty())
    {
        Logger::Error("Can't copy custom colors texture (%s)", pathname.c_str());
        return;
    }

    FilePath texPathname = projectPath + pathname;
    sceneUtils.AddFile(texPathname);

    FilePath newTexPathname = sceneUtils.GetNewFilePath(texPathname);
    FilePath newProjectPathname = ProjectManagerData::CreateProjectPathFromPath(sceneUtils.dataFolder);
    if (newProjectPathname.IsEmpty())
    {
        Logger::Error("Can't save custom colors texture (%s)", pathname.c_str());
        return;
    }

    //save new path to custom colors texture
    customProps->SetString(ResourceEditor::CUSTOM_COLOR_TEXTURE_PROP, newTexPathname.GetRelativePathname(newProjectPathname));
}

void SceneSaver::ResaveYamlFilesRecursive(const DAVA::FilePath& folder) const
{
    using namespace DAVA;

    ScopedPtr<FileList> fileList(new FileList(folder));
    for (uint32 i = 0; i < fileList->GetCount(); ++i)
    {
        const FilePath& pathname = fileList->GetPathname(i);
        if (fileList->IsDirectory(i))
        {
            if (!fileList->IsNavigationDirectory(i))
            {
                ResaveYamlFilesRecursive(pathname);
            }
        }
        else if (pathname.IsEqualToExtension(".yaml"))
        {
            ScopedPtr<ParticleEmitter> emitter(new ParticleEmitter());
            emitter->LoadFromYaml(pathname);
            emitter->SaveToYaml(pathname);
        }
    }
}
