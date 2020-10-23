#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Reflection/Reflection.h"
#include "Animation/AnimatedObject.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Render/Highlevel/RenderBatch.h"
#include "Scene3D/Scene.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
const static uint16 INVALID_STATIC_OCCLUSION_INDEX = uint16(-1);

class RenderBatch;

struct RenderBatchWithOptions : public InspBase
{
    RenderBatch* renderBatch = nullptr;
    int32 lodIndex = -2;
    int32 switchIndex = -1;

    RenderBatchWithOptions() = default;

    RenderBatchWithOptions(const RenderBatchWithOptions& r) = default;

    RenderBatchWithOptions(RenderBatch* rb, int32 l, int32 s)
        : renderBatch(rb)
        , lodIndex(l)
        , switchIndex(s)
    {
    }

    bool operator==(const RenderBatchWithOptions& other) const;

    DAVA_VIRTUAL_REFLECTION(RenderBatchWithOptions, InspBase);
};

class RenderBatchProvider : public BaseObject
{
public:
    virtual ~RenderBatchProvider()
    {
    }
    virtual const Vector<RenderBatchWithOptions> GetRenderBatches() const = 0;

    DAVA_VIRTUAL_REFLECTION(RenderBatchProvider, BaseObject);
};

class RenderObject : public BaseObject
{
public:
    enum eType : uint32
    {
        TYPE_RENDEROBJECT = 0, // Base Render Object
        TYPE_MESH, // Normal mesh
        TYPE_SKINNED_MESH, // Animated mesh for skinned animations
        TYPE_LANDSCAPE, // Landscape object
        TYPE_CUSTOM_DRAW, // Custom drawn object
        TYPE_SPRITE, // Sprite Node
        TYPE_PARTICLE_EMITTER, // Particle Emitter
        TYPE__DELETED__SKYBOX, //keept for legasy, skybox removed in RHI
        TYPE_VEGETATION,
        TYPE_SPEED_TREE,
        TYPE_BILLBOARD,
    };

    enum eFlags
    {
        VISIBLE = 1 << 0,
        ALWAYS_CLIPPING_VISIBLE = 1 << 4,
        VISIBLE_STATIC_OCCLUSION = 1 << 5,
        TREE_NODE_NEED_UPDATE = 1 << 6,
        NEED_UPDATE = 1 << 7,
        MARKED_FOR_UPDATE = 1 << 8,

        CUSTOM_PREPARE_TO_RENDER = 1 << 9, //if set, PrepareToRender would be called

        VISIBLE_REFLECTION = 1 << 10,
        VISIBLE_REFRACTION = 1 << 11,
        VISIBLE_QUALITY = 1 << 12,

        TRANSFORM_UPDATED = 1 << 15,
    };

    static const uint32 VISIBILITY_CRITERIA = VISIBLE | VISIBLE_STATIC_OCCLUSION | VISIBLE_QUALITY;
    static const uint32 CLIPPING_VISIBILITY_CRITERIA = VISIBLE | VISIBLE_STATIC_OCCLUSION | VISIBLE_QUALITY;
    static const uint32 SERIALIZATION_CRITERIA = VISIBLE | VISIBLE_REFLECTION | VISIBLE_REFRACTION | ALWAYS_CLIPPING_VISIBLE;
    static const uint32 MAX_LIGHT_COUNT = 2;

protected:
    virtual ~RenderObject();

public:
    RenderObject();

    inline void SetRemoveIndex(uint32 removeIndex);
    inline uint32 GetRemoveIndex();

    inline void SetTreeNodeIndex(uint16 index);
    inline uint16 GetTreeNodeIndex();

    void AddRenderBatch(RenderBatch* batch);
    void AddRenderBatch(RenderBatch* batch, int32 lodIndex, int32 switchIndex);
    void RemoveRenderBatch(RenderBatch* batch);
    void RemoveRenderBatch(uint32 batchIndex);
    void ReplaceRenderBatch(RenderBatch* oldBatch, RenderBatch* newBatch);
    void ReplaceRenderBatch(uint32 batchIndex, RenderBatch* newBatch);

    void SetRenderBatchLODIndex(uint32 batchIndex, int32 newLodIndex);
    void SetRenderBatchSwitchIndex(uint32 batchIndex, int32 newSwitchIndex);

    void AddRenderBatchProvider(RenderBatchProvider*);
    void RemoveRenderBatchProvider(RenderBatchProvider*);

    virtual void RecalcBoundingBox();

    inline uint32 GetRenderBatchCount() const;
    inline RenderBatch* GetRenderBatch(uint32 batchIndex) const;
    inline RenderBatch* GetRenderBatch(uint32 batchIndex, int32& lodIndex, int32& switchIndex) const;

    /**
     \brief collect render batches and append it to vector by request lods/switches
     \param[in] requestLodIndex - request lod index. if -1 considering all lods
     \param[in] requestSwitchIndex - request switch index. if -1 considering all switches
     \param[in, out] batches vector of RenderBatch'es
     \param[in] includeShareLods - if true considering request lod and lods with INVALID_INDEX(-1)
     */
    void CollectRenderBatches(int32 requestLodIndex, int32 requestSwitchIndex, Vector<RenderBatch*>& batches, bool includeShareLods = false) const;

    inline uint32 GetActiveRenderBatchCount() const;
    inline RenderBatch* GetActiveRenderBatch(uint32 batchIndex) const;

    inline void SetFlags(uint32 _flags)
    {
        flags = _flags;
    }
    inline uint32 GetFlags()
    {
        return flags;
    }
    inline void AddFlag(uint32 _flag)
    {
        flags |= _flag;
    }
    inline void RemoveFlag(uint32 _flag)
    {
        flags &= ~_flag;
    }

    inline void SetAABBox(const AABBox3& bbox);
    inline void SetWorldAABBox(const AABBox3& bbox);

    inline const AABBox3& GetBoundingBox() const;
    inline const AABBox3& GetWorldBoundingBox() const;

    inline void SetWorldMatrixPtr(Matrix4* _worldTransform);
    inline Matrix4* GetWorldMatrixPtr() const;
    inline void SetInverseTransform(const Matrix4& _inverseWorldTransform);
    inline const Matrix4& GetInverseWorldTransform() const;

    inline eType GetType() const
    {
        return static_cast<eType>(type);
    }

    virtual RenderObject* Clone(RenderObject* newObject);
    virtual void Save(KeyedArchive* archive, SerializationContext* serializationContext);
    virtual void Load(KeyedArchive* archive, SerializationContext* serializationContext);

    void SetOwnerDebugInfo(const FastName& str)
    {
        ownerDebugInfo = str;
    };

    virtual void SetRenderSystem(RenderSystem* renderSystem);
    RenderSystem* GetRenderSystem();

    virtual void BakeGeometry(const Matrix4& transform);

    virtual void RecalculateWorldBoundingBox();

    virtual void BindDynamicParameters(Camera* camera, RenderBatch* batch);

    inline uint16 GetStaticOcclusionIndex() const;
    inline void SetStaticOcclusionIndex(uint16 index);
    virtual void PrepareToRender(Camera* camera); //objects passed all tests and is going to be rendered this frame - by default calculates final matrix

    void SetLodIndex(const int32 lodIndex);
    void SetSwitchIndex(const int32 switchIndex);
    int32 GetLodIndex() const;
    int32 GetSwitchIndex() const;
    int32 GetMaxLodIndex() const;
    int32 GetMaxLodIndexForSwitchIndex(int32 forSwitchIndex) const;
    int32 GetMaxSwitchIndex() const;

    inline bool GetReflectionVisible() const;
    inline void SetReflectionVisible(bool visible);
    inline bool GetRefractionVisible() const;
    inline void SetRefractionVisible(bool visible);
    inline bool GetClippingVisible() const;
    inline void SetClippingVisible(bool visible);

    virtual void GetDataNodes(Set<DataNode*>& dataNodes);

    inline void SetLight(uint32 index, Light* light);
    inline Light* GetLight(uint32 index);

    inline void AddVisibilityStructureNode(uint32 nodeValue);
    inline void RemoveVisibilityStructureNode(uint32 nodeValue);
    inline uint32 GetVisibilityStructureNode(uint32 index) const;
    inline bool IsInVisibilityStructureNode(uint32 nodeValue) const;
    inline uint32 GetVisibilityStructureNodeCount() const;
    uint8 startClippingPlane = 0;

    static const uint32 MAX_VISIBILITY_STRUCTURE_NODE_COUNT = 8;
    uint32 inVisibilityNodes[MAX_VISIBILITY_STRUCTURE_NODE_COUNT];
    uint32 inVisibilityNodeCount = 0;

    struct VoxelCoord : public InspBase
    {
        VoxelCoord()
        {
        }
        VoxelCoord(uint32 x)
            : packedCoord(x)
        {
        }

        union
        {
            struct
            {
                uint32 level : 5;
                uint32 x : 9;
                uint32 y : 9;
                uint32 z : 9;
            };
            uint32 packedCoord;
        };
        uint32 GetLevel() const
        {
            return level;
        };
        void SetLevel(const uint32& _level)
        {
            level = _level;
        };
        uint32 GetX() const
        {
            return x;
        };
        void SetX(const uint32& _x)
        {
            x = _x;
        };
        uint32 GetY() const
        {
            return y;
        };
        void SetY(const uint32& _y)
        {
            y = _y;
        };
        uint32 GetZ() const
        {
            return z;
        };
        void SetZ(const uint32& _z)
        {
            z = _z;
        };
    };
    std::vector<VoxelCoord> inVisCopy;

protected:
    void UpdateAddedRenderBatch(RenderBatch* batch);
    void UpdateRemovedRenderBatch(RenderBatch* batch);
    void InternalAddRenderBatchToCollection(Vector<RenderBatchWithOptions>& dest, RenderBatch* batch, int32 lodIndex, int32 switchIndex);
    void InternalRemoveRenderBatchFromCollection(Vector<RenderBatchWithOptions>& collection, RenderBatch* batch);
    void UpdateActiveRenderBatchesFromCollection(const Vector<RenderBatchWithOptions>& collection);
    void UpdateActiveRenderBatches();

    static const int32 DEFAULT_RENDEROBJECT_FLAGS = eFlags::VISIBLE | eFlags::VISIBLE_STATIC_OCCLUSION | eFlags::VISIBLE_QUALITY;

protected:
    Vector<RenderBatchWithOptions> renderBatchArray;
    Vector<RenderBatchProvider*> renderBatchProviders;
    Vector<RenderBatch*> activeRenderBatchArray;
    Light* lights[MAX_LIGHT_COUNT];
    RenderSystem* renderSystem = nullptr;
    Matrix4* worldTransform = nullptr; // temporary - this should me moved directly to matrix uniforms
    Matrix4 inverseWorldTransform;
    FastName ownerDebugInfo;
    AABBox3 bbox;
    AABBox3 worldBBox;
    int32 lodIndex = -1;
    int32 switchIndex = -1;
    uint32 type = eType::TYPE_RENDEROBJECT;
    uint32 flags = DEFAULT_RENDEROBJECT_FLAGS;
    uint32 debugFlags = 0;
    uint32 removeIndex = static_cast<uint32>(-1);
    uint16 treeNodeIndex = QuadTree::INVALID_TREE_NODE_INDEX;
    uint16 staticOcclusionIndex = INVALID_STATIC_OCCLUSION_INDEX;

    DAVA_VIRTUAL_REFLECTION(RenderObject, BaseObject);
};

inline void RenderObject::SetLight(uint32 index, Light* light)
{
    DVASSERT(index < MAX_LIGHT_COUNT);
    lights[index] = light;
}

inline Light* RenderObject::GetLight(uint32 index)
{
    DVASSERT(index < MAX_LIGHT_COUNT);
    return lights[index];
}

inline uint32 RenderObject::GetRemoveIndex()
{
    return removeIndex;
}

inline void RenderObject::SetRemoveIndex(uint32 _removeIndex)
{
    removeIndex = _removeIndex;
}

inline void RenderObject::SetTreeNodeIndex(uint16 index)
{
    treeNodeIndex = index;
}
inline uint16 RenderObject::GetTreeNodeIndex()
{
    return treeNodeIndex;
}

inline void RenderObject::SetAABBox(const AABBox3& _bbox)
{
    bbox = _bbox;
}

inline void RenderObject::SetWorldAABBox(const AABBox3& _bbox)
{
    worldBBox = _bbox;
}

inline const AABBox3& RenderObject::GetBoundingBox() const
{
    return bbox;
}

inline const AABBox3& RenderObject::GetWorldBoundingBox() const
{
    return worldBBox;
}

inline void RenderObject::SetWorldMatrixPtr(Matrix4* _worldTransform)
{
    if (worldTransform == _worldTransform)
        return;
    worldTransform = _worldTransform;
    flags |= TRANSFORM_UPDATED;
}

inline Matrix4* RenderObject::GetWorldMatrixPtr() const
{
    return worldTransform;
}

inline void RenderObject::SetInverseTransform(const Matrix4& _inverseWorldTransform)
{
    inverseWorldTransform = _inverseWorldTransform;
}

inline const Matrix4& RenderObject::GetInverseWorldTransform() const
{
    return inverseWorldTransform;
}

inline uint32 RenderObject::GetRenderBatchCount() const
{
    return uint32(renderBatchArray.size());
}

inline RenderBatch* RenderObject::GetRenderBatch(uint32 batchIndex) const
{
    DVASSERT(batchIndex < renderBatchArray.size());

    return renderBatchArray[batchIndex].renderBatch;
}

inline RenderBatch* RenderObject::GetRenderBatch(uint32 batchIndex, int32& _lodIndex, int32& _switchIndex) const
{
    const RenderBatchWithOptions& irb = renderBatchArray[batchIndex];
    _lodIndex = irb.lodIndex;
    _switchIndex = irb.switchIndex;

    return irb.renderBatch;
}

inline uint32 RenderObject::GetActiveRenderBatchCount() const
{
    return uint32(activeRenderBatchArray.size());
}

inline RenderBatch* RenderObject::GetActiveRenderBatch(uint32 batchIndex) const
{
    return activeRenderBatchArray[batchIndex];
}

inline uint16 RenderObject::GetStaticOcclusionIndex() const
{
    return staticOcclusionIndex;
}

inline void RenderObject::SetStaticOcclusionIndex(uint16 _index)
{
    staticOcclusionIndex = _index;
}

inline bool RenderObject::GetReflectionVisible() const
{
    return (flags & VISIBLE_REFLECTION) == VISIBLE_REFLECTION;
}

inline void RenderObject::SetReflectionVisible(bool visible)
{
    if (visible)
        flags |= VISIBLE_REFLECTION;
    else
        flags &= ~VISIBLE_REFLECTION;
}

inline bool RenderObject::GetRefractionVisible() const
{
    return (flags & VISIBLE_REFRACTION) == VISIBLE_REFRACTION;
}
inline void RenderObject::SetRefractionVisible(bool visible)
{
    if (visible)
        flags |= VISIBLE_REFRACTION;
    else
        flags &= ~VISIBLE_REFRACTION;
}

inline bool RenderObject::GetClippingVisible() const
{
    return (flags & ALWAYS_CLIPPING_VISIBLE) == ALWAYS_CLIPPING_VISIBLE;
}

inline void RenderObject::SetClippingVisible(bool visible)
{
    if (visible)
        flags |= ALWAYS_CLIPPING_VISIBLE;
    else
        flags &= ~ALWAYS_CLIPPING_VISIBLE;
}

inline void RenderObject::AddVisibilityStructureNode(uint32 nodeValue)
{
    inVisibilityNodes[inVisibilityNodeCount++] = nodeValue;
    inVisCopy[inVisibilityNodeCount - 1] = VoxelCoord(nodeValue);
    DVASSERT(inVisibilityNodeCount <= MAX_VISIBILITY_STRUCTURE_NODE_COUNT);
}

inline void RenderObject::RemoveVisibilityStructureNode(uint32 nodeValue)
{
    DVASSERT(inVisibilityNodeCount <= MAX_VISIBILITY_STRUCTURE_NODE_COUNT);

    for (uint32 k = 0; k < inVisibilityNodeCount; ++k)
    {
        if (inVisibilityNodes[k] == nodeValue)
        {
            inVisibilityNodes[k] = inVisibilityNodes[inVisibilityNodeCount - 1];
            inVisCopy[k] = inVisCopy[inVisibilityNodeCount - 1];
            k--;

            DVASSERT(inVisibilityNodeCount > 0);
            inVisibilityNodeCount--;
        }
    }
}

inline bool RenderObject::IsInVisibilityStructureNode(uint32 nodeValue) const
{
    for (uint32 k = 0; k < inVisibilityNodeCount; ++k)
        if (inVisibilityNodes[k] == nodeValue)
            return true;
    return false;
}

inline uint32 RenderObject::GetVisibilityStructureNode(uint32 index) const
{
    return inVisibilityNodes[index];
}

inline uint32 RenderObject::GetVisibilityStructureNodeCount() const
{
    return inVisibilityNodeCount;
}

template <>
bool AnyCompare<RenderBatchWithOptions>::IsEqual(const DAVA::Any& v1, const DAVA::Any& v2);

extern template struct AnyCompare<RenderBatchWithOptions>;
}
