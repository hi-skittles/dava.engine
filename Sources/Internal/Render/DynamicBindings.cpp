#include "DynamicBindings.h"

#include "Math/AABBox3.h"
#include "Time/SystemTimer.h"
#include "Render/RenderBase.h"

#include "Engine/Engine.h"

namespace DAVA
{
namespace
{
Array<FastName, DynamicBindings::DYNAMIC_PARAMETERS_COUNT> DYNAMIC_PARAM_NAMES;

void InitDynamicParamNames()
{
    if (DYNAMIC_PARAM_NAMES[0].IsValid() == false)
    {
        DYNAMIC_PARAM_NAMES = {
            FastName("unknownSemantic"),
            FastName("worldMatrix"), //PARAM_WORLD,
            FastName("invWorldMatrix"), //PARAM_INV_WORLD,
            FastName("worldInvTransposeMatrix"), //PARAM_WORLD_INV_TRANSPOSE,

            FastName("viewMatrix"), //PARAM_VIEW,
            FastName("invViewMatrix"), //PARAM_INV_VIEW,
            FastName("projMatrix"), //PARAM_PROJ,
            FastName("invProjMatrix"), //PARAM_INV_PROJ,

            FastName("worldViewMatrix"), //PARAM_WORLD_VIEW,
            FastName("invWorldViewMatrix"), //PARAM_INV_WORLD_VIEW,
            FastName("worldViewInvTransposeMatrix"), //PARAM_NORMAL, // NORMAL MATRIX

            FastName("viewProjMatrix"), //PARAM_VIEW_PROJ,
            FastName("invViewProjMatrix"), //PARAM_INV_VIEW_PROJ,

            FastName("worldViewProjMatrix"), //PARAM_WORLD_VIEW_PROJ,
            FastName("invWorldViewProjMatrix"), //PARAM_INV_WORLD_VIEW_PROJ,

            FastName("globalTime"),
            FastName("worldScale"),

            FastName("cameraPosition"), // PARAM_CAMERA_POS,
            FastName("cameraDirection"), // PARAM_CAMERA_DIR,
            FastName("cameraUp"), // PARAM_CAMERA_UP,

            FastName("lightPosition0"),
            FastName("lightColor0"),
            FastName("lightAmbientColor0"),

            FastName("localBoundingBox"),
            FastName("worldViewObjectCenter"),
            FastName("boundingBoxSize"),

            FastName("trunkOscillationParams"),
            FastName("leafOscillationParams"),
            FastName("speedTreeLightSmoothing"),

            FastName("sphericalHarmonics"),

            FastName("jointPositions"),
            FastName("jointQuaternions"),
            FastName("jointsCount"),

            FastName("viewportSize"),
            FastName("rcpViewportSize"),
            FastName("viewportOffset"),

            FastName("heightmapTextureSize"),

            FastName("shadowColor"),
            FastName("waterClearColor"),

            FastName("projectionFlip")
        };
    }
}
}

DynamicBindings::eUniformSemantic DynamicBindings::GetUniformSemanticByName(const FastName& name)
{
    InitDynamicParamNames();
    for (int32 k = 0; k < DYNAMIC_PARAMETERS_COUNT; ++k)
        if (name == DYNAMIC_PARAM_NAMES[k])
            return static_cast<eUniformSemantic>(k);
    return UNKNOWN_SEMANTIC;
}

void DynamicBindings::SetDynamicParam(DynamicBindings::eUniformSemantic shaderSemantic, const void* value, pointer_size _updateSemantic)
{
    //AutobindVariableData * var = &dynamicParameters[shaderSemantic];
    //if (var->updateSemantic
    if (_updateSemantic == UPDATE_SEMANTIC_ALWAYS || dynamicParameters[shaderSemantic].updateSemantic != _updateSemantic)
    {
        if (_updateSemantic == UPDATE_SEMANTIC_ALWAYS)
            dynamicParameters[shaderSemantic].updateSemantic++;
        else
            dynamicParameters[shaderSemantic].updateSemantic = _updateSemantic;

        dynamicParameters[shaderSemantic].value = value;
        dynamicParamersRequireUpdate &= ~(1ull << shaderSemantic);

        switch (shaderSemantic)
        {
        case PARAM_WORLD:
            dynamicParamersRequireUpdate |= ((1 << PARAM_INV_WORLD) | (1 << PARAM_WORLD_VIEW) | (1 << PARAM_INV_WORLD_VIEW) | (1 << PARAM_WORLD_VIEW_OBJECT_CENTER) | (1 << PARAM_WORLD_VIEW_PROJ) | (1 << PARAM_INV_WORLD_VIEW_PROJ) | (1 << PARAM_WORLD_VIEW_INV_TRANSPOSE) | (1 << PARAM_WORLD_INV_TRANSPOSE) | (1 << PARAM_WORLD_SCALE));
            break;
        case PARAM_VIEW:
            dynamicParamersRequireUpdate |= ((1 << PARAM_INV_VIEW) | (1 << PARAM_WORLD_VIEW) | (1 << PARAM_INV_WORLD_VIEW) | (1 << PARAM_WORLD_VIEW_PROJ) | (1 << PARAM_INV_WORLD_VIEW_PROJ) | (1 << PARAM_VIEW_PROJ) | (1 << PARAM_INV_VIEW_PROJ) | (1 << PARAM_WORLD_VIEW_INV_TRANSPOSE) | (1 << PARAM_WORLD_VIEW_OBJECT_CENTER));
            break;
        case PARAM_PROJ:
            dynamicParamersRequireUpdate |= ((1 << PARAM_INV_PROJ) | (1 << PARAM_VIEW_PROJ) | (1 << PARAM_INV_VIEW_PROJ) |
                                             (1 << PARAM_WORLD_VIEW_PROJ) | (1 << PARAM_INV_WORLD_VIEW_PROJ));
            break;
        case PARAM_LOCAL_BOUNDING_BOX:
            dynamicParamersRequireUpdate |= (1 << PARAM_BOUNDING_BOX_SIZE) | (1 << PARAM_WORLD_VIEW_OBJECT_CENTER);
        default:
            break;
        }
    }
}

void DynamicBindings::ComputeWorldViewMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_WORLD_VIEW))
    {
        worldViewMatrix = GetDynamicParamMatrix(PARAM_WORLD) * GetDynamicParamMatrix(PARAM_VIEW);
        SetDynamicParam(PARAM_WORLD_VIEW, &worldViewMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}

void DynamicBindings::ComputeWorldViewObjectCenterIfRequired()
{
    ComputeWorldViewMatrixIfRequired();
    if (dynamicParamersRequireUpdate & (1 << PARAM_WORLD_VIEW_OBJECT_CENTER))
    {
        const AABBox3* objectBox = reinterpret_cast<const AABBox3*>(GetDynamicParam(PARAM_LOCAL_BOUNDING_BOX));
        const Matrix4& worldView = GetDynamicParamMatrix(PARAM_WORLD_VIEW);
        worldViewObjectCenter = objectBox->GetCenter() * worldView;
        SetDynamicParam(PARAM_WORLD_VIEW_OBJECT_CENTER, &worldViewObjectCenter, UPDATE_SEMANTIC_ALWAYS);
    }
}

void DynamicBindings::ComputeWorldScaleIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_WORLD_SCALE))
    {
        worldScale = GetDynamicParamMatrix(PARAM_WORLD).GetScaleVector();

        SetDynamicParam(PARAM_WORLD_SCALE, &worldScale, UPDATE_SEMANTIC_ALWAYS);
    }
}

inline void DynamicBindings::ComputeLocalBoundingBoxSizeIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_BOUNDING_BOX_SIZE))
    {
        const AABBox3* objectBox = reinterpret_cast<const AABBox3*>(GetDynamicParam(PARAM_LOCAL_BOUNDING_BOX));
        boundingBoxSize = objectBox->GetSize();

        SetDynamicParam(PARAM_BOUNDING_BOX_SIZE, &boundingBoxSize, UPDATE_SEMANTIC_ALWAYS);
    }
}

inline void DynamicBindings::ComputeViewProjMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_VIEW_PROJ))
    {
        viewProjMatrix = GetDynamicParamMatrix(PARAM_VIEW) * GetDynamicParamMatrix(PARAM_PROJ);
        SetDynamicParam(PARAM_VIEW_PROJ, &viewProjMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}

inline void DynamicBindings::ComputeWorldViewProjMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_WORLD_VIEW_PROJ))
    {
        ComputeViewProjMatrixIfRequired();
        worldViewProjMatrix = GetDynamicParamMatrix(PARAM_WORLD) * GetDynamicParamMatrix(PARAM_VIEW_PROJ);
        SetDynamicParam(PARAM_WORLD_VIEW_PROJ, &worldViewProjMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}

inline void DynamicBindings::ComputeInvWorldViewMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_INV_WORLD_VIEW))
    {
        ComputeWorldViewMatrixIfRequired();
        worldViewMatrix.GetInverse(invWorldViewMatrix);
        SetDynamicParam(PARAM_INV_WORLD_VIEW, &invWorldViewMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}

inline void DynamicBindings::ComputeWorldViewInvTransposeMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_WORLD_VIEW_INV_TRANSPOSE))
    {
        ComputeInvWorldViewMatrixIfRequired();
        normalMatrix = invWorldViewMatrix;
        normalMatrix.Transpose();
        SetDynamicParam(PARAM_WORLD_VIEW_INV_TRANSPOSE, &normalMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}

inline void DynamicBindings::ComputeInvWorldMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_INV_WORLD))
    {
        const Matrix4& worldMatrix = GetDynamicParamMatrix(PARAM_WORLD);
        worldMatrix.GetInverse(invWorldMatrix);
        SetDynamicParam(PARAM_INV_WORLD, &invWorldMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}

inline void DynamicBindings::ComputeWorldInvTransposeMatrixIfRequired()
{
    if (dynamicParamersRequireUpdate & (1 << PARAM_WORLD_INV_TRANSPOSE))
    {
        ComputeInvWorldMatrixIfRequired();
        worldInvTransposeMatrix = invWorldMatrix;
        worldInvTransposeMatrix.Transpose();
        SetDynamicParam(PARAM_WORLD_INV_TRANSPOSE, &worldInvTransposeMatrix, UPDATE_SEMANTIC_ALWAYS);
    }
}

uint32 DynamicBindings::GetDynamicParamArraySize(DynamicBindings::eUniformSemantic shaderSemantic, uint32 defaultValue)
{
    if ((shaderSemantic == PARAM_JOINT_POSITIONS) || (shaderSemantic == PARAM_JOINT_QUATERNIONS))
        return *(reinterpret_cast<const uint32*>(GetDynamicParam(PARAM_JOINTS_COUNT)));
    else
        return defaultValue;
}

const void* DynamicBindings::GetDynamicParam(eUniformSemantic shaderSemantic)
{
    switch (shaderSemantic)
    {
    case PARAM_WORLD_VIEW_PROJ:
        ComputeWorldViewProjMatrixIfRequired();
        break;
    case PARAM_WORLD_VIEW:
        ComputeWorldViewMatrixIfRequired();
        break;
    case PARAM_WORLD_SCALE:
        ComputeWorldScaleIfRequired();
        break;
    case PARAM_INV_WORLD_VIEW:
        ComputeInvWorldViewMatrixIfRequired();
        break;
    case PARAM_WORLD_VIEW_INV_TRANSPOSE:
        ComputeWorldViewInvTransposeMatrixIfRequired();
        break;
    case PARAM_WORLD_INV_TRANSPOSE:
        ComputeWorldInvTransposeMatrixIfRequired();
        break;
    case PARAM_WORLD_VIEW_OBJECT_CENTER:
        ComputeWorldViewObjectCenterIfRequired();
        break;
    case PARAM_BOUNDING_BOX_SIZE:
        ComputeLocalBoundingBoxSizeIfRequired();
        break;
    default:
        break;
    }
    DVASSERT(dynamicParameters[shaderSemantic].value != 0);
    return dynamicParameters[shaderSemantic].value;
}

pointer_size DynamicBindings::GetDynamicParamUpdateSemantic(eUniformSemantic shaderSemantic)
{
    return dynamicParameters[shaderSemantic].updateSemantic;
}
}