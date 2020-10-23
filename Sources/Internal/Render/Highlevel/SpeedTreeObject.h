#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Reflection/Reflection.h"
#include "Render/Highlevel/Mesh.h"

namespace DAVA
{
class SpeedTreeUpdateSystem;
class SpeedTreeObject : public RenderObject
{
public:
    static const size_t HARMONICS_BUFFER_CAPACITY = 4 * 7; //7 registers (float4)

    SpeedTreeObject();
    virtual ~SpeedTreeObject();

    void RecalcBoundingBox() override;
    RenderObject* Clone(RenderObject* newObject) override;
    void Save(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Load(KeyedArchive* archive, SerializationContext* serializationContext) override;

    void BindDynamicParameters(Camera* camera, RenderBatch* batch) override;
    void PrepareToRender(Camera* camera) override;

    void SetSphericalHarmonics(const DAVA::Array<float32, HARMONICS_BUFFER_CAPACITY>& coeffs);
    const DAVA::Array<float32, HARMONICS_BUFFER_CAPACITY>& GetSphericalHarmonics() const;

    //Interpolate between globally smoothed (0.0) and locally smoothed (1.0) leafs lighting
    void SetLightSmoothing(const float32& smooth);
    const float32& GetLightSmoothing() const;

    static PolygonGroup* CreateSortedPolygonGroup(PolygonGroup* pg);

protected:
    static const FastName FLAG_WIND_ANIMATION;
    static const uint32 SORTING_DIRECTION_COUNT = 8;

    static Vector3 GetSortingDirection(uint32 directionIndex);
    static uint32 SelectDirectionIndex(const Vector3& direction);

    AABBox3 CalcBBoxForSpeedTreeGeometry(RenderBatch* rb);

    void SetTreeAnimationParams(const Vector2& trunkOscillationParams, const Vector2& leafOscillationParams);
    void UpdateAnimationFlag(int32 maxAnimatedLod);

    void SetInvWorldTransformPtr(const Matrix4* invWT);

    Vector2 trunkOscillation;
    Vector2 leafOscillation;

    DAVA::Array<float32, HARMONICS_BUFFER_CAPACITY> sphericalHarmonics;
    float32 lightSmoothing;

    const Matrix4* invWorldTransform = nullptr;

    DAVA_VIRTUAL_REFLECTION(SpeedTreeObject, RenderObject);

    friend class SpeedTreeUpdateSystem;
};

inline void SpeedTreeObject::SetInvWorldTransformPtr(const Matrix4* invWT)
{
    invWorldTransform = invWT;
}
}
