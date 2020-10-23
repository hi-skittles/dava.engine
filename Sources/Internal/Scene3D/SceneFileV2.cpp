#include "Scene3D/SceneFileV2.h"
#include "Scene3D/Entity.h"
#include "Render/Texture.h"
#include "Scene3D/PathManip.h"
#include "Render/Highlevel/Camera.h"
#include "Render/Highlevel/Mesh.h"
#include "Render/3D/MeshUtils.h"
#include "Render/Material/NMaterialNames.h"

#include "Scene3D/Systems/TransformSystem.h"
#include "Scene3D/Lod/LodComponent.h"
#include "Scene3D/Components/TransformComponent.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Systems/EventSystem.h"
#include "Scene3D/Components/CameraComponent.h"
#include "Scene3D/Components/ParticleEffectComponent.h"
#include "Scene3D/Components/LightComponent.h"
#include "Scene3D/Components/SwitchComponent.h"
#include "Scene3D/Components/UserComponent.h"

#include "Logger/Logger.h"
#include "Utils/StringFormat.h"
#include "FileSystem/FileSystem.h"
#include "Base/ObjectFactory.h"
#include "Base/TemplateHelpers.h"
#include "Render/Highlevel/Landscape.h"
#include "Render/Highlevel/ShadowVolume.h"
#include "Render/Highlevel/SpriteObject.h"
#include "Render/Highlevel/RenderObject.h"

#include "Render/Material/NMaterial.h"
#include "Scene3D/Components/CustomPropertiesComponent.h"
#include "Scene3D/Components/RenderComponent.h"
#include "Scene3D/Components/ComponentHelpers.h"

#include "Scene3D/Scene.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"

#include "Scene3D/Converters/SpeedTreeConverter.h"

#include "Job/JobManager.h"

#include <functional>
#include "Engine/EngineContext.h"
#include "Engine/Engine.h"

namespace DAVA
{
SceneFileV2::SceneFileV2() //-V730 no need to init descriptor
{
    isDebugLogEnabled = false;
    isSaveForGame = false;
    lastError = ERROR_NO_ERROR;

    serializationContext.SetDebugLogEnabled(isDebugLogEnabled);
    serializationContext.SetLastError(lastError);
}

SceneFileV2::~SceneFileV2()
{
}

void SceneFileV2::EnableSaveForGame(bool _isSaveForGame)
{
    isSaveForGame = _isSaveForGame;
}

void SceneFileV2::EnableDebugLog(bool _isDebugLogEnabled)
{
    isDebugLogEnabled = _isDebugLogEnabled;
    serializationContext.SetDebugLogEnabled(isDebugLogEnabled);
}

bool SceneFileV2::DebugLogEnabled()
{
    return isDebugLogEnabled;
}

void SceneFileV2::SetError(eError error)
{
    lastError = error;
}

SceneFileV2::eError SceneFileV2::GetError() const
{
    return lastError;
}

SceneFileV2::eError SceneFileV2::SaveScene(const FilePath& filename, Scene* scene, eFileType fileType)
{
    ScopedPtr<File> file(File::Create(filename, File::CREATE | File::WRITE));
    if (!file)
    {
        Logger::Error("SceneFileV2::SaveScene failed to create file: %s", filename.GetAbsolutePathname().c_str());
        SetError(ERROR_FAILED_TO_CREATE_FILE);
        return GetError();
    }

    // save header
    header.signature[0] = 'S';
    header.signature[1] = 'F';
    header.signature[2] = 'V';
    header.signature[3] = '2';

    header.version = GetEngineContext()->versionInfo->GetCurrentVersion().version;
    header.nodeCount = scene->GetChildrenCount();

    if (scene->GetGlobalMaterial())
    {
        header.nodeCount++;
    }

    descriptor.size = sizeof(descriptor.fileType); // + sizeof(descriptor.additionalField1) + sizeof(descriptor.additionalField1) +....
    descriptor.fileType = fileType;

    serializationContext.SetRootNodePath(filename);
    serializationContext.SetScenePath(FilePath(filename.GetDirectory()));
    serializationContext.SetVersion(header.version);
    serializationContext.SetScene(scene);

    if (sizeof(Header) != file->Write(&header, sizeof(Header)))
    {
        Logger::Error("SceneFileV2::SaveScene failed to write header file: %s", filename.GetAbsolutePathname().c_str());
        SetError(ERROR_FILE_WRITE_ERROR);
        return GetError();
    }

    // save version tags
    {
        ScopedPtr<KeyedArchive> tagsArchive(new KeyedArchive());
        const VersionInfo::TagsMap& tags = GetEngineContext()->versionInfo->GetCurrentVersion().tags;
        for (VersionInfo::TagsMap::const_iterator it = tags.begin(); it != tags.end(); ++it)
        {
            tagsArchive->SetUInt32(it->first, it->second);
        }
        if (!tagsArchive->Save(file))
        {
            Logger::Error("SceneFileV2::SaveScene failed to write tags file: %s", filename.GetAbsolutePathname().c_str());
            SetError(ERROR_FILE_WRITE_ERROR);
            return GetError();
        }
    }

    if (!WriteDescriptor(file, descriptor))
    {
        SetError(ERROR_FILE_WRITE_ERROR);
        return GetError();
    }

    // save data objects
    if (isDebugLogEnabled)
    {
        Logger::FrameworkDebug("+ save data objects");
        Logger::FrameworkDebug("- save file path: %s", filename.GetDirectory().GetAbsolutePathname().c_str());
    }

    if (isSaveForGame)
    {
        scene->OptimizeBeforeExport();
    }

    Set<DataNode*> nodes;
    scene->GetDataNodes(nodes);

    uint32 serializableNodesCount = 0;
    uint64 maxDataNodeID = 0;

    // compute maxid for datanodes
    for (auto node : nodes)
    {
        // TODO: now one datanode can be used in multiple scenes,
        // but datanote->scene points only on single scene. This should be
        // discussed and fixed in the future.
        if (node->GetScene() == scene && node->GetNodeID() > maxDataNodeID)
        {
            maxDataNodeID = node->GetNodeID();
        }
    }

    // assign datanode id-s and
    // count serializable nodes
    for (auto node : nodes)
    {
        if (IsDataNodeSerializable(node))
        {
            // TODO: if datanode is from another scene, it should be saved with newly
            // generated datanode-id. Unfortunately this ID will be generated on every scene save,
            // because we don't change scene pointer in datanode->scene.
            // This should be discussed and fixed in the future.
            serializableNodesCount++;
            if (node->GetScene() != scene || node->GetNodeID() == DataNode::INVALID_ID)
            {
                node->SetNodeID(++maxDataNodeID);
            }
        }
    }

    // do we need to save globalmaterial?
    NMaterial* globalMaterial = scene->GetGlobalMaterial();
    if (nullptr != globalMaterial)
    {
        if (nodes.count(globalMaterial) > 0)
        {
            // remove global material from set,
            // as it should be saved exclusively
            // on the top of data nodes
            nodes.erase(globalMaterial);
        }
        else
        {
            serializableNodesCount++;
        }
    }

    // save datanodes count
    if (sizeof(uint32) != file->Write(&serializableNodesCount, sizeof(uint32)))
    {
        Logger::Error("SceneFileV2::SaveScene failed to write datanodes count file: %s", filename.GetAbsolutePathname().c_str());
        SetError(ERROR_FILE_WRITE_ERROR);
        return GetError();
    }

    // save global material on top of datanodes
    if (nullptr != globalMaterial)
    {
        if (globalMaterial->GetNodeID() == DataNode::INVALID_ID)
        {
            globalMaterial->SetNodeID(++maxDataNodeID);
        }
        if (!SaveDataNode(globalMaterial, file))
        {
            Logger::Error("SceneFileV2::SaveScene failed to write global materials file: %s", filename.GetAbsolutePathname().c_str());
            SetError(ERROR_FILE_WRITE_ERROR);
            return GetError();
        }
    }

    // sort in ascending ID order
    Set<DataNode*, std::function<bool(DataNode*, DataNode*)>> orderedNodes(nodes.begin(), nodes.end(),
                                                                           [](DataNode* a, DataNode* b) { return a->GetNodeID() < b->GetNodeID(); });

    // save the rest of datanodes
    for (auto node : orderedNodes)
    {
        if (IsDataNodeSerializable(node))
        {
            if (!SaveDataNode(node, file))
            {
                Logger::Error("SceneFileV2::SaveScene failed to write datanode file: %s", filename.GetAbsolutePathname().c_str());
                SetError(ERROR_FILE_WRITE_ERROR);
                return GetError();
            }
        }
    }

    // save global material settings
    if (nullptr != globalMaterial)
    {
        ScopedPtr<KeyedArchive> archive(new KeyedArchive());
        const uint64 globalMaterialId = scene->GetGlobalMaterial()->GetNodeID();

        archive->SetString("##name", "GlobalMaterial");
        archive->SetUInt64("globalMaterialId", globalMaterialId);
        if (!archive->Save(file))
        {
            Logger::Error("SceneFileV2::SaveScene failed to write global material settings file: %s", filename.GetAbsolutePathname().c_str());
            SetError(ERROR_FILE_WRITE_ERROR);
            return GetError();
        }
    }

    // save hierarchy
    if (isDebugLogEnabled)
    {
        Logger::FrameworkDebug("+ save hierarchy");
    }

    for (int ci = 0; ci < scene->GetChildrenCount(); ++ci)
    {
        if (!SaveHierarchy(scene->GetChild(ci), file, 1))
        {
            Logger::Error("SceneFileV2::SaveScene failed to save hierarchy file: %s", filename.GetAbsolutePathname().c_str());
            return GetError();
        }
    }

    if (!file->Flush())
    {
        SetError(ERROR_FILE_WRITE_ERROR);
        return GetError();
    }

    return GetError();
}

bool SceneFileV2::ReadHeader(SceneFileV2::Header& _header, File* file)
{
    DVASSERT(file);

    const uint32 result = file->Read(&_header, sizeof(Header));
    if (result != sizeof(Header))
    {
        Logger::Error("SceneFileV2::ReadHeader failed. Read file return %d.", result);
        return false;
    }

    if ((_header.signature[0] != 'S')
        || (_header.signature[1] != 'F')
        || (_header.signature[2] != 'V')
        || (_header.signature[3] != '2'))
    {
        Logger::Error("SceneFileV2::LoadSceneVersion header is wrong");
        return false;
    }

    return true;
}

bool SceneFileV2::ReadVersionTags(VersionInfo::SceneVersion& _version, File* file)
{
    DVASSERT(file);

    bool loaded = false;
    if (_version.version >= 14)
    {
        ScopedPtr<KeyedArchive> tagsArchive(new KeyedArchive());
        loaded = tagsArchive->Load(file);

        if (loaded)
        {
            const KeyedArchive::UnderlyingMap& keyedTags = tagsArchive->GetArchieveData();
            for (KeyedArchive::UnderlyingMap::const_iterator it = keyedTags.begin(); it != keyedTags.end(); it++)
            {
                const String& tag = it->first;
                const uint32 ver = it->second->AsUInt32();
                _version.tags.insert(VersionInfo::TagsMap::value_type(tag, ver));
            }
        }
    }
    else
    {
        loaded = true;
    }

    return loaded;
}

VersionInfo::SceneVersion SceneFileV2::LoadSceneVersion(const FilePath& filename)
{
    const ScopedPtr<File> file(File::Create(filename, File::OPEN | File::READ));
    if (!file)
    {
        Logger::Error("SceneFileV2::LoadSceneVersion failed to open file: %s", filename.GetAbsolutePathname().c_str());
        return VersionInfo::SceneVersion();
    }

    VersionInfo::SceneVersion version;

    Header header;
    const bool headerValid = ReadHeader(header, file);
    if (headerValid)
    {
        version.version = header.version;
        const bool versionValid = ReadVersionTags(version, file);
        if (!versionValid)
        {
            version = VersionInfo::SceneVersion();
        }
    }
    else
    {
        Logger::Error("SceneFileV2::LoadSceneVersion  header is wrong in file: %s", filename.GetAbsolutePathname().c_str());
        return version;
    }

    return version;
}

SceneFileV2::eError SceneFileV2::LoadScene(const FilePath& filename, Scene* scene)
{
    ScopedPtr<File> file(File::Create(filename, File::OPEN | File::READ));
    if (!file)
    {
        Logger::Error("SceneFileV2::LoadScene failed to open file: %s", filename.GetAbsolutePathname().c_str());
        SetError(ERROR_FAILED_TO_CREATE_FILE);
        return GetError();
    }

    const bool headerValid = ReadHeader(header, file);

    if (!headerValid)
    {
        Logger::Error("SceneFileV2::LoadScene: scene header is not valid in file: %s", filename.GetAbsolutePathname().c_str());
        SetError(ERROR_VERSION_IS_TOO_OLD);
        return GetError();
    }

    if (header.version < SCENE_FILE_MINIMAL_SUPPORTED_VERSION)
    {
        Logger::Error("SceneFileV2::LoadScene: scene version %d is too old. Minimal supported version is %d. File: %s", header.version, SCENE_FILE_MINIMAL_SUPPORTED_VERSION, filename.GetAbsolutePathname().c_str());
        SetError(ERROR_VERSION_IS_TOO_OLD);
        return GetError();
    }

    // load version tags
    scene->version.version = header.version;
    const bool versionValid = ReadVersionTags(scene->version, file);
    if (!versionValid)
    {
        Logger::Error("SceneFileV2::LoadScene version tags are wrong in file: %s", filename.GetAbsolutePathname().c_str());
        SetError(ERROR_VERSION_TAGS_INVALID);
        return GetError();
    }

    if (header.version >= 10)
    {
        const bool resultRead = ReadDescriptor(file, descriptor);
        if (!resultRead)
        {
            Logger::Error("SceneFileV2::LoadScene ReadDescriptor failed in file: %s", filename.GetAbsolutePathname().c_str());
            SetError(ERROR_FILE_READ_ERROR);
            return GetError();
        }
    }

    VersionInfo::eStatus status = GetEngineContext()->versionInfo->TestVersion(scene->version);
    switch (status)
    {
    case VersionInfo::COMPATIBLE:
    {
        const String tags = GetEngineContext()->versionInfo->UnsupportedTagsMessage(scene->version);
        Logger::Warning("SceneFileV2::LoadScene scene was saved with older version of framework. Saving scene will broke compatibility. Missed tags: %s", tags.c_str());
    }
    break;
    case VersionInfo::INVALID:
    {
        const String tags = GetEngineContext()->versionInfo->NoncompatibleTagsMessage(scene->version);
        Logger::Error("SceneFileV2::LoadScene scene(%d) is incompatible with current version(%d). Wrong tags: %s", header.version, SCENE_FILE_CURRENT_VERSION, tags.c_str());
        SetError(ERROR_VERSION_TAGS_INVALID);
        return GetError();
    }
    default:
        break;
    }

    serializationContext.SetRootNodePath(filename);
    serializationContext.SetScenePath(filename.GetDirectory());
    serializationContext.SetVersion(header.version);
    serializationContext.SetScene(scene);
    serializationContext.SetDefaultMaterialQuality(NMaterialQualityName::DEFAULT_QUALITY_NAME);

    if (isDebugLogEnabled)
        Logger::FrameworkDebug("+ load data objects");

    if (header.version >= 2)
    {
        int32 dataNodeCount = 0;
        uint32 result = file->Read(&dataNodeCount, sizeof(int32));
        if (result != sizeof(int32))
        {
            Logger::Error("SceneFileV2::LoadScene read(%d) dataNodeCount failed in file: %s", result, filename.GetAbsolutePathname().c_str());
            SetError(ERROR_FILE_READ_ERROR);
            return GetError();
        }

        for (int k = 0; k < dataNodeCount; ++k)
        {
            const bool nodeLoaded = LoadDataNode(scene, nullptr, file);
            if (!nodeLoaded)
            {
                Logger::Error("SceneFileV2::LoadScene LoadDataNode failed in file: %s", filename.GetAbsolutePathname().c_str());
                SetError(ERROR_FILE_READ_ERROR);
                return GetError();
            }
        }

        NMaterial* globalMaterial = nullptr;

        if (header.nodeCount > 0)
        {
            // try to load global material
            uint32 filePos = static_cast<uint32>(file->GetPos());
            ScopedPtr<KeyedArchive> archive(new KeyedArchive());
            const bool loaded = archive->Load(file);
            if (!loaded)
            {
                Logger::Error("SceneFileV2::LoadScene load KeyedArchive with global material failed in file: %s", filename.GetAbsolutePathname().c_str());
                SetError(ERROR_FILE_READ_ERROR);
                return GetError();
            }

            String name = archive->GetString("##name");
            if (name == "GlobalMaterial")
            {
                uint64 globalMaterialId = archive->GetUInt64("globalMaterialId");
                globalMaterial = static_cast<NMaterial*>(serializationContext.GetDataBlock(globalMaterialId));
                serializationContext.SetGlobalMaterialKey(globalMaterialId);
                --header.nodeCount;
            }
            else
            {
                const bool res = file->Seek(filePos, File::SEEK_FROM_START);
                if (!res)
                {
                    Logger::Error("SceneFileV2::LoadScene seek failed in file: %s", filename.GetAbsolutePathname().c_str());
                    SetError(ERROR_FILE_READ_ERROR);
                    return GetError();
                }
            }
        }

        serializationContext.ResolveMaterialBindings();

        ApplyFogQuality(globalMaterial);
        scene->SetGlobalMaterial(globalMaterial);
    }

    if (isDebugLogEnabled)
    {
        Logger::FrameworkDebug("+ load hierarchy");
    }

    scene->children.reserve(header.nodeCount);
    for (int ci = 0; ci < header.nodeCount; ++ci)
    {
        const bool loaded = LoadHierarchy(0, scene, file, 1);
        if (!loaded)
        {
            Logger::Error("SceneFileV2::LoadScene LoadHierarchy failed in file: %s", filename.GetAbsolutePathname().c_str());
            SetError(ERROR_FILE_READ_ERROR);
            return GetError();
        }
    }

    UpdatePolygonGroupRequestedFormatRecursively(scene);
    const bool contextLoaded = serializationContext.LoadPolygonGroupData(file);
    if (!contextLoaded)
    {
        Logger::Error("SceneFileV2::LoadScene LoadPolygonGroupData failed in file: %s", filename.GetAbsolutePathname().c_str());
        SetError(ERROR_FILE_READ_ERROR);
        return GetError();
    }
    OptimizeScene(scene);

    if (serializationContext.GetVersion() < LODSYSTEM2)
    {
        FixLodForLodsystem2(scene);
    }

    if (GetError() == ERROR_NO_ERROR)
    {
        scene->SceneDidLoaded();
        scene->OnSceneReady(scene);
    }

    return GetError();
}

void SceneFileV2::ApplyFogQuality(NMaterial* globalMaterial)
{
    QualitySettingsSystem* qss = QualitySettingsSystem::Instance();
    bool removeVertexFog = qss->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_DISABLE_FOG);
    bool removeHalfSpaceFog = qss->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_DISABLE_FOG_HALF_SPACE);

    if (globalMaterial != nullptr)
    {
        if (qss->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_DISABLE_FOG_ATMOSPHERE_ATTENUATION))
            globalMaterial->AddFlag(NMaterialFlagName::FLAG_FOG_ATMOSPHERE_NO_ATTENUATION, 1);
        if (qss->IsOptionEnabled(QualitySettingsSystem::QUALITY_OPTION_DISABLE_FOG_ATMOSPHERE_SCATTERING))
            globalMaterial->AddFlag(NMaterialFlagName::FLAG_FOG_ATMOSPHERE_NO_SCATTERING, 1);
    }

    if (removeVertexFog || removeHalfSpaceFog)
    {
        Vector<NMaterial*> materials;
        serializationContext.GetDataNodes(materials);

        for (NMaterial* material : materials)
        {
            if (removeVertexFog && material->HasLocalFlag(NMaterialFlagName::FLAG_VERTEXFOG))
                material->RemoveFlag(NMaterialFlagName::FLAG_VERTEXFOG);

            if (removeHalfSpaceFog && material->HasLocalFlag(NMaterialFlagName::FLAG_FOG_HALFSPACE))
                material->RemoveFlag(NMaterialFlagName::FLAG_FOG_HALFSPACE);
        }
    }
}

SceneArchive* SceneFileV2::LoadSceneArchive(const FilePath& filename)
{
    SceneArchive* res = nullptr;
    ScopedPtr<File> file(File::Create(filename, File::OPEN | File::READ));
    if (!file)
    {
        Logger::Error("SceneFileV2::LoadScene failed to open file: %s", filename.GetAbsolutePathname().c_str());
        return res;
    }

    const bool headerValid = ReadHeader(header, file);

    if (!headerValid)
    {
        Logger::Error("SceneFileV2::LoadScene: scene header is not valid");
        return res;
    }

    if (header.version < SCENE_FILE_MINIMAL_SUPPORTED_VERSION)
    {
        Logger::Error("SceneFileV2::LoadScene: scene version %d is too old. Minimal supported version is %d", header.version, SCENE_FILE_MINIMAL_SUPPORTED_VERSION);
        return res;
    }

    // load version tags
    VersionInfo::SceneVersion version;
    version.version = header.version;
    const bool versionValid = ReadVersionTags(version, file);
    if (!versionValid)
    {
        Logger::Error("SceneFileV2::LoadScene version tags are wrong");
        return res;
    }

    if (header.version >= 10)
    {
        const bool resultRead = ReadDescriptor(file, descriptor);
        if (!resultRead)
        {
            Logger::Error("SceneFileV2::LoadScene ReadDescriptor failed in file: %s", filename.GetAbsolutePathname().c_str());
            return res;
        }
    }

    VersionInfo::eStatus status = GetEngineContext()->versionInfo->TestVersion(version);
    switch (status)
    {
    case VersionInfo::COMPATIBLE:
    {
        const String tags = GetEngineContext()->versionInfo->UnsupportedTagsMessage(version);
        Logger::Warning("SceneFileV2::LoadScene scene was saved with older version of framework. Saving scene will broke compatibility. Missed tags: %s", tags.c_str());
    }
    break;
    case VersionInfo::INVALID:
    {
        const String tags = GetEngineContext()->versionInfo->NoncompatibleTagsMessage(version);
        Logger::Error("SceneFileV2::LoadScene scene is incompatible with current version. Wrong tags: %s", tags.c_str());
        return res;
    }
    default:
        break;
    }

    res = new SceneArchive();

    if (header.version >= 2)
    {
        int32 dataNodeCount = 0;
        uint32 result = file->Read(&dataNodeCount, sizeof(int32));
        if (result != sizeof(int32))
        {
            Logger::Error("SceneFileV2::LoadScene read file failed, file: %s", file->GetFilename().GetAbsolutePathname().c_str());
            SafeRelease(res);
            return nullptr;
        }
        bool loadedNodes = true;
        for (int k = 0; k < dataNodeCount; ++k)
        {
            KeyedArchive* archive = new KeyedArchive();
            loadedNodes &= archive->Load(file);
            if (!loadedNodes)
            {
                SafeRelease(archive);
                Logger::Error("SceneFileV2::LoadScene load KeyedArchive in node:%d failed, in file %s", k, file->GetFilename().GetAbsolutePathname().c_str());
                break;
            }
            res->dataNodes.push_back(archive);
        }
        if (!loadedNodes)
        {
            for (KeyedArchive* iter : res->dataNodes)
            {
                SafeRelease(iter);
            }
            SafeRelease(res);
            return nullptr;
        }
    }

    bool loadNodes = true;
    res->children.reserve(header.nodeCount);
    for (int ci = 0; ci < header.nodeCount; ++ci)
    {
        SceneArchive::SceneArchiveHierarchyNode* child = new SceneArchive::SceneArchiveHierarchyNode();
        loadNodes &= child->LoadHierarchy(file);
        if (!loadNodes)
        {
            SafeRelease(child);
            Logger::Error("SceneFileV2::LoadScene LoadHierarchy failed in node:%d, in file %s", ci, file->GetFilename().GetAbsolutePathname().c_str());
            break;
        }
        res->children.push_back(child);
    }
    if (!loadNodes)
    {
        for (SceneArchive::SceneArchiveHierarchyNode* iter : res->children)
        {
            SafeRelease(iter);
        }
        for (KeyedArchive* iter : res->dataNodes)
        {
            SafeRelease(iter);
        }
        SafeRelease(res);
        return nullptr;
    }
    return res;
}

bool SceneFileV2::WriteDescriptor(File* file, const Descriptor& descriptor)
{
    if (sizeof(descriptor.size) != file->Write(&descriptor.size, sizeof(descriptor.size)))
    {
        return false;
    }

    if (sizeof(descriptor.fileType) != file->Write(&descriptor.fileType, sizeof(descriptor.fileType)))
    {
        return false;
    }

    return true;
}

bool SceneFileV2::ReadDescriptor(File* file, /*out*/ Descriptor& descriptor)
{
    uint32 result = file->Read(&descriptor.size, sizeof(descriptor.size));
    if (result != sizeof(descriptor.size))
    {
        return false;
    }
    DVASSERT(descriptor.size >= sizeof(descriptor.fileType));

    result = file->Read(&descriptor.fileType, sizeof(descriptor.fileType));
    if (result != sizeof(descriptor.size))
    {
        return false;
    }

    if (descriptor.size > sizeof(descriptor.fileType))
    {
        //skip extra data probably added by future versions
        const bool seekResult = file->Seek(descriptor.size - sizeof(descriptor.fileType), File::SEEK_FROM_CURRENT);
        if (!seekResult)
        {
            return false;
        }
    }
    return true;
}

bool SceneFileV2::SaveDataNode(DataNode* node, File* file)
{
    KeyedArchive* archive = new KeyedArchive();

    node->Save(archive, &serializationContext);
    if (!archive->Save(file))
    {
        SafeRelease(archive);
        return false;
    }

    SafeRelease(archive);
    return true;
}

bool SceneFileV2::LoadDataNode(Scene* scene, DataNode* parent, File* file)
{
    bool loaded = true;
    uint32 currFilePos = static_cast<uint32>(file->GetPos());
    ScopedPtr<KeyedArchive> archive(new KeyedArchive());
    loaded &= archive->Load(file);

    String name = archive->GetString("##name");
    DataNode* node = dynamic_cast<DataNode*>(ObjectFactory::Instance()->New<BaseObject>(name));

    if (node)
    {
        if (node->GetClassName() == "DataNode")
        {
            SafeRelease(node);
            return false;
        }
        node->SetScene(scene);

        if (isDebugLogEnabled)
        {
            String arcName = archive->GetString("name");
            Logger::FrameworkDebug("- %s(%s)", arcName.c_str(), node->GetClassName().c_str());
        }
        node->Load(archive, &serializationContext);
        AddToNodeMap(node);

        if (name == "PolygonGroup")
        {
            serializationContext.AddLoadedPolygonGroup(static_cast<PolygonGroup*>(node), currFilePos);
        }

        int32 childrenCount = archive->GetInt32("#childrenCount", 0);
        DVASSERT(0 == childrenCount && "We don't support hierarchical dataNodes load.");

        SafeRelease(node);
    }
    return loaded;
}

bool SceneFileV2::SaveDataHierarchy(DataNode* node, File* /*file*/, int32 /*level*/)
{
    ScopedPtr<KeyedArchive> archive(new KeyedArchive());
    node->Save(archive, &serializationContext);
    return true;
}

void SceneFileV2::LoadDataHierarchy(Scene* scene, DataNode* root, File* file, int32 level)
{
    ScopedPtr<KeyedArchive> archive(new KeyedArchive());
    archive->Load(file);

    // DataNode * node = dynamic_cast<DataNode*>(BaseObject::LoadFromArchive(archive));

    String name = archive->GetString("##name");
    DataNode* node = dynamic_cast<DataNode*>(ObjectFactory::Instance()->New<BaseObject>(name));

    if (node)
    {
        if (node->GetClassName() == "DataNode")
        {
            SafeRelease(node);
            node = SafeRetain(root); // retain root here because we release it at the end
        }

        node->SetScene(scene);

        if (isDebugLogEnabled)
        {
            String arcName = archive->GetString("name");
            Logger::FrameworkDebug("%s %s(%s)", GetIndentString('-', level).c_str(), arcName.c_str(), node->GetClassName().c_str());
        }

        node->Load(archive, &serializationContext);
        AddToNodeMap(node);

        int32 childrenCount = archive->GetInt32("#childrenCount", 0);
        DVASSERT(0 == childrenCount && "We don't support hierarchical dataNodes load.");

        SafeRelease(node);
    }
}

void SceneFileV2::AddToNodeMap(DataNode* node)
{
    uint64 id = node->GetNodeID();
    serializationContext.SetDataBlock(id, SafeRetain(node));
}

bool SceneFileV2::SaveHierarchy(Entity* node, File* file, int32 level)
{
    ScopedPtr<KeyedArchive> archive(new KeyedArchive());
    if (isDebugLogEnabled)
        Logger::FrameworkDebug("%s %s(%s) %d", GetIndentString('-', level).c_str(), node->GetName().c_str(), node->GetClassName().c_str(), node->GetChildrenCount());
    node->Save(archive, &serializationContext);

    archive->SetInt32("#childrenCount", node->GetChildrenCount());

    if (!archive->Save(file))
    {
        return false;
    }

    for (int ci = 0; ci < node->GetChildrenCount(); ++ci)
    {
        Entity* child = node->GetChild(ci);
        SaveHierarchy(child, file, level + 1);
    }
    return true;
}

bool SceneFileV2::LoadHierarchy(Scene* scene, Entity* parent, File* file, int32 level)
{
    bool resultLoad = true;
    bool keepUnusedQualityEntities = QualitySettingsSystem::Instance()->GetKeepUnusedEntities();
    ScopedPtr<KeyedArchive> archive(new KeyedArchive());
    resultLoad &= archive->Load(file);

    String name = archive->GetString("##name");

    bool removeChildren = false;
    bool skipNode = false;

    Entity* node = nullptr;
    if (name == "LandscapeNode")
    {
        node = LoadLandscape(scene, archive);
    }
    else if (name == "Camera")
    {
        node = LoadCamera(scene, archive);
    }
    else if ((name == "LightNode")) // || (name == "EditorLightNode"))
    {
        node = LoadLight(scene, archive);
        removeChildren = true;
    }
    else if (name == "SceneNode")
    {
        node = LoadEntity(scene, archive);
    }
    else
    {
        BaseObject* obj = ObjectFactory::Instance()->New<BaseObject>(name);
        node = dynamic_cast<Entity*>(obj);
        if (node)
        {
            node->SetScene(scene);
            node->Load(archive, &serializationContext);
        }
        else //in case if editor class is loading in non-editor sprsoject
        {
            SafeRelease(obj);
            node = new Entity();
            skipNode = true;
        }
    }

    if (nullptr != node)
    {
        if (isDebugLogEnabled)
        {
            String arcName = archive->GetString("name");
            Logger::FrameworkDebug("%s %s(%s)", GetIndentString('-', level).c_str(), arcName.c_str(), node->GetClassName().c_str());
        }

        if (!skipNode && (keepUnusedQualityEntities || QualitySettingsSystem::Instance()->IsQualityVisible(node)))
        {
            parent->AddNode(node);
        }

        int32 childrenCount = archive->GetInt32("#childrenCount", 0);
        node->children.reserve(childrenCount);
        for (int ci = 0; ci < childrenCount; ++ci)
        {
            resultLoad &= LoadHierarchy(scene, node, file, level + 1);
        }

        if (removeChildren && childrenCount)
        {
            node->RemoveAllChildren();
        }

        ParticleEffectComponent* effect = node->GetComponent<ParticleEffectComponent>();
        if (effect && (effect->loadedVersion == 0))
            effect->CollapseOldEffect(&serializationContext);

        SafeRelease(node);
    }
    return resultLoad;
}

void SceneFileV2::FixLodForLodsystem2(Entity* entity)
{
    LodComponent* lod = GetLodComponent(entity);
    RenderObject* ro = GetRenderObject(entity);
    ParticleEffectComponent* effect = GetParticleEffectComponent(entity);
    if (lod && ro && !effect)
    {
        int32 maxLod = Max(ro->GetMaxLodIndex(), 0);
        for (int32 i = maxLod; i < LodComponent::MAX_LOD_LAYERS; ++i)
        {
            lod->SetLodLayerDistance(i, std::numeric_limits<float32>::max());
        }
    }

    int32 size = entity->GetChildrenCount();
    for (int32 i = 0; i < size; ++i)
    {
        Entity* child = entity->GetChild(i);
        FixLodForLodsystem2(child);
    }
}

Entity* SceneFileV2::LoadEntity(Scene* scene, KeyedArchive* archive)
{
    Entity* entity = new Entity();
    entity->SetScene(scene);
    entity->Load(archive, &serializationContext);
    return entity;
}

Entity* SceneFileV2::LoadLandscape(Scene* scene, KeyedArchive* archive)
{
    Entity* landscapeEntity = LoadEntity(scene, archive);

    Landscape* landscapeRenderObject = new Landscape();
    landscapeRenderObject->Load(archive, &serializationContext);

    landscapeEntity->AddComponent(new RenderComponent(landscapeRenderObject));
    SafeRelease(landscapeRenderObject);

    return landscapeEntity;
}

Entity* SceneFileV2::LoadCamera(Scene* scene, KeyedArchive* archive)
{
    Entity* cameraEntity = LoadEntity(scene, archive);

    Camera* cameraObject = new Camera();
    cameraObject->LoadObject(archive);

    cameraEntity->AddComponent(new CameraComponent(cameraObject));
    SafeRelease(cameraObject);

    return cameraEntity;
}

Entity* SceneFileV2::LoadLight(Scene* scene, KeyedArchive* archive)
{
    Entity* lightEntity = LoadEntity(scene, archive);

    bool isDynamic = true;
    KeyedArchive* props = GetCustomPropertiesArchieve(lightEntity);
    if (props)
    {
        isDynamic = props->GetBool("editor.dynamiclight.enable", true);
    }

    Light* light = new Light();
    light->Load(archive, &serializationContext);
    light->SetDynamic(isDynamic);

    lightEntity->AddComponent(new LightComponent(light));
    SafeRelease(light);

    return lightEntity;
}

bool SceneFileV2::RemoveEmptyHierarchy(Entity* currentNode)
{
    for (int32 c = 0; c < currentNode->GetChildrenCount(); ++c)
    {
        Entity* childNode = currentNode->GetChild(c);

        bool dec = RemoveEmptyHierarchy(childNode);
        if (dec)
            c--;
    }

    KeyedArchive* customProperties = GetCustomPropertiesArchieve(currentNode);
    bool doNotRemove = customProperties && customProperties->IsKeyExists("editor.donotremove");
    if (doNotRemove == true)
    {
        return false;
    }

    if (currentNode->GetChildrenCount() == 1)
    {
        uint32 allowed_comp_count = 0;
        if (nullptr != currentNode->GetComponent<TransformComponent>())
        {
            allowed_comp_count++;
        }

        if (nullptr != currentNode->GetComponent<CustomPropertiesComponent>())
        {
            allowed_comp_count++;
        }

        if (currentNode->GetComponentCount() > allowed_comp_count)
        {
            return false;
        }

        TransformComponent* transform = currentNode->GetComponent<TransformComponent>();
        if (transform->GetLocalMatrix() == Matrix4::IDENTITY)
        {
            Entity* parent = currentNode->GetParent();

            if (parent)
            {
                if (header.version < OLD_LODS_SCENE_VERSION && GetLodComponent(parent))
                {
                    return false;
                }

                Entity* childNode = SafeRetain(currentNode->GetChild(0));

                FastName currentName = currentNode->GetName();
                KeyedArchive* currentProperties = GetCustomPropertiesArchieve(currentNode);

                //Logger::FrameworkDebug("remove node: %s %p", currentNode->GetName().c_str(), currentNode);
                parent->InsertBeforeNode(childNode, currentNode);

                //MEGA kostyl
                if (!childNode->GetComponent<ParticleEffectComponent>()) //do not rename effects
                {
                    childNode->SetName(currentName);
                }
                //merge custom properties

                if (currentProperties)
                {
                    KeyedArchive* newProperties = GetOrCreateCustomProperties(childNode)->GetArchive();
                    const KeyedArchive::UnderlyingMap& oldMap = currentProperties->GetArchieveData();
                    KeyedArchive::UnderlyingMap::const_iterator itEnd = oldMap.end();
                    for (KeyedArchive::UnderlyingMap::const_iterator it = oldMap.begin(); it != itEnd; ++it)
                    {
                        newProperties->SetVariant(it->first, *it->second);
                    }
                }

                //VI: remove node after copying its properties since properties become invalid after node removal
                parent->RemoveNode(currentNode);

                removedNodeCount++;
                SafeRelease(childNode);

                return true;
            }
            //RemoveEmptyHierarchy(childNode);
        }
    }
    return false;
}

void SceneFileV2::RemoveDeprecatedMaterialFlags(Entity* node)
{
    RenderObject* ro = GetRenderObject(node);
    if (ro)
    {
        static const FastName FLAG_TILED_DECAL = FastName("TILED_DECAL");
        static const FastName FLAG_FOG_EXP = FastName("FOG_EXP");

        uint32 batchCount = ro->GetRenderBatchCount();
        for (uint32 ri = 0; ri < batchCount; ++ri)
        {
            RenderBatch* batch = ro->GetRenderBatch(ri);
            NMaterial* material = batch->GetMaterial();

            while (material)
            {
                if (material->HasLocalFlag(FLAG_FOG_EXP))
                {
                    material->RemoveFlag(FLAG_FOG_EXP);
                }
                if (material->HasLocalFlag(FLAG_TILED_DECAL))
                {
                    material->AddFlag(NMaterialFlagName::FLAG_TILED_DECAL_MASK, material->GetLocalFlagValue(FLAG_TILED_DECAL));
                    material->RemoveFlag(FLAG_TILED_DECAL);
                }

                material = material->GetParent();
            }
        }
    }

    uint32 size = node->GetChildrenCount();
    for (uint32 i = 0; i < size; ++i)
    {
        Entity* child = node->GetChild(i);
        RemoveDeprecatedMaterialFlags(child);
    }
}

void SceneFileV2::ConvertAlphatestValueMaterials(Entity* node)
{
    static const float32 alphatestThresholdValue = .3f;
    static const Array<FastName, 7> alphatestValueMaterials =
    {
      FastName("~res:/Materials/NormalizedBlinnPhongPerPixel.Alphatest.material"),
      FastName("~res:/Materials/NormalizedBlinnPhongPerPixel.Alphatest.Alphablend.material"),
      FastName("~res:/Materials/NormalizedBlinnPhongPerPixelFast.Alphatest.material"),
      FastName("~res:/Materials/NormalizedBlinnPhongPerVertex.Alphatest.material"),
      FastName("~res:/Materials/NormalizedBlinnPhongPerVertex.Alphatest.Alphablend.material"),
      FastName("~res:/Materials/NormalizedBlinnPhongAllQualities.Alphatest.material"),
      FastName("~res:/Materials/NormalizedBlinnPhongAllQualities.Alphatest.Alphablend.material"),
    };

    RenderObject* ro = GetRenderObject(node);
    if (ro)
    {
        uint32 batchCount = ro->GetRenderBatchCount();
        for (uint32 ri = 0; ri < batchCount; ++ri)
        {
            RenderBatch* batch = ro->GetRenderBatch(ri);
            NMaterial* material = batch->GetMaterial();

            while (material)
            {
                for (auto& alphatestTemplate : alphatestValueMaterials)
                {
                    if (alphatestTemplate == material->GetLocalFXName())
                    {
                        if (!material->HasLocalProperty(NMaterialParamName::PARAM_ALPHATEST_THRESHOLD))
                            material->AddProperty(NMaterialParamName::PARAM_ALPHATEST_THRESHOLD, &alphatestThresholdValue, rhi::ShaderProp::TYPE_FLOAT1);
                    }
                }
                material = material->GetParent();
            }
        }
    }

    uint32 size = node->GetChildrenCount();
    for (uint32 i = 0; i < size; ++i)
    {
        Entity* child = node->GetChild(i);
        ConvertAlphatestValueMaterials(child);
    }
}

void SceneFileV2::RebuildTangentSpace(Entity* entity)
{
    static int32 prerequiredFormat = EVF_TANGENT | EVF_NORMAL;
    RenderObject* ro = GetRenderObject(entity);

    if (ro)
    {
        for (int32 i = 0, sz = ro->GetRenderBatchCount(); i < sz; ++i)
        {
            RenderBatch* renderBatch = ro->GetRenderBatch(i);
            PolygonGroup* group = renderBatch->GetPolygonGroup();
            if (group)
            {
                int32 format = group->GetFormat();
                if (((format & prerequiredFormat) == prerequiredFormat) && !(format & EVF_BINORMAL))
                    MeshUtils::RebuildMeshTangentSpace(group, true);
            }
        }
    }

    for (int32 i = 0, sz = entity->GetChildrenCount(); i < sz; ++i)
        RebuildTangentSpace(entity->GetChild(i));
}

void SceneFileV2::ConvertShadowVolumes(Entity* entity, NMaterial* shadowMaterialParent)
{
    RenderObject* ro = GetRenderObject(entity);
    if (ro)
    {
        int32 batchCount = ro->GetRenderBatchCount();
        for (int32 ri = 0; ri < batchCount; ++ri)
        {
            RenderBatch* batch = ro->GetRenderBatch(ri);
            if (typeid(*batch) == typeid(ShadowVolume) || entity->GetName().find("_shadow") != String::npos)
            {
                RenderBatch* shadowBatch = new RenderBatch();
                if (typeid(*batch) == typeid(ShadowVolume))
                {
                    shadowBatch->SetPolygonGroup(batch->GetPolygonGroup());
                }
                else
                {
                    PolygonGroup* shadowPg = MeshUtils::CreateShadowPolygonGroup(batch->GetPolygonGroup());
                    shadowBatch->SetPolygonGroup(shadowPg);
                    shadowPg->Release();
                }

                NMaterial* shadowMaterial = new NMaterial();
                shadowMaterial->SetParent(shadowMaterialParent);

                shadowMaterial->SetMaterialName(FastName(Format("%s-%u",
                                                                shadowMaterialParent->GetMaterialName().c_str(),
                                                                static_cast<DAVA::uint32>(shadowMaterialParent->GetChildren().size()))));

                shadowBatch->SetMaterial(shadowMaterial);
                shadowMaterial->Release();

                ro->ReplaceRenderBatch(ri, shadowBatch);
                shadowBatch->Release();
            }
        }
    }

    uint32 size = entity->GetChildrenCount();
    for (uint32 i = 0; i < size; ++i)
    {
        Entity* child = entity->GetChild(i);
        ConvertShadowVolumes(child, shadowMaterialParent);
    }
}

void SceneFileV2::OptimizeScene(Entity* rootNode)
{
    removedNodeCount = 0;
    rootNode->BakeTransforms();

    RemoveEmptyHierarchy(rootNode);

    if (header.version < SHADOW_VOLUME_SCENE_VERSION)
    {
        NMaterial* shadowMaterial = new NMaterial();
        shadowMaterial->SetMaterialName(FastName("Shadow_Material"));
        shadowMaterial->SetFXName(NMaterialName::SHADOW_VOLUME);
        ConvertShadowVolumes(rootNode, shadowMaterial);
        shadowMaterial->Release();
    }

    if (header.version < TREE_ANIMATION_SCENE_VERSION)
    {
        SpeedTreeConverter treeConverter;
        treeConverter.ConvertTrees(rootNode);
    }

    if (header.version < PREREQUIRED_BINORMAL_SCENE_VERSION)
    {
        RebuildTangentSpace(rootNode);
    }

    if (header.version < DEPRECATED_MATERIAL_FLAGS_SCENE_VERSION)
    {
        RemoveDeprecatedMaterialFlags(rootNode);
    }

    if (header.version < ALPHATEST_VALUE_FLAG_SCENE_VERSION)
    {
        ConvertAlphatestValueMaterials(rootNode);
    }

    if (header.version < OLD_MATERIAL_FLAGS_SCENE_VERSION)
    {
        RemoveDeprecatedMaterialFlags(rootNode);
    }

    if (header.version < SPEED_TREE_POLYGON_GROUPS_PIVOT3_SCENE_VERSION)
    {
        SpeedTreeConverter covert;
        covert.ConvertPolygonGroupsPivot3(rootNode);
    }

    {
        SpeedTreeConverter convert;
        convert.ValidateSpeedTreeComponentCount(rootNode);
    }

    QualitySettingsSystem::Instance()->UpdateEntityAfterLoad(rootNode);
}

void SceneFileV2::UpdatePolygonGroupRequestedFormatRecursively(Entity* entity)
{
    RenderObject* ro = GetRenderObject(entity);

    if (ro)
    {
        for (int32 i = 0, sz = ro->GetRenderBatchCount(); i < sz; ++i)
        {
            RenderBatch* renderBatch = ro->GetRenderBatch(i);
            PolygonGroup* group = renderBatch->GetPolygonGroup();
            NMaterial* material = renderBatch->GetMaterial();
            if (group && material)
                serializationContext.AddRequestedPolygonGroupFormat(group, material->GetRequiredVertexFormat());
        }
    }

    for (int32 i = 0, sz = entity->GetChildrenCount(); i < sz; ++i)
        UpdatePolygonGroupRequestedFormatRecursively(entity->GetChild(i));
}

SceneArchive::~SceneArchive()
{
    for (auto& node : dataNodes)
    {
        SafeRelease(node);
    }
    for (auto& child : children)
    {
        SafeRelease(child);
    }
}

SceneArchive::SceneArchiveHierarchyNode::SceneArchiveHierarchyNode()
    : archive(nullptr)
{
}

bool SceneArchive::SceneArchiveHierarchyNode::LoadHierarchy(File* file)
{
    bool resultLoad = true;
    archive = new KeyedArchive();
    resultLoad &= archive->Load(file);
    int32 childrenCount = archive->GetInt32("#childrenCount", 0);
    children.reserve(childrenCount);
    for (int ci = 0; ci < childrenCount; ++ci)
    {
        SceneArchiveHierarchyNode* child = new SceneArchiveHierarchyNode();
        resultLoad &= child->LoadHierarchy(file);
        children.push_back(child);
    }
    return resultLoad;
}

SceneArchive::SceneArchiveHierarchyNode::~SceneArchiveHierarchyNode()
{
    SafeRelease(archive);
    for (auto& child : children)
    {
        SafeRelease(child);
    }
}
};
