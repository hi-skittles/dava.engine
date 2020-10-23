#include "Utils/SceneExporter/SceneExporter.h"
#include "CommandLine/Private/SceneConsoleHelper.h"

#include "StringConstants.h"
#include "Scene/SceneHelper.h"
#include "Classes/Utils/FileSystemUtils/FileSystemTagGuard.h"

#include <AssetCache/AssetCacheClient.h>

#include <Engine/Engine.h>
#include <Engine/EngineContext.h>
#include <FileSystem/FilePath.h>
#include <FileSystem/FileList.h>
#include <FileSystem/FileSystem.h>
#include <Functional/Function.h>
#include <Logger/Logger.h>
#include <Particles/ParticleLayer.h>
#include <Particles/ParticleEmitter.h>
#include <Platform/Process.h>
#include <Render/GPUFamilyDescriptor.h>
#include <Render/TextureDescriptor.h>
#include <Render/Highlevel/Heightmap.h>
#include <Render/Highlevel/Landscape.h>
#include <Render/Image/ImageSystem.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/MotionComponent.h>
#include <Scene3D/Components/ParticleEffectComponent.h>
#include <Scene3D/Components/SlotComponent.h>
#include <Scene3D/Components/CustomPropertiesComponent.h>
#include <Scene3D/SceneFile/VersionInfo.h>
#include <Scene3D/Systems/SlotSystem.h>
#include <Utils/StringUtils.h>
#include <Utils/MD5.h>
#include <Entity/ComponentManager.h>
#include <Reflection/ReflectedTypeDB.h>

#include <algorithm>

namespace SceneExporterCache
{
const DAVA::uint32 EXPORTER_VERSION = 1;
const DAVA::uint32 LINKS_PARSER_VERSION = 2;
const DAVA::String LINKS_NAME = "links.txt";

void CalculateSceneKey(const DAVA::FilePath& scenePathname, const DAVA::String& sceneLink, DAVA::AssetCache::CacheItemKey& key, DAVA::uint32 optimize)
{
    using namespace DAVA;

    { //calculate digest for scene file
        MD5::MD5Digest fileDigest;
        MD5::ForFile(scenePathname, fileDigest);
        key.SetPrimaryKey(fileDigest);
    }

    { //calculate digest for params
        ScopedPtr<File> file(File::Create(scenePathname, File::OPEN | File::READ));

        MD5::MD5Digest sceneParamsDigest;
        String params = "ResourceEditor";
        params += Format("Pathname: %s", sceneLink.c_str());
        params += Format("FileSize: %d", (file) ? file->GetSize() : 0);
        params += Format("SceneFileVersion: %d", SCENE_FILE_CURRENT_VERSION);
        params += Format("ExporterVersion: %u", EXPORTER_VERSION);
        params += Format("LinksParserVersion: %u", LINKS_PARSER_VERSION);
        params += Format("Optimized: %u", optimize);
        for (int32 linkType = 0; linkType < SceneExporter::OBJECT_COUNT; ++linkType)
        {
            params += Format("LinkType: %d", linkType);
        }

        MD5::ForData(reinterpret_cast<const uint8*>(params.data()), static_cast<uint32>(params.size()), sceneParamsDigest);
        key.SetSecondaryKey(sceneParamsDigest);
    }
}

} //namespace SceneExporterCache

namespace SceneExporterDetails
{
bool SaveExportedObjects(const DAVA::FilePath& linkPathname, const DAVA::Vector<SceneExporter::ExportedObjectCollection>& exportedObjects)
{
    using namespace DAVA;

    DAVA::ScopedPtr<DAVA::File> linksFile(DAVA::File::Create(linkPathname, DAVA::File::CREATE | DAVA::File::WRITE));
    if (linksFile)
    {
        linksFile->WriteLine(Format("%d", static_cast<int32>(exportedObjects.size())));
        for (const SceneExporter::ExportedObjectCollection& collection : exportedObjects)
        {
            linksFile->WriteLine(Format("%d", static_cast<int32>(collection.size())));
            for (const SceneExporter::ExportedObject& object : collection)
            {
                linksFile->WriteLine(Format("%d,%s", object.type, object.relativePathname.c_str()));
            }
        }
        return true;
    }
    else
    {
        Logger::Error("Cannot open file with links: %s", linkPathname.GetAbsolutePathname().c_str());
        return false;
    }
}

std::pair<bool, DAVA::int32> ReadInt(DAVA::File* file)
{
    using namespace DAVA;

    String numStr = file->ReadLine();
    int32 numValue = 0;
    int32 numbers = sscanf(numStr.c_str(), "%d", &numValue);

    return std::pair<bool, DAVA::int32>((numbers == 1), numValue);
}

bool LoadExportedObjects(const DAVA::FilePath& linkPathname, DAVA::Vector<SceneExporter::ExportedObjectCollection>& exportedObjects)
{
    using namespace DAVA;

    ScopedPtr<File> linksFile(File::Create(linkPathname, File::OPEN | File::READ));
    if (linksFile)
    {
        std::pair<bool, DAVA::int32> collectionsCount = ReadInt(linksFile);
        if (collectionsCount.first == true && collectionsCount.second > 0)
        {
            int32 count = Min(collectionsCount.second, static_cast<int32>(SceneExporter::eExportedObjectType::OBJECT_COUNT));
            exportedObjects.resize(count);

            int32 collectionType = SceneExporter::eExportedObjectType::OBJECT_SCENE;
            for (SceneExporter::ExportedObjectCollection& collection : exportedObjects)
            {
                std::pair<bool, DAVA::int32> objectsCount = ReadInt(linksFile);
                if (objectsCount.first == true && objectsCount.second > 0)
                {
                    collection.reserve(objectsCount.second);
                    uint32 size = objectsCount.second;
                    while (size--)
                    {
                        if (linksFile->IsEof())
                        {
                            Logger::Warning("Reading of file stopped by EOF: %s", linkPathname.GetAbsolutePathname().c_str());
                            break;
                        }

                        String formatedString = linksFile->ReadLine();
                        if (formatedString.empty())
                        {
                            Logger::Warning("Reading of file stopped by empty string: %s", linkPathname.GetAbsolutePathname().c_str());
                            break;
                        }

                        auto dividerPos = formatedString.find(',', 1); //skip first number
                        DVASSERT(dividerPos != String::npos);

                        SceneExporter::eExportedObjectType type = static_cast<SceneExporter::eExportedObjectType>(atoi(formatedString.substr(0, dividerPos).c_str()));

                        if (collectionType == type)
                        {
                            collection.emplace_back(type, formatedString.substr(dividerPos + 1));
                        }
                        else
                        {
                            Logger::Error("Read wrong object at links file (%d instead of %d)", type, collectionType);
                        }
                    }
                }
                ++collectionType;
            }
        }
        else if (collectionsCount.second != 1)
        {
            Logger::Error("Cannot read size value from file: %s", linkPathname.GetAbsolutePathname().c_str());
            return false;
        }
    }
    else
    {
        Logger::Error("Cannot open file with links: %s", linkPathname.GetAbsolutePathname().c_str());
        return false;
    }

    return true;
}

inline bool IsEditorEntity(DAVA::Entity* entity)
{
    const DAVA::String::size_type pos = entity->GetName().find(ResourceEditor::EDITOR_BASE);
    return (DAVA::String::npos != pos);
}

void RemoveEditorCustomProperties(DAVA::Entity* entity)
{
    using namespace DAVA;

    //    "editor.dynamiclight.enable";
    //    "editor.donotremove";
    //
    //    "editor.referenceToOwner";
    //    "editor.isSolid";
    //    "editor.isLocked";
    //    "editor.designerName"
    //    "editor.modificationData"
    //    "editor.staticlight.enable";
    //    "editor.staticlight.used"
    //    "editor.staticlight.castshadows";
    //    "editor.staticlight.receiveshadows";
    //    "editor.staticlight.falloffcutoff"
    //    "editor.staticlight.falloffexponent"
    //    "editor.staticlight.shadowangle"
    //    "editor.staticlight.shadowsamples"
    //    "editor.staticlight.shadowradius"
    //    "editor.intensity"

    KeyedArchive* props = GetCustomPropertiesArchieve(entity);
    if (props)
    {
        const KeyedArchive::UnderlyingMap propsMap = props->GetArchieveData();

        for (auto& it : propsMap)
        {
            const String& key = it.first;

            if (key.find(ResourceEditor::EDITOR_BASE) == 0)
            {
                if ((key != ResourceEditor::EDITOR_DO_NOT_REMOVE) && (key != ResourceEditor::EDITOR_DYNAMIC_LIGHT_ENABLE))
                {
                    props->DeleteKey(key);
                }
            }
        }

        if (props->Count() == 0)
        {
            entity->RemoveComponent<CustomPropertiesComponent>();
        }
    }
}

void PrepareSceneToExport(DAVA::Scene* scene, bool removeCustomProperties)
{
    using namespace DAVA;

    ComponentManager* cm = GetEngineContext()->componentManager;

    const Vector<const Type*> sceneComponentsTypes = cm->GetRegisteredSceneComponents();

    //Remove scene nodes
    Vector<Entity*> entities;
    scene->GetChildNodes(entities);

    for (auto& entity : entities)
    {
        bool needRemove = IsEditorEntity(entity);
        if (needRemove)
        {
            //remove nodes from hierarchy
            DVASSERT(entity->GetParent() != nullptr);
            entity->GetParent()->RemoveNode(entity);
        }
        else
        {
            if (removeCustomProperties)
            {
                RemoveEditorCustomProperties(entity);
            }

            for (const Type* type : sceneComponentsTypes)
            { // remove RE specific components
                const ReflectedType* refType = ReflectedTypeDB::GetByType(type);

                DVASSERT(refType != nullptr);

                ReflectedMeta* meta = refType->GetStructure()->meta.get();

                if (meta == nullptr || meta->GetMeta<M::NonExportableComponent>() == nullptr)
                {
                    continue;
                }

                while (entity->GetComponentCount(type) > 0)
                {
                    entity->RemoveComponent(type);
                }
            }
        }
    }
}

void CollectHeightmapPathname(DAVA::Scene* scene, const DAVA::FilePath& dataSourceFolder, SceneExporter::ExportedObjectCollection& exportedObjects)
{
    DAVA::Landscape* landscape = DAVA::FindLandscape(scene);
    if (landscape != nullptr)
    {
        const DAVA::FilePath& heightmapPath = landscape->GetHeightmapPathname();
        exportedObjects.emplace_back(SceneExporter::OBJECT_HEIGHTMAP, heightmapPath.GetRelativePathname(dataSourceFolder));
    }
}

void CollectTextureDescriptors(DAVA::Scene* scene, const DAVA::FilePath& dataSourceFolder, SceneExporter::ExportedObjectCollection& exportedObjects)
{
    SceneHelper::TextureCollector collector(SceneHelper::TextureCollector::IncludeNullTextures);
    SceneHelper::EnumerateSceneTextures(scene, collector);

    exportedObjects.reserve(exportedObjects.size() + collector.GetTextures().size());
    for (const auto& scTex : collector.GetTextures())
    {
        const DAVA::FilePath& path = scTex.first;
        if (path.GetType() == DAVA::FilePath::PATH_IN_MEMORY)
        {
            continue;
        }

        DVASSERT(path.IsEmpty() == false);

        exportedObjects.emplace_back(SceneExporter::OBJECT_TEXTURE, path.GetRelativePathname(dataSourceFolder));
    }
}

void CollectParticleConfigs(DAVA::Scene* scene, const DAVA::FilePath& dataSourceFolder, SceneExporter::ExportedObjectCollection& exportedObjects)
{
    using namespace DAVA;

    Function<void(ParticleEmitter*)> collectSuperEmitters = [&collectSuperEmitters, &exportedObjects, &dataSourceFolder](ParticleEmitter* emitter)
    {
        if (emitter->configPath.IsEmpty() == false)
        {
            exportedObjects.emplace_back(SceneExporter::OBJECT_EMITTER_CONFIG, emitter->configPath.GetRelativePathname(dataSourceFolder));
        }

        for (ParticleLayer* layer : emitter->layers)
        {
            if (layer->type == ParticleLayer::TYPE_SUPEREMITTER_PARTICLES)
            {
                collectSuperEmitters(layer->innerEmitter->GetEmitter());
            }
        }
    };

    Vector<Entity*> effects;
    scene->GetChildEntitiesWithComponent(effects, Type::Instance<ParticleEffectComponent>());
    for (Entity* e : effects)
    {
        uint32 count = e->GetComponentCount<ParticleEffectComponent>();
        for (uint32 ic = 0; ic < count; ++ic)
        {
            ParticleEffectComponent* effectComponent = e->GetComponent<ParticleEffectComponent>(ic);
            uint32 emittersCount = effectComponent->GetEmittersCount();
            for (uint32 id = 0; id < emittersCount; ++id)
            {
                ParticleEmitterInstance* emitterInstance = effectComponent->GetEmitterInstance(id);
                ParticleEmitter* emitter = emitterInstance->GetEmitter();
                collectSuperEmitters(emitter);
            }
        }
    }
}

void CollectSlotConfigs(DAVA::Scene* scene, const DAVA::FilePath& dataSourceFolder, SceneExporter::ExportedObjectCollection& exportedObjects)
{
    using namespace DAVA;
    Vector<Entity*> slotHolders;
    scene->GetChildEntitiesWithComponent(slotHolders, Type::Instance<SlotComponent>());
    for (Entity* slotHolder : slotHolders)
    {
        uint32 slotComponentCount = slotHolder->GetComponentCount<SlotComponent>();
        for (uint32 i = 0; i < slotComponentCount; ++i)
        {
            SlotComponent* slotComponent = slotHolder->GetComponent<SlotComponent>(i);
            FilePath configPath = slotComponent->GetConfigFilePath();
            if (configPath.IsEmpty() == false)
            {
                String configRelativePath = configPath.GetRelativePathname(dataSourceFolder);
                exportedObjects.emplace_back(SceneExporter::eExportedObjectType::OBJECT_SLOT_CONFIG, configRelativePath);
            }
        }
    }
}

void CollectAnimationClips(DAVA::Scene* scene, const DAVA::FilePath& dataSourceFolder, SceneExporter::ExportedObjectCollection& exportedObjects)
{
    using namespace DAVA;
    Vector<Entity*> animationHolders;
    scene->GetChildEntitiesWithComponent(animationHolders, Type::Instance<MotionComponent>());
    for (Entity* entity : animationHolders)
    {
        uint32 componentCount = entity->GetComponentCount<MotionComponent>();
        for (uint32 i = 0; i < componentCount; ++i)
        {
            MotionComponent* motionComponent = entity->GetComponent<MotionComponent>(i);
            Vector<FilePath> dependencies = motionComponent->GetDependencies();
            for (const FilePath& fp : dependencies)
            {
                String relativePath = fp.GetRelativePathname(dataSourceFolder);
                exportedObjects.emplace_back(SceneExporter::eExportedObjectType::OBJECT_ANIMATION_CLIP, relativePath);
            }
        }
    }
}

void RemoveDuplicates(SceneExporter::ExportedObjectCollection& exportedObjects)
{
    std::sort(exportedObjects.begin(), exportedObjects.end());
    exportedObjects.erase(std::unique(exportedObjects.begin(), exportedObjects.end()), exportedObjects.end());
}

} //namespace SceneExporterDetails

SceneExporter::~SceneExporter() = default;

void SceneExporter::SetExportingParams(const SceneExporter::Params& exportingParams_)
{
    exportingParams = exportingParams_;

    DVASSERT(exportingParams.outputs.empty() == false);
    DVASSERT(exportingParams.dataSourceFolder.IsDirectoryPathname());

    for (const Params::Output& output : exportingParams.outputs)
    {
        DVASSERT(output.dataFolder.IsDirectoryPathname());
        DVASSERT(output.exportForGPUs.empty() == false);
    }
}

void SceneExporter::SetCacheClient(DAVA::AssetCacheClient* cacheClient_, DAVA::String machineName, DAVA::String runDate, DAVA::String comment)
{
    cacheClient = cacheClient_;
    cacheItemDescription.machineName = machineName;
    cacheItemDescription.creationDate = runDate;
    cacheItemDescription.comment = comment;
}

bool SceneExporter::ExportSceneObject(const ExportedObject& sceneObject)
{
    using namespace DAVA;

    DVASSERT(sceneObject.type == eExportedObjectType::OBJECT_SCENE);

    Logger::Info("Exporting of %s", sceneObject.relativePathname.c_str());

    FileSystem* fileSystem = GetEngineContext()->fileSystem;

    FilePath scenePathname = exportingParams.dataSourceFolder + sceneObject.relativePathname;
    FilePath rootFolder = fileSystem->GetTempDirectoryPath() + "/Export/";
    FilePath tempDataFolder = rootFolder + "Data/";

    FilePath outScenePathname = tempDataFolder + sceneObject.relativePathname;
    FilePath outSceneFolder = outScenePathname.GetDirectory();
    fileSystem->CreateDirectory(outSceneFolder, true);

    FilePath linksPathname(outSceneFolder + SceneExporterCache::LINKS_NAME);

    SCOPE_EXIT
    { //delete temporary file
        fileSystem->DeleteDirectoryFiles(rootFolder, true);
        fileSystem->DeleteDirectory(rootFolder, true);
    };

    auto copyScene = [this, &outScenePathname, &sceneObject]()
    {
        bool filesCopied = true;
        for (const Params::Output& output : exportingParams.outputs)
        {
            filesCopied = CopyFile(outScenePathname, output.dataFolder + sceneObject.relativePathname) && filesCopied;
        }

        return filesCopied;
    };

    AssetCache::CacheItemKey cacheKey;
    if (cacheClient != nullptr && cacheClient->IsConnected())
    { //request Scene from cache
        SceneExporterCache::CalculateSceneKey(scenePathname, sceneObject.relativePathname, cacheKey, static_cast<uint32>(exportingParams.optimizeOnExport));

        AssetCache::CachedItemValue retrievedData;
        AssetCache::Error requested = cacheClient->RequestFromCacheSynchronously(cacheKey, &retrievedData);
        if (requested == AssetCache::Error::NO_ERRORS)
        {
            bool exportedToFolder = retrievedData.ExportToFolder(outSceneFolder);

            bool filesCopied = copyScene();
            bool objectsLoaded = SceneExporterDetails::LoadExportedObjects(linksPathname, objectsToExport);
            return exportedToFolder && objectsLoaded && filesCopied;
        }
        else
        {
            Logger::Info("%s - failed to retrieve from cache(%s)", scenePathname.GetAbsolutePathname().c_str(), AssetCache::ErrorToString(requested).c_str());
        }
    }

    bool sceneExported = false;
    Vector<ExportedObjectCollection> externalLinks;
    externalLinks.resize(eExportedObjectType::OBJECT_COUNT);

    { //has no scene in cache or using of cache is disabled. Export scene directly
        sceneExported = ExportSceneFileInternal(scenePathname, outScenePathname, externalLinks);
        sceneExported = copyScene() && sceneExported;

        //add links to whole list of files
        for (int32 i = 0; i < eExportedObjectType::OBJECT_COUNT; ++i)
        {
            objectsToExport[i].insert(objectsToExport[i].end(), externalLinks[i].begin(), externalLinks[i].end());
        }
    }

    if (cacheClient != nullptr && cacheClient->IsConnected())
    { //place exported scene into cache
        SceneExporterDetails::SaveExportedObjects(linksPathname, externalLinks);

        AssetCache::CachedItemValue value;
        value.Add(outScenePathname);
        value.Add(linksPathname);
        value.UpdateValidationData();
        value.SetDescription(cacheItemDescription);

        AssetCache::Error added = cacheClient->AddToCacheSynchronously(cacheKey, value);
        if (added == AssetCache::Error::NO_ERRORS)
        {
            Logger::Info("%s - added to cache", scenePathname.GetAbsolutePathname().c_str());
        }
        else
        {
            Logger::Info("%s - failed to add to cache (%s)", scenePathname.GetAbsolutePathname().c_str(), AssetCache::ErrorToString(added).c_str());
        }
    }

    return sceneExported;
}

bool SceneExporter::ExportSceneFileInternal(const DAVA::FilePath& scenePathname, const DAVA::FilePath& outScenePathname, DAVA::Vector<SceneExporter::ExportedObjectCollection>& exportedObjects)
{
    bool sceneExported = false;

    //Load scene from *.sc2
    DAVA::ScopedPtr<DAVA::Scene> scene(new DAVA::Scene());
    if (DAVA::SceneFileV2::ERROR_NO_ERROR == scene->LoadScene(scenePathname))
    {
        sceneExported = ExportScene(scene, scenePathname, outScenePathname, exportedObjects);
    }
    else
    {
        DAVA::Logger::Error("[SceneExporterV2::%s] Can't open file %s", __FUNCTION__, scenePathname.GetAbsolutePathname().c_str());
    }

    SceneConsoleHelper::FlushRHI();
    return sceneExported;
}

namespace TextureDescriptorValidator
{
bool IsImageValidForFormat(const DAVA::ImageInfo& info, const DAVA::PixelFormat format)
{
    if (info.width != info.height && (format == DAVA::FORMAT_PVR2 || format == DAVA::FORMAT_PVR4))
    {
        return false;
    }

    return true;
}

bool IsImageSizeValidForTextures(const DAVA::ImageInfo& info)
{
    return ((info.width >= DAVA::Texture::MINIMAL_WIDTH) && (info.height >= DAVA::Texture::MINIMAL_HEIGHT));
}
}

namespace SceneExporterLocal
{
void CompressNotActualTexture(const DAVA::eGPUFamily gpu, DAVA::TextureConverter::eConvertQuality quality, DAVA::TextureDescriptor& descriptor)
{
    DVASSERT(DAVA::GPUFamilyDescriptor::IsGPUForDevice(gpu));

    if (descriptor.IsCompressedTextureActual(gpu) == false)
    {
        DAVA::Logger::Warning("Need recompress texture: %s", descriptor.GetSourceTexturePathname().GetAbsolutePathname().c_str());
        DAVA::TextureConverter::ConvertTexture(descriptor, gpu, true, quality);
    }
}

void CollectSourceImageInfo(const DAVA::TextureDescriptor& descriptor, DAVA::Vector<DAVA::ImageInfo>& sourceImageInfos)
{
    DAVA::Vector<DAVA::FilePath> imagePathnames;
    if (descriptor.IsCubeMap())
    {
        descriptor.GetFacePathnames(imagePathnames);
    }
    else
    {
        imagePathnames.push_back(descriptor.GetSourceTexturePathname());
    }

    sourceImageInfos.reserve(imagePathnames.size());
    for (const DAVA::FilePath& path : imagePathnames)
    {
        sourceImageInfos.push_back(DAVA::ImageSystem::GetImageInfo(path));
    }
}
}

bool SceneExporter::ExportTextureObjectTagged(const ExportedObject& object)
{
    using namespace DAVA;

    DAVA::FileSystem* fs = DAVA::GetEngineContext()->fileSystem;

    FilePath taggedPathname;
    String nonTaggedBasename;
    String taggedBasename;

    bool needExportTagged = exportingParams.filenamesTag.empty() == false;
    if (needExportTagged)
    { // try to find tagget texture file
        taggedPathname = exportingParams.dataSourceFolder + object.relativePathname;
        nonTaggedBasename = taggedPathname.GetBasename();
        taggedBasename = nonTaggedBasename + exportingParams.filenamesTag;
        taggedPathname.ReplaceBasename(taggedBasename);
        needExportTagged = fs->Exists(taggedPathname);
    }

    if (needExportTagged)
    { // export tagged texure
        ExportedObject taggedObject = object;
        taggedObject.relativePathname = taggedPathname.GetRelativePathname(exportingParams.dataSourceFolder);

        //real export of tagged file
        bool exported = ExportTextureObject(taggedObject);

        //restore filenames to non-tagged style
        for (const Params::Output& output : exportingParams.outputs)
        {
            FilePath texPath = output.dataFolder + object.relativePathname;

            ScopedPtr<FileList> fileList(new FileList(texPath.GetDirectory(), false));
            for (uint32 index = 0; index < fileList->GetCount(); ++index)
            {
                const FilePath& path = fileList->GetPathname(index);
                if (path.IsDirectoryPathname() == false)
                {
                    String name = path.GetBasename();
                    if (name.find(taggedBasename) != String::npos)
                    {
                        String nontaggedFileName = path.GetAbsolutePathname();
                        String::size_type pos = nontaggedFileName.find(taggedBasename);

                        nontaggedFileName.replace(pos, taggedBasename.length(), nonTaggedBasename);

                        exported = fs->MoveFile(path, nontaggedFileName, true) && exported;
                    }
                }
            }
        }

        return exported;
    }

    // export non-tagged file
    return ExportTextureObject(object);
}

bool SceneExporter::ExportTextureObject(const ExportedObject& object)
{
    using namespace DAVA;
    DVASSERT(object.type == eExportedObjectType::OBJECT_TEXTURE);

    FilePath descriptorPathname = exportingParams.dataSourceFolder + object.relativePathname;

    bool texturesExported = true;
    for (const Params::Output& output : exportingParams.outputs)
    {
        std::unique_ptr<TextureDescriptor> descriptor(TextureDescriptor::CreateFromFile(descriptorPathname));
        if (!descriptor)
        {
            Logger::Error("Can't create descriptor for pathname %s", descriptorPathname.GetStringValue().c_str());
            return false;
        }

        texturesExported = ExportDescriptor(*descriptor, output);
        if (texturesExported)
        {
            if (output.exportForGPUs.size() == 1)
            {
                descriptor->Export(output.dataFolder + object.relativePathname, output.exportForGPUs[0]);
            }
            else
            {
                descriptor->Save(output.dataFolder + object.relativePathname);
            }
        }
    }

    return texturesExported;
}

bool SceneExporter::ExportDescriptor(DAVA::TextureDescriptor& descriptor, const Params::Output& output)
{
    using namespace DAVA;

    Set<eGPUFamily> exportFailed;
    bool shouldSplitHDTextures = (output.useHDTextures && descriptor.dataSettings.GetGenerateMipMaps());

    { // compress images

        Vector<ImageInfo> sourceImageInfos;
        SceneExporterLocal::CollectSourceImageInfo(descriptor, sourceImageInfos);

        for (eGPUFamily gpu : output.exportForGPUs)
        {
            if (gpu == eGPUFamily::GPU_ORIGIN)
            {
                ImageFormat targetFormat = static_cast<ImageFormat>(descriptor.compression[gpu].imageFormat);
                if (shouldSplitHDTextures && (targetFormat != ImageFormat::IMAGE_FORMAT_DDS && targetFormat != IMAGE_FORMAT_PVR))
                {
                    Logger::Error("HD texture will not be created for exported %s for GPU 'origin'", descriptor.pathname.GetStringValue().c_str());
                    exportFailed.insert(gpu);
                    continue;
                }
            }
            else if (GPUFamilyDescriptor::IsGPUForDevice(gpu))
            {
                PixelFormat format = descriptor.GetPixelFormatForGPU(gpu);
                if (format == PixelFormat::FORMAT_INVALID)
                {
                    Logger::Error("Texture %s has not pixel format specified for GPU %s", descriptor.pathname.GetStringValue().c_str(), GlobalEnumMap<eGPUFamily>::Instance()->ToString(gpu));
                    exportFailed.insert(gpu);
                    continue;
                }

                for (const ImageInfo& imgInfo : sourceImageInfos)
                {
                    if (!TextureDescriptorValidator::IsImageValidForFormat(imgInfo, format))
                    {
                        Logger::Error("Can't export non-square texture %s into compression format %s",
                                      descriptor.pathname.GetAbsolutePathname().c_str(), GlobalEnumMap<PixelFormat>::Instance()->ToString(format));
                        exportFailed.insert(gpu);
                        break;
                    }
                    else if (!TextureDescriptorValidator::IsImageSizeValidForTextures(imgInfo))
                    {
                        Logger::Error("Can't export small sized texture %s into compression format %s",
                                      descriptor.pathname.GetAbsolutePathname().c_str(), GlobalEnumMap<PixelFormat>::Instance()->ToString(format));
                        exportFailed.insert(gpu);
                        break;
                    }
                }

                if (exportFailed.count(gpu) == 0)
                {
                    SceneExporterLocal::CompressNotActualTexture(gpu, output.quality, descriptor);
                }
            }
            else if (gpu != eGPUFamily::GPU_ORIGIN)
            {
                Logger::Error("Has no code for GPU %d (%s)", gpu, GlobalEnumMap<eGPUFamily>::Instance()->ToString(gpu));
                exportFailed.insert(gpu);
            }
        }
    }

    //modify descriptors in data
    descriptor.dataSettings.SetSeparateHDTextures(shouldSplitHDTextures);

    { // copy or separate images
        for (eGPUFamily gpu : output.exportForGPUs)
        {
            if (exportFailed.count(gpu) != 0)
            { // found errors on compress step
                continue;
            }

            bool copied = true;

            if (gpu == eGPUFamily::GPU_ORIGIN)
            {
                if (descriptor.IsCubeMap())
                {
                    Vector<FilePath> faceNames;
                    descriptor.GetFacePathnames(faceNames);
                    for (const auto& faceName : faceNames)
                    {
                        if (faceName.IsEmpty())
                            continue;

                        copied = CopyFileToOutput(faceName, output) && copied;
                    }
                }
                else
                {
                    copied = CopyFileToOutput(descriptor.GetSourceTexturePathname(), output);
                }
            }
            else if (GPUFamilyDescriptor::IsGPUForDevice(gpu))
            {
                if (shouldSplitHDTextures)
                {
                    copied = SplitCompressedFile(descriptor, gpu, output);
                }
                else
                {
                    FilePath compressedName = descriptor.CreateMultiMipPathnameForGPU(gpu);
                    copied = (compressedName.IsEmpty() ? false : CopyFileToOutput(compressedName, output));
                }
            }

            if (!copied)
            {
                exportFailed.insert(gpu);
            }
        }
    }

    return (exportFailed.size() < output.exportForGPUs.size());
}

bool SceneExporter::SplitCompressedFile(const DAVA::TextureDescriptor& descriptor, DAVA::eGPUFamily gpu, const Params::Output& output) const
{
    using namespace DAVA;

    Vector<Image*> loadedImages;
    SCOPE_EXIT
    {
        for (Image* image : loadedImages)
        {
            SafeRelease(image);
        }
    };

    FilePath compressedTexturePath = descriptor.CreateMultiMipPathnameForGPU(gpu);

    eErrorCode loadError = ImageSystem::Load(compressedTexturePath, loadedImages);
    if (loadError != eErrorCode::SUCCESS || loadedImages.empty())
    {
        Logger::Error("Can't load %s", compressedTexturePath.GetStringValue().c_str());
        return false;
    }

    PixelFormat targetFormat = descriptor.GetPixelFormatForGPU(gpu);
    DVASSERT(targetFormat == loadedImages[0]->format);

    uint32 mipmapsCount = static_cast<uint32>(loadedImages.size());
    bool isCubemap = loadedImages[0]->cubeFaceID != Texture::INVALID_CUBEMAP_FACE;
    if (isCubemap)
    {
        uint32 firstFace = loadedImages[0]->cubeFaceID;
        mipmapsCount = static_cast<uint32>(count_if(loadedImages.begin(), loadedImages.end(), [&firstFace](const DAVA::Image* img) { return img->cubeFaceID == firstFace; }));
    }

    Vector<FilePath> pathnamesForGPU;
    descriptor.CreateLoadPathnamesForGPU(gpu, pathnamesForGPU);
    uint32 outTexturesCount = static_cast<uint32>(pathnamesForGPU.size());

    if (mipmapsCount < outTexturesCount)
    {
        Logger::Error("Can't split HD level for %s", compressedTexturePath.GetStringValue().c_str());
        return false;
    }

    auto createOutPathname = [&output, this](const FilePath& pathname)
    {
        String fileLink = pathname.GetRelativePathname(exportingParams.dataSourceFolder);
        return output.dataFolder + fileLink;
    };

    enum class eSavingParam
    {
        SaveOneMip,
        SaveRemainingMips
    };
    auto saveImages = [&](const FilePath& path, uint32 mip, eSavingParam param)
    {
        if (isCubemap)
        {
            Vector<Vector<Image*>> savedImages;
            for (uint32 face = 0; face < Texture::CUBE_FACE_COUNT; ++face)
            {
                Vector<Image*> faceMips;

                for (Image* loadedImage : loadedImages)
                {
                    if (loadedImage->cubeFaceID == face && loadedImage->mipmapLevel >= static_cast<DAVA::uint32>(mip))
                    {
                        faceMips.push_back(loadedImage);
                        if (param == eSavingParam::SaveOneMip)
                            break;
                    }
                }

                if (!faceMips.empty())
                {
                    savedImages.resize(savedImages.size() + 1);
                    savedImages.back().swap(faceMips);
                }
            }

            eErrorCode saveError = ImageSystem::SaveAsCubeMap(path, savedImages, targetFormat);
            if (saveError != eErrorCode::SUCCESS)
            {
                Logger::Error("Can't save %s", path.GetStringValue().c_str());
                return false;
            }
        }
        else
        {
            DVASSERT(mip < loadedImages.size());
            Vector<Image*> savedImages;
            if (param == eSavingParam::SaveOneMip)
            {
                savedImages.push_back(loadedImages[mip]);
            }
            else
            {
                savedImages.assign(loadedImages.begin() + mip, loadedImages.end());
            }

            eErrorCode saveError = ImageSystem::Save(path, savedImages, targetFormat);
            if (saveError != eErrorCode::SUCCESS)
            {
                Logger::Error("Can't save %s", path.GetStringValue().c_str());
                return false;
            }
        }

        return true;
    };

    // save hd mips, each in separate file
    uint32 savedMip = 0;
    uint32 singleMipCount = outTexturesCount - 1;
    for (uint32 mip = 0; mip < singleMipCount; ++mip)
    {
        if (loadedImages[mip]->width > Texture::MINIMAL_WIDTH && loadedImages[mip]->height > Texture::MINIMAL_HEIGHT)
        {
            bool saved = saveImages(createOutPathname(pathnamesForGPU[mip]), savedMip++, eSavingParam::SaveOneMip);
            if (!saved)
            {
                return false;
            }
        }
        else
        {
            break;
        }
    }

    // save remaining mips, all in single file
    size_t lastIndex = pathnamesForGPU.size() - 1;
    return saveImages(createOutPathname(pathnamesForGPU[lastIndex]), savedMip, eSavingParam::SaveRemainingMips);
}

bool SceneExporter::CopyFile(const DAVA::FilePath& fromPath, const DAVA::FilePath& toPath) const
{
    bool retCopy = true;
    if (fromPath != toPath)
    {
        using namespace DAVA;

        FileSystem* fileSystem = GetEngineContext()->fileSystem;
        DAVA::FilePath dstFolder = toPath.GetDirectory();
        if (fileSystem->Exists(dstFolder) == false)
        {
            fileSystem->CreateDirectory(dstFolder, true);
        }
        retCopy = fileSystem->CopyFile(fromPath, toPath, true);
        if (retCopy == false)
        {
            Logger::Error("Can't copy %s to %s", fromPath.GetStringValue().c_str(), toPath.GetStringValue().c_str());
        }
    }
    return retCopy;
}

bool SceneExporter::ExportScene(DAVA::Scene* scene, const DAVA::FilePath& scenePathname, const DAVA::FilePath& outScenePathname, DAVA::Vector<ExportedObjectCollection>& exportedObjects)
{
    DVASSERT(exportedObjects.size() == SceneExporter::OBJECT_COUNT);

    using namespace DAVA;

    CollectObjects(scene, exportedObjects);

    // save scene to new place
    FilePath tempSceneName = FilePath::CreateWithNewExtension(scenePathname, ".exported.sc2");
    scene->SaveScene(tempSceneName, exportingParams.optimizeOnExport);

    FileSystem* fileSystem = GetEngineContext()->fileSystem;
    bool moved = fileSystem->MoveFile(tempSceneName, outScenePathname, true);
    if (!moved)
    {
        Logger::Error("Can't move file %s into %s", tempSceneName.GetStringValue().c_str(), outScenePathname.GetStringValue().c_str());
        fileSystem->DeleteFile(tempSceneName);
        return false;
    }

    return true;
}

bool SceneExporter::ExportObjects(const ExportedObjectCollection& exportedObjects)
{
    using namespace DAVA;

    Array<Function<bool(const ExportedObject&)>, OBJECT_COUNT> exporters =
    { {
    MakeFunction(this, &SceneExporter::ExportSceneObject), // scene
    MakeFunction(this, &SceneExporter::ExportTextureObjectTagged), //texture
    MakeFunction(this, &SceneExporter::CopyObject), // heightmap
    MakeFunction(this, &SceneExporter::CopyObject), // emitter config
    MakeFunction(this, &SceneExporter::ExportSlotObject), //slot config
    MakeFunction(this, &SceneExporter::CopyObject), //anim clip
    } };

    // divide objects into different collections
    bool exportIsOk = PrepareData(exportedObjects);

    //export scenes only. Add textures, heightmaps to objectsToExport
    const ExportedObjectCollection& scenes = objectsToExport[eExportedObjectType::OBJECT_SCENE];
    for (uint32 i = 0; i < static_cast<uint32>(scenes.size()); ++i)
    {
        const ExportedObject& sceneObj = scenes[i];
        CreateFoldersStructure(sceneObj);

        DAVA::FilePath fullScenePath(exportingParams.dataSourceFolder + sceneObj.relativePathname);
        if (alreadyExportedScenes.count(fullScenePath) == 0)
        {
            alreadyExportedScenes.insert(fullScenePath);
            exportIsOk = ExportSceneObject(scenes[i]) && exportIsOk;
        }
    }

    { //export only slot objects
        SceneExporterDetails::RemoveDuplicates(objectsToExport[OBJECT_SLOT_CONFIG]);
        for (const ExportedObject& slot : objectsToExport[OBJECT_SLOT_CONFIG])
        {
            DAVA::Vector<DAVA::SlotSystem::ItemsCache::Item> items;
            { // load tagged config
                FileSystemTagGuard tagGuard(exportingParams.filenamesTag);
                items = SlotSystem::ParseConfig(exportingParams.dataSourceFolder + slot.relativePathname);
            }

            for (const DAVA::SlotSystem::ItemsCache::Item& item : items)
            {
                ExportedObject sceneObject(OBJECT_SCENE, item.scenePath.GetRelativePathname(exportingParams.dataSourceFolder));
                CreateFoldersStructure(sceneObject);

                //create folders structure
                exportIsOk = ExportSceneObject(sceneObject) & exportIsOk;
            }
        }
    }

    //export objects
    for (int32 i = eExportedObjectType::OBJECT_SCENE + 1; i < eExportedObjectType::OBJECT_COUNT; ++i)
    {
        SceneExporterDetails::RemoveDuplicates(objectsToExport[i]);
        for (const ExportedObject& object : objectsToExport[i])
        {
            DVASSERT(object.type != eExportedObjectType::OBJECT_SCENE);
            CreateFoldersStructure(object);
            exportIsOk = exporters[i](object) && exportIsOk;
        }
    }

    return exportIsOk;
}

bool SceneExporter::PrepareData(const ExportedObjectCollection& exportedObjects)
{
    objectsToExport.clear();
    objectsToExport.resize(eExportedObjectType::OBJECT_COUNT);

    bool dataIsValid = true;
    for (const ExportedObject& object : exportedObjects)
    {
        if (object.type != OBJECT_NONE && object.type < OBJECT_COUNT)
        {
            objectsToExport[object.type].emplace_back(object.type, object.relativePathname);
        }
        else
        {
            DAVA::Logger::Error("Found wrong path: %s", object.relativePathname.c_str());
            dataIsValid = false;
        }
    }

    return dataIsValid;
}

void SceneExporter::CreateFoldersStructure(const ExportedObject& object)
{
    using namespace DAVA;

    String folder = object.relativePathname;
    const String::size_type slashpos = folder.rfind("/");
    if (slashpos != String::npos)
    {
        folder = folder.substr(0, slashpos + 1);
    }
    else
    {
        folder = "/";
    }

    if (cachedFoldersForCreation.count(folder) == 0)
    {
        cachedFoldersForCreation.insert(folder);
        FileSystem* fileSystem = GetEngineContext()->fileSystem;
        for (const Params::Output& output : exportingParams.outputs)
        {
            fileSystem->CreateDirectory(output.dataFolder + folder, true);
        }
    }
}

bool SceneExporter::ExportSlotObject(const ExportedObject& object)
{
    using namespace DAVA;

    FilePath fromPath = exportingParams.dataSourceFolder + object.relativePathname;
    if (exportingParams.filenamesTag.empty() == false)
    {
        FilePath fromPathTagged = fromPath;
        fromPathTagged.ReplaceBasename(fromPath.GetBasename() + exportingParams.filenamesTag);

        FileSystem* fs = GetEngineContext()->fileSystem;
        if (fs->Exists(fromPathTagged))
        {
            fromPath = fromPathTagged;
        }
    }

    bool filesCopied = true;
    for (const Params::Output& output : exportingParams.outputs)
    {
        filesCopied = CopyFile(fromPath, output.dataFolder + object.relativePathname) && filesCopied;
    }

    return filesCopied;
}

bool SceneExporter::CopyFileToOutput(const DAVA::FilePath& fromPath, const Params::Output& output) const
{
    using namespace DAVA;

    String relativePathname = fromPath.GetRelativePathname(exportingParams.dataSourceFolder);
    return CopyFile(fromPath, output.dataFolder + relativePathname);
}

bool SceneExporter::CopyObject(const ExportedObject& object)
{
    using namespace DAVA;

    bool filesCopied = true;

    FilePath fromPath = exportingParams.dataSourceFolder + object.relativePathname;
    for (const Params::Output& output : exportingParams.outputs)
    {
        filesCopied = CopyFile(fromPath, output.dataFolder + object.relativePathname) && filesCopied;
    }

    return filesCopied;
}

const DAVA::Array<SceneExporter::ExportedObjectDesc, SceneExporter::OBJECT_COUNT>& SceneExporter::GetExportedObjectsDescriptions()
{
    static DAVA::Array<SceneExporter::ExportedObjectDesc, SceneExporter::OBJECT_COUNT> desc =
    {
      ExportedObjectDesc(OBJECT_SCENE, DAVA::Vector<DAVA::String>{ ".sc2" }),
      ExportedObjectDesc(OBJECT_TEXTURE, DAVA::Vector<DAVA::String>{ ".tex" }),
      ExportedObjectDesc(OBJECT_HEIGHTMAP, DAVA::Vector<DAVA::String>{ DAVA::Heightmap::FileExtension() }),
      ExportedObjectDesc(OBJECT_EMITTER_CONFIG, DAVA::Vector<DAVA::String>{ ".yaml" }),
      ExportedObjectDesc(OBJECT_SLOT_CONFIG, DAVA::Vector<DAVA::String>{ ".yaml", ".xml" }),
      ExportedObjectDesc(OBJECT_ANIMATION_CLIP, DAVA::Vector<DAVA::String>{ ".anim" })
    };

    return desc;
}

void SceneExporter::CollectObjects(DAVA::Scene* scene, DAVA::Vector<ExportedObjectCollection>& exportedObjects)
{
    SceneExporterDetails::PrepareSceneToExport(scene, exportingParams.optimizeOnExport);

    SceneExporterDetails::CollectHeightmapPathname(scene, exportingParams.dataSourceFolder, exportedObjects[eExportedObjectType::OBJECT_HEIGHTMAP]); //must be first
    SceneExporterDetails::CollectTextureDescriptors(scene, exportingParams.dataSourceFolder, exportedObjects[eExportedObjectType::OBJECT_TEXTURE]);
    SceneExporterDetails::CollectParticleConfigs(scene, exportingParams.dataSourceFolder, exportedObjects[eExportedObjectType::OBJECT_EMITTER_CONFIG]);
    SceneExporterDetails::CollectSlotConfigs(scene, exportingParams.dataSourceFolder, exportedObjects[eExportedObjectType::OBJECT_SLOT_CONFIG]);
    SceneExporterDetails::CollectAnimationClips(scene, exportingParams.dataSourceFolder, exportedObjects[eExportedObjectType::OBJECT_ANIMATION_CLIP]);
}

bool operator==(const SceneExporter::ExportedObject& left, const SceneExporter::ExportedObject& right)
{
    return left.relativePathname == right.relativePathname && left.type == right.type;
}

bool operator<(const SceneExporter::ExportedObject& left, const SceneExporter::ExportedObject& right)
{
    if (left.relativePathname == right.relativePathname)
    {
        return left.type < right.type;
    }

    return left.relativePathname < right.relativePathname;
}
