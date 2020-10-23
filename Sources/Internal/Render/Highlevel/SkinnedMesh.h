#pragma once

#include "Animation/AnimatedObject.h"
#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/UnordererMap.h"
#include "Debug/DVAssert.h"
#include "Render/Highlevel/RenderSystem.h"
#include "Render/Highlevel/RenderObject.h"
#include "Scene3D/SceneFile/SerializationContext.h"

namespace DAVA
{
class PolygonGroup;
class RenderBatch;
class ShadowVolume;
class NMaterial;
class JointTransform;
class SkinnedMesh : public RenderObject
{
public:
    const static uint32 MAX_TARGET_JOINTS = 32; //same as in shader

    using JointTargets = Vector<int32>; // Vector index is joint target, value - skeleton joint index.

    struct JointTargetsData
    {
        JointTargetsData() = default;

        Vector<Vector4> positions;
        Vector<Vector4> quaternions;
        uint32 jointsDataCount = 0;
    };

    SkinnedMesh();

    RenderObject* Clone(RenderObject* newObject) override;
    void Save(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Load(KeyedArchive* archive, SerializationContext* serializationContext) override;

    void BindDynamicParameters(Camera* camera, RenderBatch* batch) override;

    void SetBoundingBox(const AABBox3& box);
    void UpdateJointTransforms(const Vector<JointTransform>& finalTransforms);

    void SetJointTargets(RenderBatch* batch, const JointTargets& jointTargets);

    const JointTargets& GetJointTargets(RenderBatch* batch);
    const JointTargetsData& GetJointTargetsData(RenderBatch* batch);

protected:
    UnorderedMap<RenderBatch*, uint32> jointTargetsDataMap; //RenderBatch -> targets-data index
    Vector<std::pair<JointTargets, JointTargetsData>> jointTargetsData;
};

inline void SkinnedMesh::SetBoundingBox(const AABBox3& box)
{
    bbox = box;
}

} //ns
