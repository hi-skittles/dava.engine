#ifndef __DAVAENGINE_SPRITE_OBJECT_H__
#define __DAVAENGINE_SPRITE_OBJECT_H__

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Render/Highlevel/RenderObject.h"
#include "Render/2D/Sprite.h"

namespace DAVA
{
class SpriteObject : public RenderObject
{
public:
    SpriteObject();
    SpriteObject(const FilePath& pathToSprite, int32 frame = 0
                 ,
                 const Vector2& reqScale = Vector2(1.0f, 1.0f)
                 ,
                 const Vector2& pivotPoint = Vector2(0.0f, 0.0f));
    SpriteObject(Sprite* spr, int32 frame = 0
                 ,
                 const Vector2& reqScale = Vector2(1.0f, 1.0f)
                 ,
                 const Vector2& pivotPoint = Vector2(0.0f, 0.0f));

    virtual ~SpriteObject();

    enum eSpriteType
    {
        SPRITE_OBJECT = 0, //! draw sprite without any transformations. Set by default.
        SPRITE_BILLBOARD, //! normal billboard when billboard is always parallel to the camera projection plane. It computed by multiplication of worldMatrix of node to [R]^-1 matrix of camera
        SPRITE_BILLBOARD_TO_CAMERA, //! billboard is facing to camera point
    };

    /**
        \brief Set type of coordinates modification for the given sprite node
        \param[in] type type you want to set
     */
    void SetSpriteType(eSpriteType type);
    /**
        \brief Get type of coordinates modification for the given sprite node
        \returns type that was set to this sprite node
     */
    eSpriteType GetSpriteType() const;

    /**
        \brief Change sprite frame for this sprite node 
        \param[in] newFrame frame you want to set
     */
    void SetFrame(int32 newFrame);

    /**
        \brief Get frame for this sprite node
        \returns frame index that was set for this node last time
     */
    int32 GetFrame() const;

    Sprite* GetSprite() const;

    const Vector2& GetScale() const;
    const Vector2& GetPivot() const;

    RenderObject* Clone(RenderObject* newObject) override;

    void Save(KeyedArchive* archive, SerializationContext* serializationContext) override;
    void Load(KeyedArchive* archive, SerializationContext* serializationContext) override;

    void RecalcBoundingBox() override;

    void BindDynamicParameters(Camera* camera, RenderBatch* batch) override;

private:
    void Clear();
    void ClearRenderBatches();
    void Restore();
    void UpdateBufferData(RenderBatch* batch);
    void Init(Sprite* spr, int32 _frame, const Vector2& reqScale, const Vector2& pivotPoint);
    void SetupRenderBatch();
    void RegisterRestoreCallback();

private:
    Sprite* sprite = nullptr;
    Matrix4 worldMatrix;
    Vector2 sprScale;
    Vector2 sprPivot;
    int32 frame = 0;
    eSpriteType spriteType = SPRITE_OBJECT;
};
};

#endif // __DAVAENGINE_SPRITE_OBJECT_H__
