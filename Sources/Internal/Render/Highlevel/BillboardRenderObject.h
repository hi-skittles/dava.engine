#pragma once

#include "Render/Highlevel/RenderObject.h"
#include "Reflection/Reflection.h"

namespace DAVA
{
class BillboardRenderObject : public RenderObject
{
public:
    enum BillboardType : uint32
    {
        BILLBOARD_SPHERICAL,
        BILLBOARD_CYLINDRICAL
    };

public:
    BillboardRenderObject();

    void RecalcBoundingBox() override;
    void PrepareToRender(Camera* camera) override;
    void BindDynamicParameters(Camera* camera, RenderBatch* batch) override;

    void Save(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Load(KeyedArchive* archive, SerializationContext* serializationContext) override;

    uint32 GetBillboardType() const;
    void SetBillboardType(uint32 type);

    RenderObject* Clone(RenderObject* toObject) override;

private:
    Matrix4 billboardTransform = Matrix4::IDENTITY;
    uint32 billboardType = BillboardType::BILLBOARD_SPHERICAL;

    DAVA_VIRTUAL_REFLECTION(BillboardRenderObject, RenderObject);
};

inline uint32 BillboardRenderObject::GetBillboardType() const
{
    return billboardType;
}
}
