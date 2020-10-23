#include "stdafx.h"
#include "ColladaSceneNode.h"
#include "Scene3D/SceneNodeAnimation.h"
#include "Utils/StringFormat.h"
#include <cmath>

namespace DAVA
{
Matrix4 ConvertMatrix(FMMatrix44& matrix)
{
    Matrix4 result;
    for (int k = 0; k < 4; ++k)
        for (int l = 0; l < 4; ++l)
            result._data[k][l] = matrix.m[k][l];

    return result;
}

Matrix4 ConvertMatrixT(FMMatrix44& matrix)
{
    Matrix4 result;
    for (int k = 0; k < 4; ++k)
        for (int l = 0; l < 4; ++l)
            result._data[k][l] = matrix.m[l][k];

    return result;
}

ColladaSceneNode::ColladaSceneNode(ColladaScene* _scene, FCDSceneNode* _node)
{
    originalNode = _node;
    localTransform.Identity();
    parent = 0;
    animation = 0;
    scene = _scene;
    inverse0.Identity();
}

ColladaSceneNode::~ColladaSceneNode()
{
}

void ColladaSceneNode::AddNode(ColladaSceneNode* node)
{
    childs.push_back(node);
    node->parent = this;
}

void ColladaSceneNode::AddMeshInstance(ColladaMeshInstance* meshInstance)
{
    meshInstances.push_back(meshInstance);
}

void ColladaSceneNode::PreProcessLights(ColladaLightState& state)
{
    Matrix4 localTransformTransposed = localTransform;

    glPushMatrix();
    glMultMatrixf(localTransformTransposed.data);

    for (int lightIndex = 0; lightIndex < (int)lights.size(); ++lightIndex)
    {
        ColladaLight* light = lights[lightIndex];
        light->ApplyLight(state);
    }

    for (int c = 0; c < (int)childs.size(); ++c)
        childs[c]->PreProcessLights(state);

    glPopMatrix();
}

void ColladaSceneNode::Render(Matrix4 currentMatrix)
{
#ifdef COLLADA_GLUT_RENDER
    worldTransform = localTransform * currentMatrix;
    //Matrix4 worldTransformCopy = worldTransform;
    //worldTransformCopy.Transpose(); */

    Matrix4 localTransformTransposed = localTransform;
    //localTransformTransposed.Transpose();

    glPushMatrix();
    glMultMatrixf(worldTransform.data);

    if (originalNode->GetJointFlag())
    {
        glDisable(GL_LIGHTING);
        glColor3f(0.984375, 0.078125, 0.64453125);
        //glutWireCube(0.5f);
        glEnable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    for (int m = 0; m < (int)meshInstances.size(); ++m)
    {
        meshInstances[m]->Render();
    }
    glPopMatrix();

    for (int c = 0; c < (int)childs.size(); ++c)
        childs[c]->Render(worldTransform);

#endif
}

void ColladaSceneNode::AddLight(ColladaLight* light)
{
    lights.push_back(light);
}

void ColladaSceneNode::UpdateTransforms(float time)
{
    /*FMMatrix44 colladaLocalMatrix;
	colladaLocalMatrix = FMMatrix44::Identity;// = FMMatrix44::Identity(); 

	for (int t = 0; t < originalNode->GetTransformCount(); ++t)
	{
		FCDTransform * transform = originalNode->GetTransform(t);
		if (transform->IsAnimated()) // process all animations to make CalculateWorldTransform work
		{
			FCDAnimated * animated = transform->GetAnimated();
			animated->Evaluate(time);
		}
		
		if (transform->GetType() == FCDTransform::TRANSLATION)
		{
			FCDTTranslation * translation = dynamic_cast<FCDTTranslation*>(transform);
			FMVector3 point = FMVector3(0.0f, 0.0f, 0.0f);
			point = translation->GetTranslation();
			if (transform->IsAnimated()) 
			{
				FCDAnimationCurve* curve;

				// look for x animation
				curve = transform->GetAnimated()->FindCurve(".X");
				if (curve != 0) 
					point.x = curve->Evaluate(time);

				// look for y animation
				curve = transform->GetAnimated()->FindCurve(".Y");
				if (curve != 0) 
					point.y = curve->Evaluate(time);

				// look for z animation
				curve = transform->GetAnimated()->FindCurve(".Z");
				if (curve != 0) 
					point.z = curve->Evaluate(time);
			}
			colladaLocalMatrix = colladaLocalMatrix * FMMatrix44::TranslationMatrix(point);
		}else if (transform->GetType() == FCDTransform::ROTATION)
		{
			FCDTRotation * rotation = dynamic_cast<FCDTRotation*>(transform);
			FMVector3 axis = FMVector3(0.0f, 0.0f, 0.0f);
			float angle = 0;
			axis = rotation->GetAxis();
			angle = rotation->GetAngle();

			if (rotation->IsAnimated()) 
			{
				FCDAnimationCurve* curve;

				// look for x animation
				curve = rotation->GetAnimated()->FindCurve(".X");
				if (curve != 0) 
					axis.x = curve->Evaluate(time);

				// look for y animation
				curve = rotation->GetAnimated()->FindCurve(".Y");
				if (curve != 0) 
					axis.y = curve->Evaluate(time);

				// look for z animation
				curve = rotation->GetAnimated()->FindCurve(".Z");
				if (curve != 0) 
					axis.z = curve->Evaluate(time);

				// look for z animation
				curve = rotation->GetAnimated()->FindCurve(".ANGLE");
				if (curve != 0) 
					angle = curve->Evaluate(time);
			}
			colladaLocalMatrix = colladaLocalMatrix * FMMatrix44::AxisRotationMatrix(axis, angle * PI / 180.0f);
		}else
		{
			colladaLocalMatrix = colladaLocalMatrix * transform->ToMatrix();
		}

	}*/

    //colladaLocalMatrix = originalNode->ToMatrix();
    //localTransform = ConvertMatrix(colladaLocalMatrix);

    if (animation)
    {
        SceneNodeAnimationKey& key = animation->Intepolate(time);
        key.GetMatrix(localTransform);
    }
    else
    {
        // merged:
        FMMatrix44 colladaLocalMatrix = ColladaSceneNode::CalculateTransformForTime(originalNode, time);
        localTransform = ConvertMatrix(colladaLocalMatrix);
    }

    for (int c = 0; c < (int)childs.size(); ++c)
        childs[c]->UpdateTransforms(time);
}

bool ColladaSceneNode::IsAnimated(FCDSceneNode* originalNode)
{
    for (int t = 0; t < (int)originalNode->GetTransformCount(); ++t)
    {
        FCDTransform* transform = originalNode->GetTransform(t);
        if (transform->IsAnimated()) // process all animations to make CalculateWorldTransform work
        {
            return true;
        }

        if (transform->GetType() == FCDTransform::TRANSLATION)
        {
            FCDTTranslation* translation = dynamic_cast<FCDTTranslation*>(transform);
            if (translation->IsAnimated())
            {
                return true;
            }
        }
        else if (transform->GetType() == FCDTransform::ROTATION)
        {
            FCDTRotation* rotation = dynamic_cast<FCDTRotation*>(transform);
            if (rotation->IsAnimated())
            {
                return true;
            }
        }
        else if (transform->GetType() == FCDTransform::SCALE)
        {
            FCDTScale* scale = dynamic_cast<FCDTScale*>(transform);
            if (scale->IsAnimated())
            {
                return true;
            }
        }
        else if (transform->GetType() == FCDTransform::MATRIX)
        {
            FCDTMatrix* matrix = dynamic_cast<FCDTMatrix*>(transform);
            if (matrix->IsAnimated())
            {
                return true;
            }
        }
    }
    return false;
}

FMMatrix44 ColladaSceneNode::CalculateTransformForTime(FCDSceneNode* originalNode, float32 time)
{
    FMMatrix44 colladaLocalMatrix;
    colladaLocalMatrix = FMMatrix44::Identity; // = FMMatrix44::Identity();

    for (int t = 0; t < (int)originalNode->GetTransformCount(); ++t)
    {
        FCDTransform* transform = originalNode->GetTransform(t);
        if (transform->IsAnimated()) // process all animations to make CalculateWorldTransform work
        {
            FCDAnimated* animated = transform->GetAnimated();
            animated->Evaluate(time);
        }

        if (transform->GetType() == FCDTransform::TRANSLATION)
        {
            FCDTTranslation* translation = dynamic_cast<FCDTTranslation*>(transform);
            FMVector3 point = FMVector3(0.0f, 0.0f, 0.0f);
            point = translation->GetTranslation();
            if (transform->IsAnimated())
            {
                FCDAnimationCurve* curve;

                // look for x animation
                curve = transform->GetAnimated()->FindCurve(".X");
                if (curve != 0)
                    point.x = curve->Evaluate(time);
                // look for y animation
                curve = transform->GetAnimated()->FindCurve(".Y");
                if (curve != 0)
                    point.y = curve->Evaluate(time);

                // look for z animation
                curve = transform->GetAnimated()->FindCurve(".Z");
                if (curve != 0)
                    point.z = curve->Evaluate(time);
            }
            colladaLocalMatrix = colladaLocalMatrix * FMMatrix44::TranslationMatrix(point);
        }
        else if (transform->GetType() == FCDTransform::ROTATION)
        {
            FCDTRotation* rotation = dynamic_cast<FCDTRotation*>(transform);
            FMVector3 axis = FMVector3(0.0f, 0.0f, 0.0f);
            float angle = 0;
            axis = rotation->GetAxis();
            angle = rotation->GetAngle();

            if (rotation->IsAnimated())
            {
                FCDAnimationCurve* curve;

                // look for x animation
                curve = rotation->GetAnimated()->FindCurve(".X");
                if (curve != 0)
                    axis.x = curve->Evaluate(time);

                // look for y animation
                curve = rotation->GetAnimated()->FindCurve(".Y");
                if (curve != 0)
                    axis.y = curve->Evaluate(time);

                // look for z animation
                curve = rotation->GetAnimated()->FindCurve(".Z");
                if (curve != 0)
                    axis.z = curve->Evaluate(time);

                // look for z animation
                curve = rotation->GetAnimated()->FindCurve(".ANGLE");
                if (curve != 0)
                    angle = curve->Evaluate(time);
            }
            colladaLocalMatrix = colladaLocalMatrix * FMMatrix44::AxisRotationMatrix(axis, angle * PI / 180.0f);
        }
        else if (transform->GetType() == FCDTransform::SCALE)
        {
            FCDTScale* scaleTransform = dynamic_cast<FCDTScale*>(transform);
            FMVector3 scale = FMVector3(1.0f, 1.0f, 1.0f);
            scale = scaleTransform->GetScale();

            if (scaleTransform->IsAnimated())
            {
                FCDAnimationCurve* curve;

                // look for x animation
                curve = scaleTransform->GetAnimated()->FindCurve(".X");
                if (curve != 0)
                    scale.x = curve->Evaluate(time);

                // look for y animation
                curve = scaleTransform->GetAnimated()->FindCurve(".Y");
                if (curve != 0)
                    scale.y = curve->Evaluate(time);

                // look for z animation
                curve = scaleTransform->GetAnimated()->FindCurve(".Z");
                if (curve != 0)
                    scale.z = curve->Evaluate(time);
            }
            colladaLocalMatrix = colladaLocalMatrix * FMMatrix44::ScaleMatrix(scale);
        }
        else if (transform->GetType() == FCDTransform::MATRIX)
        {
            FCDTMatrix* matrixTransform = dynamic_cast<FCDTMatrix*>(transform);
            FMMatrix44 matrix = transform->ToMatrix();

            if (matrixTransform->IsAnimated())
            {
                FCDAnimationCurve* curve;

                FCDAnimated* animated = matrixTransform->GetAnimated();

                for (int32 i = 0; i < 4; ++i)
                    for (int32 j = 0; j < 4; ++j)
                    {
                        curve = animated->FindCurve(Format("(%i)(%i)", i, j).c_str());
                        if (curve != 0)
                            matrix.m[i][j] = curve->Evaluate(time);
                    }
            }
            colladaLocalMatrix = colladaLocalMatrix * matrix;
        }
        else
        {
            colladaLocalMatrix = colladaLocalMatrix * transform->ToMatrix();
        }
    }
    return colladaLocalMatrix;
}

Matrix4 ColladaSceneNode::AccumulateTransformUptoFarParent(ColladaSceneNode* farParent) const
{
    if (farParent == this)
    {
        return localTransform;
    }

    return localTransform * parent->AccumulateTransformUptoFarParent(farParent);
}

SceneNodeAnimationKey ColladaSceneNode::ExportAnimationKey(FCDSceneNode* originalNode, float32 time)
{
    SceneNodeAnimationKey key;
    FMMatrix44 colladaLocalMatrix = ColladaSceneNode::CalculateTransformForTime(originalNode, time);
    Matrix4 lt = ConvertMatrix(colladaLocalMatrix);
    key.time = time;
    lt.Decomposition(key.translation, key.scale, key.rotation);

    return key;
}

bool ColladaSceneNode::KeyTimeEqual(float32 first, float32 second)
{
    return fabsf(first - second) <= EPSILON;
}

SceneNodeAnimation* ColladaSceneNode::ExportNodeAnimation(FCDSceneNode* originalNode, float32 startTime, float32 endTime, float32 fps)
{
    if (!originalNode->GetJointFlag() && !IsAnimated(originalNode))
        return 0;

    Vector<float32> keyTimes;
    // collect animation key times
    for (int transformIndex = 0; transformIndex < (int)originalNode->GetTransformCount(); ++transformIndex)
    {
        FCDTransform* transform = originalNode->GetTransform(transformIndex);
        if (transform->IsAnimated())
        {
            if ((transform->GetType() == FCDTransform::MATRIX) && (originalNode->GetTransformCount() > 1))
            {
                DVASSERT(false, "Multiple matrix animations are not supported.");
                return NULL;
            }

            FCDAnimated* animated = transform->GetAnimated();

            const FCDAnimationCurveListList& curves = animated->GetCurves();
            for (FCDAnimationCurveListList::const_iterator curveIter = curves.begin(); curveIter != curves.end(); ++curveIter)
            {
                for (FCDAnimationCurveTrackList::const_iterator curveTrackIter = curveIter->begin(); curveTrackIter != curveIter->end(); ++curveTrackIter)
                {
                    for (size_t keyIndex = 0; keyIndex < (*curveTrackIter)->GetKeyCount(); ++keyIndex)
                    {
                        float32 key = (*curveTrackIter)->GetKey(keyIndex)->input;

                        if (!std::binary_search(keyTimes.begin(), keyTimes.end(), key))
                        {
                            keyTimes.insert(std::lower_bound(keyTimes.begin(), keyTimes.end(), key), key);
                        }
                    }
                }
            }
        }
        else
        {
            if (!std::binary_search(keyTimes.begin(), keyTimes.end(), 0.f))
            {
                keyTimes.insert(std::lower_bound(keyTimes.begin(), keyTimes.end(), 0.f), 0.f);
            }
        }
    }

    Vector<float32>::iterator last = std::unique(keyTimes.begin(), keyTimes.end(), KeyTimeEqual);
    keyTimes.erase(last, keyTimes.end());

    SceneNodeAnimation* anim = new SceneNodeAnimation(static_cast<uint32>(keyTimes.size()));
    anim->SetDuration(endTime);
    for (int k = 0; k < (int)keyTimes.size(); ++k)
    {
        anim->SetKey(k, ExportAnimationKey(originalNode, keyTimes[k]));
    }

    printf("= keys export: keyCount:%u\n", static_cast<uint32>(keyTimes.size()));

    return anim;
}

ColladaSceneNode* ColladaSceneNode::FindNode(const fm::string& daeId)
{
    if (originalNode->GetDaeId() == daeId)
        return this;

    for (int k = 0; k < (int)childs.size(); ++k)
    {
        ColladaSceneNode* node = childs[k]->FindNode(daeId);
        if (node != nullptr)
            return node;
    }

    return nullptr;
}

void ColladaSceneNode::SetAnimation(SceneNodeAnimation* _animation, bool recursive)
{
    animation = _animation;
    if (recursive)
    {
        for (int k = 0; k < (int)childs.size(); ++k)
        {
            ColladaSceneNode* node = childs[k];
            node->SetAnimation(_animation, recursive);
        }
    }
}

void ColladaSceneNode::AddCamera(ColladaCamera* cam)
{
    cameras.push_back(cam);
}
};
