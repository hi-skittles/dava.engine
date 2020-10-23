#ifndef __DAVAENGINE_DYNAMIC_BINDINGS_H__
#define __DAVAENGINE_DYNAMIC_BINDINGS_H__

#include "Math/Color.h"
#include "Math/Matrix4.h"
#include "Math/Vector.h"
#include "Base/FastName.h"
#include "Render/RHI/rhi_Public.h"

namespace DAVA
{
class DynamicBindings
{
public:
    enum eUniformSemantic
    {
        UNKNOWN_SEMANTIC = 0,

        PARAM_WORLD,
        PARAM_INV_WORLD,
        PARAM_WORLD_INV_TRANSPOSE,
        PARAM_VIEW,
        PARAM_INV_VIEW,
        PARAM_PROJ,
        PARAM_INV_PROJ,

        PARAM_WORLD_VIEW,
        PARAM_INV_WORLD_VIEW,
        PARAM_WORLD_VIEW_INV_TRANSPOSE, //NORMAL, // NORMAL MATRIX

        PARAM_VIEW_PROJ,
        PARAM_INV_VIEW_PROJ,

        PARAM_WORLD_VIEW_PROJ,
        PARAM_INV_WORLD_VIEW_PROJ,

        PARAM_GLOBAL_TIME,
        PARAM_WORLD_SCALE,

        PARAM_CAMERA_POS,
        PARAM_CAMERA_DIR,
        PARAM_CAMERA_UP,

        PARAM_LIGHT0_POSITION,
        PARAM_LIGHT0_COLOR,
        PARAM_LIGHT0_AMBIENT_COLOR,

        PARAM_LOCAL_BOUNDING_BOX,
        PARAM_WORLD_VIEW_OBJECT_CENTER,
        PARAM_BOUNDING_BOX_SIZE,

        PARAM_SPEED_TREE_TRUNK_OSCILLATION,
        PARAM_SPEED_TREE_LEAFS_OSCILLATION,
        PARAM_SPEED_TREE_LIGHT_SMOOTHING,

        PARAM_SPHERICAL_HARMONICS,

        PARAM_JOINT_POSITIONS,
        PARAM_JOINT_QUATERNIONS,
        PARAM_JOINTS_COUNT, //it will not be bound into shader, but will be used to bind joints

        PARAM_VIEWPORT_SIZE,
        PARAM_RCP_VIEWPORT_SIZE, // = 1/PARAM_VIEWPORT_SIZE
        PARAM_VIEWPORT_OFFSET,

        PARAM_LANDSCAPE_HEIGHTMAP_TEXTURE_SIZE,

        PARAM_SHADOW_COLOR,
        PARAM_WATER_CLEAR_COLOR,

        PARAM_PROJECTION_FLIP, //1.0 regular, -1.0 if projection matrix is y-inverted (rendering to RT with lower left origin API)

        AUTOBIND_UNIFORMS_END,

        DYNAMIC_PARAMETERS_COUNT = AUTOBIND_UNIFORMS_END,
    };

    enum
    {
        UPDATE_SEMANTIC_ALWAYS = 0,
    };

public:
    static DynamicBindings::eUniformSemantic GetUniformSemanticByName(const FastName& name);

    void SetDynamicParam(eUniformSemantic shaderSemantic, const void* value, pointer_size updateSemantic);
    const void* GetDynamicParam(eUniformSemantic shaderSemantic);
    pointer_size GetDynamicParamUpdateSemantic(eUniformSemantic shaderSemantic);
    uint32 GetDynamicParamArraySize(eUniformSemantic shaderSemantic, uint32 defaultValue = 1);
    inline const Matrix4& GetDynamicParamMatrix(eUniformSemantic shaderSemantic);

private:
    struct AutobindVariableData
    {
        pointer_size updateSemantic; // Use lower 1 bit, for indication of update
        const void* value;
    };
    AutobindVariableData dynamicParameters[DYNAMIC_PARAMETERS_COUNT];
    uint32 dynamicParamersRequireUpdate;

    Matrix4 worldViewMatrix;
    Matrix4 viewProjMatrix;
    Matrix4 worldViewProjMatrix;
    Matrix4 invWorldViewMatrix;
    Matrix4 normalMatrix;
    Matrix4 invWorldMatrix;
    Matrix4 worldInvTransposeMatrix;
    Vector3 worldScale;
    Vector3 boundingBoxSize;
    Vector3 worldViewObjectCenter;

    void ComputeWorldViewMatrixIfRequired();
    void ComputeWorldScaleIfRequired();
    void ComputeViewProjMatrixIfRequired();
    void ComputeWorldViewProjMatrixIfRequired();
    void ComputeInvWorldViewMatrixIfRequired();
    void ComputeWorldViewInvTransposeMatrixIfRequired();
    void ComputeLocalBoundingBoxSizeIfRequired();
    void ComputeWorldViewObjectCenterIfRequired();

    void ComputeInvWorldMatrixIfRequired();
    void ComputeWorldInvTransposeMatrixIfRequired();
};

inline const Matrix4& DynamicBindings::GetDynamicParamMatrix(DynamicBindings::eUniformSemantic shaderSemantic)
{
    return *static_cast<const Matrix4*>(GetDynamicParam(shaderSemantic));
}
}
#endif
