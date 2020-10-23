#ifndef __DAVAENGINE_SCENE_NODE_ANIMATION_KEY_H__
#define __DAVAENGINE_SCENE_NODE_ANIMATION_KEY_H__

#include "Base/BaseTypes.h"
#include "Scene3D/Entity.h"

namespace DAVA
{
class SceneNodeAnimationKey
{
public:
    float32 time = 0;
    Vector3 translation;
    Quaternion rotation;
    Vector3 scale;

    inline void GetMatrix(Matrix4& matrix) const;
};

inline void SceneNodeAnimationKey::GetMatrix(Matrix4& result) const
{
    Matrix4 localTransformRot;
    Matrix4 localTransformScale;
    localTransformRot = rotation.GetMatrix();
    localTransformScale.BuildScale(scale);

    result = localTransformRot * localTransformScale;
    result.SetTranslationVector(translation);
}
};

#endif // __DAVAENGINE_SCENE_NODE_ANIMATION_KEY_H__
