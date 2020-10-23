#ifndef __DAVAENGINE_SCENEFILEV2_H__
#define __DAVAENGINE_SCENEFILEV2_H__

#include "Base/BaseObject.h"
#include "Base/BaseMath.h"
#include "Render/3D/StaticMesh.h"
#include "Render/3D/PolygonGroup.h"
#include "Utils/Utils.h"
#include "FileSystem/File.h"
#include "Scene3D/SceneFile/SerializationContext.h"
#include "Scene3D/SceneFile/VersionInfo.h"

namespace DAVA
{
/*
     Scene serialization thoughts: 
     
     There are 2 types of nodes: 
     1. Data Object
     - Texture - WILL BE REMOVED
     - Material - textures is instanced by names
     - Polygroup - group of polygons
     - Static Mesh - several polygroups combined
     - Animated Mesh - mesh with weights and bones heirarhy information
     - Animations - animation for the specific mesh / bones
     - Tiled Textures - in FUTURE for dynamic loading of landscapes when traveling over the world
     
     2. Hierarchy Node
     
     * Object nodes 
     - Light - light in the scene
     - Camera - camera in the scene
     - MeshInstance - instance of the specific mesh in the scene
     - AnimatedMeshInstance - instance of the specific animated mesh in the scene / animated mesh instances should store data inside.
     - Landscape - terrain object
     - Skeleton - root of the skeletal animated mesh
     - Bone - node for skeletal animation
     - Dummy - dummy object that can be used for additional client logic
     - ParticleEmitter - emitter node
     - Occluder - for occlusion culling
     
     * Clusterization nodes 
     - BSPTree - tbd later
     - QuadTree - tbd later 
     - OctTree - tbd later
     - Portal - tbd later
     
     General thoughts
     - Format should support uploading of data nodes on the fly
     - Format should support versioning for nodes
     - Keyed archive
     
 
    3. Required types
    -   int32, float32, 
    -   Vector2, Vector3, Vector4
    -   Matrix3, Matrix4
    -   Quaternion
 
 
     Scene * scene = ...;
     scene->Load("filename
*/

class NMaterial;
class Scene;

class SceneArchive : public BaseObject
{
public:
    struct SceneArchiveHierarchyNode : public BaseObject
    {
        KeyedArchive* archive;
        Vector<SceneArchiveHierarchyNode*> children;
        SceneArchiveHierarchyNode();
        bool LoadHierarchy(File* file);

    protected:
        ~SceneArchiveHierarchyNode();
    };

    Vector<SceneArchiveHierarchyNode*> children;
    Vector<KeyedArchive*> dataNodes;

protected:
    ~SceneArchive();
};

class SceneFileV2 : public BaseObject
{
private:
    struct Header
    {
        Header()
            : version(0)
            , nodeCount(0)
              {};

        char signature[4];
        int32 version;
        int32 nodeCount;
    };

protected:
    virtual ~SceneFileV2();

public:
    enum eError
    {
        ERROR_NO_ERROR = 0,
        ERROR_VERSION_IS_TOO_OLD,
        ERROR_FAILED_TO_CREATE_FILE,
        ERROR_FILE_WRITE_ERROR,
        ERROR_FILE_READ_ERROR,
        ERROR_VERSION_TAGS_INVALID,
    };

    enum eFileType
    {
        SceneFile = 0,
        ModelFile = 1
    };

    SceneFileV2();

    eError SaveScene(const FilePath& filename, Scene* _scene, SceneFileV2::eFileType fileType = SceneFileV2::SceneFile);
    eError LoadScene(const FilePath& filename, Scene* _scene);
    static VersionInfo::SceneVersion LoadSceneVersion(const FilePath& filename);

    void EnableDebugLog(bool _isDebugLogEnabled);
    bool DebugLogEnabled();
    void EnableSaveForGame(bool _isSaveForGame);

    //Material * GetMaterial(int32 index);
    //StaticMesh * GetStaticMesh(int32 index);

    //DataNode * GetNodeByPointer(uint64 pointer);

    void SetError(eError error);
    eError GetError() const;

    void OptimizeScene(Entity* rootNode);
    bool RemoveEmptyHierarchy(Entity* currentNode);
    void RebuildTangentSpace(Entity* entity);
    void ConvertShadowVolumes(Entity* rootNode, NMaterial* shadowMaterialParent);
    void RemoveDeprecatedMaterialFlags(Entity* rootNode);
    void ConvertAlphatestValueMaterials(Entity* rootNode);
    int32 removedNodeCount = 0;

    void UpdatePolygonGroupRequestedFormatRecursively(Entity* entity);
    SceneArchive* LoadSceneArchive(const FilePath& filename); //purely load data

private:
    static bool ReadHeader(Header& header, File* file);
    static bool ReadVersionTags(VersionInfo::SceneVersion& version, File* file);
    void AddToNodeMap(DataNode* node);

    Header header;

    struct Descriptor
    {
        uint32 size = 0;
        uint32 fileType = 0; //see enum SceneFileV2::eFileType
    };
    Descriptor descriptor;

    // Vector<StaticMesh*> staticMeshes;

    bool SaveDataHierarchy(DataNode* node, File* file, int32 level);
    void LoadDataHierarchy(Scene* scene, DataNode* node, File* file, int32 level);
    bool SaveDataNode(DataNode* node, File* file);
    bool LoadDataNode(Scene* scene, DataNode* parent, File* file);

    inline bool IsDataNodeSerializable(DataNode* node)
    {
        //VI: runtime nodes (such as ShadowVolume materials are not serializable)
        return (!node->IsRuntime());
    }

    bool SaveHierarchy(Entity* node, File* file, int32 level);
    bool LoadHierarchy(Scene* scene, Entity* node, File* file, int32 level);

    void FixLodForLodsystem2(Entity* entity);

    Entity* LoadEntity(Scene* scene, KeyedArchive* archive);
    Entity* LoadLandscape(Scene* scene, KeyedArchive* archive);
    Entity* LoadCamera(Scene* scene, KeyedArchive* archive);
    Entity* LoadLight(Scene* scene, KeyedArchive* archive);

    void ApplyFogQuality(DAVA::NMaterial* material);

    static bool WriteDescriptor(File* file, const Descriptor& descriptor);
    static bool ReadDescriptor(File* file, /*out*/ Descriptor& descriptor);

    bool isDebugLogEnabled;
    bool isSaveForGame;
    eError lastError;

    SerializationContext serializationContext;
};

}; // namespace DAVA

#endif // __DAVAENGINE_SCENEFILEV2_H__
