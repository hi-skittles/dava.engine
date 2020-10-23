#include "Scene3D/SceneFile/SerializationContext.h"
#include "Scene3D/DataNode.h"

#include "Scene3D/Scene.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "Render/Material/NMaterial.h"

#include "Utils/StringFormat.h"

#include "Render/Material/NMaterialNames.h"
#include "Render/Texture.h"

namespace DAVA
{
SerializationContext::SerializationContext()
    : globalMaterialKey(0)
{
}

SerializationContext::~SerializationContext()
{
    for (Map<uint64, DataNode*>::iterator it = dataBlocks.begin();
         it != dataBlocks.end();
         ++it)
    {
        SafeRelease(it->second);
    }

    for (Map<uint64, NMaterial*>::iterator it = importedMaterials.begin();
         it != importedMaterials.end();
         ++it)
    {
        SafeRelease(it->second);
    }

    DVASSERT(materialBindings.size() == 0 && "Serialization context destroyed without resolving material bindings!");
    materialBindings.clear();
}

void SerializationContext::ResolveMaterialBindings()
{
    size_t instanceCount = materialBindings.size();
    for (size_t i = 0; i < instanceCount; ++i)
    {
        MaterialBinding& binding = materialBindings[i];
        uint64 parentKey = (binding.parentKey) ? binding.parentKey : globalMaterialKey;
        if (!parentKey)
            continue;

        NMaterial* parentMat = static_cast<NMaterial*>(GetDataBlock(parentKey));

        DVASSERT(parentMat);
        if (parentMat != binding.childMaterial) //global material case
        {
            binding.childMaterial->SetParent(parentMat);
        }
    }

    materialBindings.clear();
}

void SerializationContext::AddLoadedPolygonGroup(PolygonGroup* group, uint32 dataFilePos)
{
    DVASSERT(loadedPolygonGroups.find(group) == loadedPolygonGroups.end());
    PolygonGroupLoadInfo loadInfo;
    loadInfo.filePos = dataFilePos;
    loadedPolygonGroups[group] = loadInfo;
}
void SerializationContext::AddRequestedPolygonGroupFormat(PolygonGroup* group, int32 format)
{
    auto foundGroup = loadedPolygonGroups.find(group);
    DVASSERT(foundGroup != loadedPolygonGroups.end());
    foundGroup->second.requestedFormat |= format;
    foundGroup->second.onScene = true;
}

bool SerializationContext::LoadPolygonGroupData(File* file)
{
    bool resultLoaded = true;
    bool cutUnusedStreams = QualitySettingsSystem::Instance()->GetAllowCutUnusedVertexStreams();
    for (Map<PolygonGroup *, PolygonGroupLoadInfo>::iterator it = loadedPolygonGroups.begin(), e = loadedPolygonGroups.end(); it != e; ++it)
    {
        if (it->second.onScene || !cutUnusedStreams)
        {
            resultLoaded &= file->Seek(it->second.filePos, File::SEEK_FROM_START);
            KeyedArchive* archive = new KeyedArchive();
            resultLoaded &= archive->Load(file);
            it->first->LoadPolygonData(archive, this, it->second.requestedFormat, cutUnusedStreams);
            SafeRelease(archive);
        }
    }
    return resultLoaded;
}
}