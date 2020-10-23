#pragma once

#include "Base/BaseObject.h"
#include "Base/BaseMath.h"
#include "Reflection/Reflection.h"
#include "Render/RenderBase.h"
#include "Scene3D/SceneFile/SerializationContext.h"

//default direction (with identity matrix) is -y
namespace DAVA
{
class SceneFileV2;
class Camera;

class Light : public BaseObject
{
public:
    enum eType
    {
        TYPE_DIRECTIONAL = 0,
        TYPE_SPOT,
        TYPE_POINT,
        TYPE_SKY,
        TYPE_AMBIENT,

        TYPE_COUNT
    };

    enum eFlags
    {
        IS_DYNAMIC = 1 << 0,
        CAST_SHADOW = 1 << 1,
    };

protected:
    virtual ~Light();

public:
    Light();

    virtual BaseObject* Clone(BaseObject* dstNode = NULL);

    void SetType(eType _type);
    void SetAmbientColor(const Color& _color);
    void SetDiffuseColor(const Color& _color);
    void SetIntensity(float32 intensity);

    eType GetType() const;
    const Color& GetAmbientColor() const;
    const Color& GetDiffuseColor() const;
    float32 GetIntensity() const;

    const Vector3& GetPosition() const;
    const Vector3& GetDirection() const;

    void SetPosition(const Vector3& position);
    void SetDirection(const Vector3& direction);

    void SetPositionDirectionFromMatrix(const Matrix4& worldTransform);

    const Vector4& CalculatePositionDirectionBindVector(Camera* camera);

    //virtual void Update(float32 timeElapsed);
    //virtual void Draw();

    virtual void Save(KeyedArchive* archive, SerializationContext* serializationContext);
    virtual void Load(KeyedArchive* archive, SerializationContext* serializationContext);

    bool IsDynamic();
    void SetDynamic(const bool& isDynamic);
    void AddFlag(uint32 flag);
    void RemoveFlag(uint32 flag);
    uint32 GetFlags();

    //void SetRenderSystem

protected:
    uint32 flags;
    Camera* camera;
    uint32 lastUpdatedFrame;
    uint32 type;
    Vector3 position;
    Vector3 direction;
    Vector4 resultPositionDirection;

    Color ambientColor;
    Color diffuseColor;
    float32 intensity;

    DAVA_VIRTUAL_REFLECTION(Light, BaseObject);
};
};
