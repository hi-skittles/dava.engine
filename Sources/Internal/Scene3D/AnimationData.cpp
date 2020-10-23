#include "Scene3D/AnimationData.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
AnimationData::AnimationData()
{
}

AnimationData::~AnimationData()
{
}

void AnimationData::AddKey(const SceneNodeAnimationKey& key)
{
    keys.push_back(key);
}

SceneNodeAnimationKey AnimationData::Interpolate(float32 t, uint32& startIdxCache) const
{
    if (keys.size() == 1)
    {
        return keys[0];
    }

    if (t < keys[startIdxCache].time)
    {
        startIdxCache = 0;
    }

    uint32 endIdx = 0;
    for (endIdx = startIdxCache; endIdx < keys.size(); ++endIdx)
    {
        if (keys[endIdx].time > t)
        {
            break;
        }
        startIdxCache = endIdx;
    }

    if (endIdx == keys.size())
    {
        endIdx = 0;
    }

    const SceneNodeAnimationKey& key1 = keys[startIdxCache];
    const SceneNodeAnimationKey& key2 = keys[endIdx];

    float32 tInter;
    if (endIdx > startIdxCache)
        tInter = (t - key1.time) / (key2.time - key1.time);
    else // interpolate from last to first
        tInter = (t - key1.time) / (duration - key1.time);

    SceneNodeAnimationKey result;
    result.translation.Lerp(key1.translation, key2.translation, tInter);
    result.rotation.Slerp(key1.rotation, key2.rotation, tInter);
    result.scale.Lerp(key1.scale, key2.scale, tInter);

    return result;
}

SceneNodeAnimationKey AnimationData::GetKeyForFrame(int32 frameIndex) const
{
    DVASSERT(frameIndex >= 0 && frameIndex < GetKeyCount());

    return keys[frameIndex];
}

void AnimationData::SetDuration(float32 _duration)
{
    duration = _duration;
}

void AnimationData::SetInvPose(const Matrix4& mat)
{
    invPose = mat;
}

const Matrix4& AnimationData::GetInvPose() const
{
    return invPose;
}

void AnimationData::Save(KeyedArchive* archive, SerializationContext* serializationContext)
{
    DataNode::Save(archive, serializationContext);

    archive->SetFloat("duration", duration);
    archive->SetInt32("keyCount", static_cast<int32>(keys.size()));
    archive->SetMatrix4("invPose", invPose);

    for (uint32 keyIndex = 0; keyIndex < keys.size(); ++keyIndex)
    {
        archive->SetFloat(Format("key_%i_time", keyIndex), keys[keyIndex].time);
        archive->SetVector3(Format("key_%i_translation", keyIndex), keys[keyIndex].translation);
        archive->SetVector3(Format("key_%i_scale", keyIndex), keys[keyIndex].scale);
        archive->SetVector4(Format("key_%i_rotation", keyIndex), Vector4(keys[keyIndex].rotation.x, keys[keyIndex].rotation.y, keys[keyIndex].rotation.z, keys[keyIndex].rotation.w));
    }
}

void AnimationData::Load(KeyedArchive* archive, SerializationContext* serializationContext)
{
    DataNode::Load(archive, serializationContext);

    const int32 keyCount = archive->GetInt32("keyCount");
    keys.resize(keyCount);

    SetDuration(archive->GetFloat("duration"));
    SetInvPose(archive->GetMatrix4("invPose"));

    for (int32 keyIndex = 0; keyIndex < keyCount; ++keyIndex)
    {
        keys[keyIndex].time = archive->GetFloat(Format("key_%i_time", keyIndex));
        keys[keyIndex].translation = archive->GetVector3(Format("key_%i_translation", keyIndex));
        keys[keyIndex].scale = archive->GetVector3(Format("key_%i_scale", keyIndex));
        Vector4 rotation = archive->GetVector4(Format("key_%i_rotation", keyIndex));
        keys[keyIndex].rotation = Quaternion(rotation.x, rotation.y, rotation.z, rotation.w);
    }
}

AnimationData* AnimationData::Clone() const
{
    AnimationData* copy = new AnimationData();

    copy->invPose = invPose;
    copy->duration = duration;
    copy->keys = keys;

    return copy;
}

void AnimationData::BakeTransform(const Matrix4& transform)
{
    for (auto& key : keys)
    {
        Matrix4 animationMatrix;
        key.GetMatrix(animationMatrix);

        Matrix4 inverseTotal;
        transform.GetInverse(inverseTotal);

        Matrix4 totalAnimation = inverseTotal * animationMatrix;
        totalAnimation.Decomposition(key.translation, key.scale, key.rotation);
    }
}
}
