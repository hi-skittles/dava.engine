#ifndef __DAVAENGINE_SERIALIZATIONCONTEXT_H__
#define __DAVAENGINE_SERIALIZATIONCONTEXT_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/FastName.h"
#include "FileSystem/FilePath.h"
#include "Render/RenderBase.h"

namespace DAVA
{
class Scene;
class DataNode;
class NMaterial;
class Texture;
class NMaterial;
class PolygonGroup;

class SerializationContext
{
public:
    struct PolygonGroupLoadInfo
    {
        uint32 filePos = 0;
        int32 requestedFormat = EVF_VERTEX; //vertex position loading is required as all code assumes it is there
        bool onScene = false;
    };

public:
    SerializationContext();
    ~SerializationContext();

    inline void SetVersion(uint32 curVersion)
    {
        version = curVersion;
    }

    inline uint32 GetVersion() const
    {
        return version;
    }

    inline void SetDebugLogEnabled(bool state)
    {
        debugLogEnabled = state;
    }

    inline bool IsDebugLogEnabled() const
    {
        return debugLogEnabled;
    }

    inline void SetScene(Scene* target)
    {
        scene = target;
    }

    inline Scene* GetScene()
    {
        return scene;
    }

    inline void SetScenePath(const FilePath& path)
    {
        scenePath = path;
    }

    inline const FilePath& GetScenePath() const
    {
        return scenePath;
    }

    inline void SetRootNodePath(const FilePath& path)
    {
        rootNodePathName = path;
    }

    inline const FilePath& GetRootNodePath() const
    {
        return rootNodePathName;
    }

    inline void SetDataBlock(uint64 blockId, DataNode* data)
    {
        DVASSERT(dataBlocks.find(blockId) == dataBlocks.end());

        dataBlocks[blockId] = data;
    }

    inline DataNode* GetDataBlock(uint64 blockId)
    {
        Map<uint64, DataNode*>::iterator it = dataBlocks.find(blockId);
        return (it != dataBlocks.end()) ? it->second : NULL;
    }

    inline void AddBinding(uint64 parentKey, NMaterial* material)
    {
        MaterialBinding binding;
        binding.childMaterial = material;
        binding.parentKey = parentKey;

        materialBindings.push_back(binding);
    }

    inline void SetGlobalMaterialKey(uint64 materialKey)
    {
        globalMaterialKey = materialKey;
    }

    inline uint64 GetGlobalMaterialKey()
    {
        return globalMaterialKey;
    }

    inline void SetLastError(uint32 error)
    {
        lastError = error;
    }

    inline uint32 GetLastError()
    {
        return lastError;
    }

    inline void SetDefaultMaterialQuality(const FastName& quality)
    {
        defaultMaterialQuality = quality;
    }

    inline const FastName& GetDefaultMaterialQuality() const
    {
        return defaultMaterialQuality;
    }

    void ResolveMaterialBindings();

    void AddLoadedPolygonGroup(PolygonGroup* group, uint32 dataFilePos);
    void AddRequestedPolygonGroupFormat(PolygonGroup* group, int32 format);
    bool LoadPolygonGroupData(File* file);

    template <template <typename, typename> class Container, class T, class A>
    void GetDataNodes(Container<T, A>& container);

private:
    struct MaterialBinding
    {
        uint64 parentKey = 0;
        NMaterial* childMaterial = nullptr;
    };

    Map<uint64, DataNode*> dataBlocks;
    Map<uint64, NMaterial*> importedMaterials;
    Vector<MaterialBinding> materialBindings;
    Map<PolygonGroup*, PolygonGroupLoadInfo> loadedPolygonGroups;

    Scene* scene = nullptr;
    FilePath rootNodePathName;
    FilePath scenePath;
    FastName defaultMaterialQuality;
    uint64 globalMaterialKey = 0;
    uint32 lastError = 0;
    uint32 version = 0;

    bool debugLogEnabled = false;
};

template <template <typename, typename> class Container, class T, class A>
void SerializationContext::GetDataNodes(Container<T, A>& container)
{
    Map<uint64, DataNode*>::const_iterator end = dataBlocks.end();
    for (Map<uint64, DataNode*>::iterator t = dataBlocks.begin(); t != end; ++t)
    {
        DataNode* obj = t->second;

        T res = dynamic_cast<T>(obj);
        if (res)
            container.push_back(res);
    }
}
};

#endif
