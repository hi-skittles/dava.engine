#pragma once
#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/FastName.h"
#include "Render/RenderBase.h"
#include "Base/BaseMath.h"
#include "Reflection/Reflection.h"

#include "Render/3D/PolygonGroup.h"
#include "Render/Highlevel/RenderObject.h"
#include "Render/Material/NMaterial.h"

#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class RenderLayer;
class Camera;
class RenderObject;
class RenderBatch;
class NMaterial;

class RenderBatch : public BaseObject
{
protected:
    virtual ~RenderBatch();

public:
    RenderBatch();

    void SetPolygonGroup(PolygonGroup* _polygonGroup);
    inline PolygonGroup* GetPolygonGroup();

    void SetMaterial(NMaterial* _material);
    inline NMaterial* GetMaterial();

    void SetRenderObject(RenderObject* renderObject);
    inline RenderObject* GetRenderObject() const;

    inline void SetStartIndex(uint32 _startIndex);
    inline void SetIndexCount(uint32 _indexCount);

    const AABBox3& GetBoundingBox() const;

    virtual void GetDataNodes(Set<DataNode*>& dataNodes);
    virtual RenderBatch* Clone(RenderBatch* destination = 0);
    virtual void Save(KeyedArchive* archive, SerializationContext* serializationContext);
    virtual void Load(KeyedArchive* archive, SerializationContext* serializationContext);

    /*
        \brief This is additional sorting key. It should be from 0 to 15.
     */
    void SetSortingKey(uint32 key);
    inline uint32 GetSortingKey() const;

    /*sorting offset allowed in 0..31 range, 15 default, more - closer to camera*/
    void SetSortingOffset(uint32 offset);
    inline uint32 GetSortingOffset();

    void BindGeometryData(rhi::Packet& packet);

    void UpdateAABBoxFromSource();

    pointer_size layerSortingKey;

    rhi::HVertexBuffer vertexBuffer;
    rhi::HVertexBuffer instanceBuffer;
    uint32 vertexCount = 0;
    uint32 vertexBase = 0;
    rhi::HIndexBuffer indexBuffer;
    uint32 startIndex = 0;
    uint32 indexCount = 0;
    uint32 instanceCount = 0;
    rhi::HPerfQuery perfQueryStart;
    rhi::HPerfQuery perfQueryEnd;

    rhi::PrimitiveType primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    uint32 vertexLayoutId = rhi::VertexLayout::InvalidUID;
    bool debugDrawOctree = false;

private:
    PolygonGroup* dataSource = nullptr;

    NMaterial* material = nullptr;
    RenderObject* renderObject = nullptr;

    const static uint32 SORTING_KEY_MASK = 0x0f;
    const static uint32 SORTING_OFFSET_MASK = 0x1f0;
    const static uint32 SORTING_OFFSET_SHIFT = 4;
    const static uint32 SORTING_KEY_DEF_VALUE = 0xf8;

    uint32 sortingKey = SORTING_KEY_DEF_VALUE; //oooookkkk - where 'o' is offset, 'k' is key

    AABBox3 aabbox = AABBox3(Vector3(), Vector3());

    void InsertDataNode(DataNode* node, Set<DataNode*>& dataNodes);

    DAVA_VIRTUAL_REFLECTION(RenderBatch, BaseObject);
};

inline PolygonGroup* RenderBatch::GetPolygonGroup()
{
    return dataSource;
}

inline NMaterial* RenderBatch::GetMaterial()
{
    return material;
}

inline RenderObject* RenderBatch::GetRenderObject() const
{
    return renderObject;
}

inline void RenderBatch::SetStartIndex(uint32 _startIndex)
{
    startIndex = _startIndex;
}

inline void RenderBatch::SetIndexCount(uint32 _indexCount)
{
    indexCount = _indexCount;
}

inline uint32 RenderBatch::GetSortingKey() const
{
    return sortingKey & SORTING_KEY_MASK;
}

inline uint32 RenderBatch::GetSortingOffset()
{
    return ((sortingKey & SORTING_OFFSET_MASK) >> SORTING_OFFSET_SHIFT);
}

inline void RenderBatch::BindGeometryData(rhi::Packet& packet)
{
    if (dataSource)
    {
        packet.vertexStreamCount = 1;
        packet.vertexStream[0] = dataSource->vertexBuffer;
        packet.vertexStream[1] = rhi::HVertexBuffer();
        packet.instanceCount = 0;
        packet.baseVertex = 0;
        packet.vertexCount = dataSource->vertexCount;
        packet.indexBuffer = dataSource->indexBuffer;
        packet.primitiveType = dataSource->primitiveType;
        packet.primitiveCount = dataSource->primitiveCount;
        packet.vertexLayoutUID = dataSource->vertexLayoutId;
        packet.startIndex = startIndex;
    }
    else
    {
        packet.vertexStreamCount = instanceCount ? 2 : 1;
        packet.vertexStream[0] = vertexBuffer;
        packet.vertexStream[1] = instanceCount ? instanceBuffer : rhi::HVertexBuffer();
        packet.instanceCount = instanceCount;
        packet.baseVertex = vertexBase;
        packet.vertexCount = vertexCount;
        packet.indexBuffer = indexBuffer;
        packet.primitiveType = primitiveType;
        packet.primitiveCount = CalculatePrimitiveCount(indexCount, primitiveType);
        packet.vertexLayoutUID = vertexLayoutId;
        packet.startIndex = startIndex;
    }
}
}
