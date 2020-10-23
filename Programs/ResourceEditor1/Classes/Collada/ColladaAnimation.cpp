#include "stdafx.h"
#include "ColladaAnimation.h"
#include "ColladaScene.h"
#include "ColladaSceneNode.h"

namespace DAVA
{
ColladaAnimation::ColladaAnimation()
{
}

ColladaAnimation::~ColladaAnimation()
{
}

void ColladaAnimation::Assign()
{
    for (Map<ColladaSceneNode*, SceneNodeAnimation*>::iterator it = animations.begin(); it != animations.end(); ++it)
    {
        ColladaSceneNode* node = it->first;
        SceneNodeAnimation* anim = it->second;
        node->SetAnimation(anim);
    }
}

Matrix4 ColladaAnimation::EvaluateMatrix(FCDTransform* transform, float32 time)
{
    DVASSERT(transform);
    DVASSERT(transform->GetType() == FCDTransform::MATRIX);

    FMMatrix44 result = transform->ToMatrix();
    if (transform->IsAnimated())
    {
        FCDAnimated* animated = transform->GetAnimated();
        for (int32 i = 0; i < 4; ++i)
        {
            for (int32 j = 0; j < 4; ++j)
            {
                FCDAnimationCurve* curve = animated->FindCurve(Format("(%i)(%i)", i, j).c_str());
                result.m[i][j] = (curve != nullptr) ? curve->Evaluate(time) : result.m[i][j];
            }
        }
    }

    return ConvertMatrix(result);
}

Vector3 ColladaAnimation::EvaluateTranslation(FCDTransform* transform, float32 time)
{
    DVASSERT(transform);
    DVASSERT(transform->GetType() == FCDTransform::TRANSLATION);

    FMVector3 result = static_cast<FCDTTranslation*>(transform)->GetTranslation();
    if (transform->IsAnimated())
    {
        FCDAnimated* animated = transform->GetAnimated();
        FCDAnimationCurve* curveX = animated->FindCurve(".X");
        FCDAnimationCurve* curveY = animated->FindCurve(".Y");
        FCDAnimationCurve* curveZ = animated->FindCurve(".Z");

        result.x = (curveX != nullptr) ? curveX->Evaluate(time) : result.x;
        result.y = (curveY != nullptr) ? curveY->Evaluate(time) : result.y;
        result.z = (curveZ != nullptr) ? curveZ->Evaluate(time) : result.z;
    }

    return Vector3(static_cast<float32*>(result));
}

Quaternion ColladaAnimation::EvaluateRotation(FCDTransform* transform, float32 time)
{
    DVASSERT(transform);
    DVASSERT(transform->GetType() == FCDTransform::ROTATION);

    FMAngleAxis angleAxis = static_cast<FCDTRotation*>(transform)->GetAngleAxis();
    if (transform->IsAnimated())
    {
        FCDAnimated* animated = transform->GetAnimated();
        FCDAnimationCurve* curveX = animated->FindCurve(".X");
        FCDAnimationCurve* curveY = animated->FindCurve(".Y");
        FCDAnimationCurve* curveZ = animated->FindCurve(".Z");
        FCDAnimationCurve* curveAngle = animated->FindCurve(".ANGLE");

        angleAxis.axis.x = (curveX != nullptr) ? curveX->Evaluate(time) : angleAxis.axis.x;
        angleAxis.axis.y = (curveY != nullptr) ? curveY->Evaluate(time) : angleAxis.axis.y;
        angleAxis.axis.z = (curveZ != nullptr) ? curveZ->Evaluate(time) : angleAxis.axis.z;
        angleAxis.angle = (curveAngle != nullptr) ? curveAngle->Evaluate(time) : angleAxis.angle;
    }

    return Quaternion::MakeRotation(Vector3(static_cast<float32*>(angleAxis.axis)), DegToRad(angleAxis.angle));
}

Vector3 ColladaAnimation::EvaluateScale(FCDTransform* transform, float32 time)
{
    DVASSERT(transform);
    DVASSERT(transform->GetType() == FCDTransform::SCALE);

    FMVector3 result = static_cast<FCDTScale*>(transform)->GetScale();
    if (transform->IsAnimated())
    {
        FCDAnimated* animated = transform->GetAnimated();
        FCDAnimationCurve* curveX = animated->FindCurve(".X");
        FCDAnimationCurve* curveY = animated->FindCurve(".Y");
        FCDAnimationCurve* curveZ = animated->FindCurve(".Z");

        result.x = (curveX != nullptr) ? curveX->Evaluate(time) : result.x;
        result.y = (curveY != nullptr) ? curveY->Evaluate(time) : result.y;
        result.z = (curveZ != nullptr) ? curveZ->Evaluate(time) : result.z;
    }

    return Vector3(static_cast<float32*>(result));
}

void ColladaAnimation::CollectAnimationKeys(FCDAnimationCurve* curve, TimeStampSet* timeStamps)
{
    if (curve == nullptr)
    {
        timeStamps->insert(0.f);
        return;
    }

    size_t keyCount = curve->GetKeyCount();
    FCDAnimationKey** keys = curve->GetKeys();
    for (size_t k = 0; k < keyCount; ++k)
        timeStamps->insert(keys[k]->input);
}

void ColladaAnimation::CollectAnimationKeys(FCDSceneNode* node, ColladaAnimatinData* data)
{
    TimeStampSet translationKeys;
    TimeStampSet rotationKeys;
    TimeStampSet scaleKeys;
    TimeStampSet mxKeys;

    for (size_t t = 0; t < node->GetTransformCount(); ++t)
    {
        FCDTransform* transform = node->GetTransform(t);
        FCDAnimated* animated = transform->GetAnimated();

        if (transform->GetType() == FCDTransform::MATRIX)
        {
            FCDAnimationCurve* mxCurve = transform->IsAnimated() ? animated->FindCurve("(0)(0)") : nullptr;
            CollectAnimationKeys(mxCurve, &mxKeys);

            translationKeys = mxKeys;
            rotationKeys = mxKeys;
            scaleKeys = mxKeys;
        }
        else
        {
            Array<FCDAnimationCurve*, 4> curves = {
                transform->IsAnimated() ? animated->FindCurve(".X") : nullptr,
                transform->IsAnimated() ? animated->FindCurve(".Y") : nullptr,
                transform->IsAnimated() ? animated->FindCurve(".Z") : nullptr,
                transform->IsAnimated() ? animated->FindCurve(".ANGLE") : nullptr
            };

            if (transform->GetType() == FCDTransform::TRANSLATION)
            {
                CollectAnimationKeys(curves, &translationKeys);
            }
            else if (transform->GetType() == FCDTransform::ROTATION)
            {
                CollectAnimationKeys(curves, &rotationKeys);
            }
            else if (transform->GetType() == FCDTransform::SCALE)
            {
                CollectAnimationKeys(curves, &scaleKeys);
            }
        }
    }

    for (float32 key : translationKeys)
        data->translations.emplace_back(key, Vector3());

    for (float32 key : rotationKeys)
        data->rotations.emplace_back(key, Quaternion());

    for (float32 key : scaleKeys)
        data->scales.emplace_back(key, Vector3(1.f, 1.f, 1.f));
}

void ColladaAnimation::EvaluateAnimationData(FCDSceneNode* node, ColladaAnimatinData* data)
{
    for (size_t t = 0; t < node->GetTransformCount(); ++t)
    {
        FCDTransform* transform = node->GetTransform(t);
        if (transform->GetType() == FCDTransform::MATRIX)
        {
            for (auto& t : data->translations)
                t.second += EvaluateMatrix(transform, t.first).GetTranslationVector();

            for (auto& r : data->rotations)
                r.second *= EvaluateMatrix(transform, r.first).GetRotation();

            for (auto& s : data->scales)
                s.second *= EvaluateMatrix(transform, s.first).GetScaleVector();
        }
        else
        {
            if (transform->GetType() == FCDTransform::TRANSLATION)
            {
                for (auto& t : data->translations)
                    t.second += EvaluateTranslation(transform, t.first);
            }
            else if (transform->GetType() == FCDTransform::ROTATION)
            {
                for (auto& r : data->rotations)
                    r.second *= EvaluateRotation(transform, r.first);
            }
            else if (transform->GetType() == FCDTransform::SCALE)
            {
                for (auto& s : data->scales)
                    s.second *= EvaluateScale(transform, s.first);
            }
        }
    }
}

void ColladaAnimation::ExportAnimationData(ColladaSceneNode* node, ColladaAnimatinData* data)
{
    CollectAnimationKeys(node->originalNode, data);
    EvaluateAnimationData(node->originalNode, data);

    //for skeleton-root node bake transform of parent
    if (node->parent != nullptr && node->originalNode->GetJointFlag() && !node->parent->originalNode->GetJointFlag())
    {
        Matrix4 pTransform = node->parent->AccumulateTransformUptoFarParent(node->scene->rootNode);
        Vector3 parentTranslation, parentScale;
        Quaternion parentRotation;

        pTransform.Decomposition(parentTranslation, parentScale, parentRotation);
        parentRotation.Normalize();

        for (auto& t : data->translations)
            t.second = parentTranslation + parentRotation.ApplyToVectorFast(t.second) * parentScale;

        for (auto& r : data->rotations)
            r.second = parentRotation * r.second;

        for (auto& s : data->scales)
            s.second *= parentScale;
    }
}
};
