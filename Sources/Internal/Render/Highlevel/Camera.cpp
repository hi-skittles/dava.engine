#include "Render/Highlevel/Camera.h"
#include "Render/RenderBase.h"
#include "Scene3D/Scene.h"
#include "Scene3D/SceneFileV2.h"
#include "Render/Renderer.h"
#include "Reflection/ReflectionRegistrator.h"
#include "Reflection/ReflectedMeta.h"
#include "Base/GlobalEnum.h"

ENUM_DECLARE(DAVA::Camera::eFlags)
{
    ENUM_ADD_DESCR(DAVA::Camera::REQUIRE_REBUILD, "Require rebuild");
    ENUM_ADD_DESCR(DAVA::Camera::REQUIRE_REBUILD_MODEL, "Require rebuild model");
    ENUM_ADD_DESCR(DAVA::Camera::REQUIRE_REBUILD_PROJECTION, "Require rebuild projection");
    ENUM_ADD_DESCR(DAVA::Camera::REQUIRE_REBUILD_UNIFORM_PROJ_MODEL, "Require rebuild uniform projection model");
}

namespace DAVA
{
DAVA_VIRTUAL_REFLECTION_IMPL(Camera)
{
    ReflectionRegistrator<Camera>::Begin()
    .Field("aspect", &Camera::GetAspect, &Camera::SetAspect)[M::DisplayName("Aspect"), M::Range(0.0001f, Any(), 1.0f)]
    .Field("znear", &Camera::GetZNear, &Camera::SetZNear)[M::DisplayName("Near plane"), M::Range(0.0001f, Any(), 1.0f)]
    .Field("zfar", &Camera::GetZFar, &Camera::SetZFar)[M::DisplayName("Far plane"), M::Range(0.0001f, Any(), 1.0f)]
    .Field("fovx", &Camera::GetFOV, &Camera::SetFOV)[M::DisplayName("FovX"), M::Range(1.0f, 179.0f, 1.0f)]
    .Field("ortho", &Camera::GetIsOrtho, &Camera::SetIsOrtho)[M::DisplayName("Is Ortho")]
    .Field("orthoWidth", &Camera::GetOrthoWidth, &Camera::SetOrthoWidth)[M::DisplayName("Ortho Width")]
    .Field("position", &Camera::GetPosition, &Camera::SetPosition)[M::DisplayName("Position")]
    .Field("target", &Camera::GetTarget, &Camera::SetTarget)[M::DisplayName("Target")]
    .Field("up", &Camera::GetUp, &Camera::SetUp)[M::DisplayName("Up")]
    .Field("left", &Camera::GetLeft, &Camera::SetLeft)[M::DisplayName("Left")]
    .Field("direction", &Camera::direction)[M::ReadOnly(), M::DisplayName("Direction")]
    .Field("flags", &Camera::flags)[M::ReadOnly(), M::DisplayName("Flags"), M::FlagsT<DAVA::Camera::eFlags>(), M::DeveloperModeOnly()]
    .Field("cameraTransform", &Camera::cameraTransform)[M::ReadOnly(), M::DisplayName("Transform"), M::DeveloperModeOnly()]
    .Field("viewMatrix", &Camera::viewMatrix)[M::ReadOnly(), M::DisplayName("View"), M::DeveloperModeOnly()]
    .Field("projMatrix", &Camera::projMatrix)[M::ReadOnly(), M::DisplayName("Projection"), M::DeveloperModeOnly()]
    .End();
}

Camera::Camera()
    : orthoWidth(35.f)
{
    SetupPerspective(35.0f, 1.0f, 1.0f, 2500.f);
    up = Vector3(0.0f, 1.0f, 0.0f);
    left = Vector3(1.0f, 0.0f, 0.0f);
    flags = REQUIRE_REBUILD | REQUIRE_REBUILD_MODEL | REQUIRE_REBUILD_PROJECTION;

    cameraTransform.Identity();
    currentFrustum = new Frustum();
}

Camera::~Camera()
{
    SafeRelease(currentFrustum);
}

void Camera::SetFOV(const float32& fovxInDegrees)
{
    fovX = fovxInDegrees;
    Recalc();
}

void Camera::SetAspect(const float32& _aspect)
{
    aspect = 1.f / _aspect;
    Recalc();
}

void Camera::SetZNear(const float32& _zNear)
{
    znear = _zNear;
    Recalc();
}

void Camera::SetZFar(const float32& _zFar)
{
    zfar = _zFar;
    Recalc();
}

void Camera::SetIsOrtho(const bool& _ortho)
{
    ortho = _ortho;
    Recalc();
}

void Camera::SetOrthoWidth(const float32& width)
{
    orthoWidth = width;
    Recalc();
}

float32 Camera::GetOrthoWidth() const
{
    return orthoWidth;
}

void Camera::SetXMin(const float32& _xmin)
{
    xmin = _xmin;
}

void Camera::SetXMax(const float32& _xmax)
{
    xmax = _xmax;
}

void Camera::SetYMin(const float32& _ymin)
{
    ymin = _ymin;
}

void Camera::SetYMax(const float32& _ymax)
{
    ymax = _ymax;
}

float32 Camera::GetXMin() const
{
    return xmin;
}

float32 Camera::GetXMax() const
{
    return xmax;
}

float32 Camera::GetYMin() const
{
    return ymin;
}

float32 Camera::GetYMax() const
{
    return ymax;
}

float32 Camera::GetFOV() const
{
    return fovX;
}

float32 Camera::GetAspect() const
{
    return 1.f / aspect;
}

float32 Camera::GetZNear() const
{
    return znear;
}

float32 Camera::GetZFar() const
{
    return zfar;
}

bool Camera::GetIsOrtho() const
{
    return ortho;
}

void Camera::SetupPerspective(float32 fovxInDegrees, float32 aspectYdivX, float32 zNear, float32 zFar)
{
    this->aspect = 1.f / aspectYdivX;

    this->fovX = fovxInDegrees;
    this->znear = zNear;
    this->zfar = zFar;
    this->ortho = false;

    Recalc();
}

void Camera::SetupOrtho(float32 width, float32 aspectYdivX, float32 zNear, float32 zFar)
{
    this->aspect = 1.f / aspectYdivX;

    this->orthoWidth = width;
    this->znear = zNear;
    this->zfar = zFar;
    this->ortho = true;

    Recalc();
}

void Camera::Setup(float32 _xmin, float32 _xmax, float32 _ymin, float32 _ymax, float32 _znear, float32 _zfar)
{
    xmin = _xmin;
    xmax = _xmax;
    ymin = _ymin;
    ymax = _ymax;
    znear = _znear;
    zfar = _zfar;
    ValidateProperties();
}

void Camera::ValidateProperties()
{
    const float minZFarZNearDifference = 0.01f;
    const float minZNear = 0.01f;
    const float minFOV = 0.01f; // in degrees
    const float maxFOV = 179.99f; // in degrees
    const float minAspect = 0.0001f;
    const float maxAspect = 10000.0f;

    if (znear < minZNear)
    {
        znear = minZNear;
    }

    if (zfar < znear + minZFarZNearDifference)
    {
        zfar = znear + minZFarZNearDifference;
    }

    fovX = (fovX < minFOV) ? minFOV : (fovX > maxFOV ? maxFOV : fovX);
    aspect = (aspect < minAspect) ? minAspect : (aspect > maxAspect ? maxAspect : aspect);
}

void Camera::Recalc()
{
    ValidateProperties();

    flags |= REQUIRE_REBUILD_PROJECTION;

    if (ortho)
    {
        xmax = orthoWidth / 2.f;
        xmin = -xmax;

        ymax = xmax * aspect;
        ymin = xmin * aspect;
    }
    else
    {
        xmax = znear * tanf(fovX * PI / 360.0f);
        xmin = -xmax;

        ymax = xmax * aspect;
        ymin = xmin * aspect;
    }

    CalculateZoomFactor();
}

Vector2 Camera::GetOnScreenPosition(const Vector3& forPoint, const Rect& viewport)
{
    Vector3 v = GetOnScreenPositionAndDepth(forPoint, viewport);
    return Vector2(v.x, v.y);
}

Vector3 Camera::GetOnScreenPositionAndDepth(const Vector3& forPoint, const Rect& viewport)
{
    // We can't compute perspective projection if forPoint.z is equal to GetPosition().z
    // Since it leads to division by zero (z will be equal to 0 in view space)
    // TODO: Uncomment this after resource editor is fixed (DF-14331)
    // DVASSERT(!FLOAT_EQUAL(forPoint.z, GetPosition().z));

    Vector4 pv(forPoint);
    pv = pv * GetViewProjMatrix();

    return Vector3(((pv.x / pv.w) * 0.5f + 0.5f) * viewport.dx + viewport.x,
                   (1.0f - ((pv.y / pv.w) * 0.5f + 0.5f)) * viewport.dy + viewport.y, pv.w + pv.z);
}

const Matrix4& Camera::GetViewProjMatrix(bool invertProjection)
{
    if (flags & REQUIRE_REBUILD)
    {
        RebuildCameraFromValues();
    }
    if (flags & REQUIRE_REBUILD_PROJECTION)
    {
        RebuildProjectionMatrix(invertProjection);
    }
    if (flags & REQUIRE_REBUILD_MODEL)
    {
        RebuildViewMatrix();
    }
    if (flags & REQUIRE_REBUILD_UNIFORM_PROJ_MODEL)
    {
        viewProjMatrix = viewMatrix * projMatrix;
        flags &= ~REQUIRE_REBUILD_UNIFORM_PROJ_MODEL;
    }

    return viewProjMatrix;
}

void Camera::RebuildProjectionMatrix(bool invertProjection)
{
    flags &= ~REQUIRE_REBUILD_PROJECTION;
    flags |= REQUIRE_REBUILD_UNIFORM_PROJ_MODEL;

    float32 xMinOrientation = xmin;
    float32 xMaxOrientation = xmax;
    float32 yMinOrientation = ymin;
    float32 yMaxOrientation = ymax;

    if (invertProjection)
    {
        yMinOrientation = ymax;
        yMaxOrientation = ymin;
    }

    if (!ortho)
    {
        projMatrix.BuildPerspective(xMinOrientation, xMaxOrientation, yMinOrientation, yMaxOrientation, znear, zfar, rhi::DeviceCaps().isZeroBaseClipRange);
        projMatrix.data[8] += projectionMatrixOffset.x;
        projMatrix.data[9] += projectionMatrixOffset.y;
    }
    else
    {
        projMatrix.BuildOrtho(xMinOrientation, xMaxOrientation, yMinOrientation, yMaxOrientation, znear, zfar, rhi::DeviceCaps().isZeroBaseClipRange);
        projMatrix.data[12] += projectionMatrixOffset.x;
        projMatrix.data[13] += projectionMatrixOffset.y;
    }
}

void Camera::RebuildViewMatrix()
{
    flags &= ~REQUIRE_REBUILD_MODEL;
    flags |= REQUIRE_REBUILD_UNIFORM_PROJ_MODEL;
    viewMatrix = cameraTransform;
}

void Camera::SetPosition(const Vector3& _position)
{
    position = _position;
    flags |= REQUIRE_REBUILD;
}

void Camera::SetDirection(const Vector3& _direction)
{
    target = position + _direction;
    flags |= REQUIRE_REBUILD;
}

void Camera::SetTarget(const Vector3& _target)
{
    target = _target;
    flags |= REQUIRE_REBUILD;
}

void Camera::SetUp(const Vector3& _up)
{
    up = _up;
    flags |= REQUIRE_REBUILD;
}

void Camera::SetLeft(const Vector3& _left)
{
    left = _left;
    flags |= REQUIRE_REBUILD;
}

const Vector3& Camera::GetTarget() const
{
    return target;
}

const Vector3& Camera::GetPosition() const
{
    return position;
}

const Vector3& Camera::GetDirection()
{
    direction = target - position;
    direction.Normalize(); //TODO: normalize only on target/position changes
    return direction;
}

const Vector3& Camera::GetUp() const
{
    return up;
}

const Vector3& Camera::GetLeft() const
{
    return left;
}

const Matrix4& Camera::GetMatrix() const
{
    return viewMatrix;
}

const Matrix4& Camera::GetProjectionMatrix() const
{
    return projMatrix;
}

void Camera::RebuildCameraFromValues()
{
    flags &= ~REQUIRE_REBUILD;
    flags |= REQUIRE_REBUILD_MODEL;

    cameraTransform.BuildLookAtMatrix(position, target, up);
    left.x = -cameraTransform._00;
    left.y = -cameraTransform._10;
    left.z = -cameraTransform._20;
}

void Camera::ExtractCameraToValues()
{
    position.x = cameraTransform._30;
    position.y = cameraTransform._31;
    position.z = cameraTransform._32;
    left.x = cameraTransform._00;
    left.y = cameraTransform._10;
    left.z = cameraTransform._20;
    up.x = cameraTransform._01;
    up.y = cameraTransform._11;
    up.z = cameraTransform._21;
    target.x = position.x - cameraTransform._02;
    target.y = position.y - cameraTransform._12;
    target.z = position.z - cameraTransform._22;
}

/*
void Camera::LookAt(Vector3	position, Vector3 view, Vector3 up)
{
	// compute matrix
	localTransform.BuildLookAtMatrixLH(position, view, up);
}
 */

void Camera::PrepareDynamicParameters(bool invertProjection, Vector4* externalClipPlane)
{
    flags = REQUIRE_REBUILD | REQUIRE_REBUILD_MODEL | REQUIRE_REBUILD_PROJECTION;
    if (flags & REQUIRE_REBUILD)
    {
        RebuildCameraFromValues();
    }
    // ApplyFrustum();
    if (flags & REQUIRE_REBUILD_PROJECTION)
    {
        RebuildProjectionMatrix(invertProjection);
    }

    if (flags & REQUIRE_REBUILD_MODEL)
    {
        RebuildViewMatrix();
    }

    viewMatrix.GetInverse(invViewMatrix);

    if (externalClipPlane)
    {
        Vector4 clipPlane(*externalClipPlane);

        if (invertProjection)
        {
            clipPlane = -clipPlane;
        }

        Matrix4 m;

        viewMatrix.GetInverse(m);
        m.Transpose();
        clipPlane = clipPlane * m;

        projMatrix.GetInverse(m);
        m.Transpose();
        Vector4 v = Vector4(Sign(clipPlane.x), Sign(clipPlane.y), 1, 1) * m;

        Vector4 m4(projMatrix.data[3], projMatrix.data[7], projMatrix.data[11], projMatrix.data[15]);
        if (rhi::DeviceCaps().isZeroBaseClipRange)
        {
            Vector4 scaledPlane = clipPlane * (v.DotProduct(m4) / v.DotProduct(clipPlane));
            projMatrix.data[2] = scaledPlane.x;
            projMatrix.data[6] = scaledPlane.y;
            projMatrix.data[10] = scaledPlane.z;
            projMatrix.data[14] = scaledPlane.w;
        }
        else
        {
            Vector4 scaledPlane = clipPlane * (2.0f * v.DotProduct(m4) / v.DotProduct(clipPlane));
            projMatrix.data[2] = scaledPlane.x;
            projMatrix.data[6] = scaledPlane.y;
            projMatrix.data[10] = scaledPlane.z + 1.0f;
            projMatrix.data[14] = scaledPlane.w;
        }
    }

    viewProjMatrix = viewMatrix * projMatrix;

    flags &= ~REQUIRE_REBUILD_UNIFORM_PROJ_MODEL;

    viewProjMatrix.GetInverse(invViewProjMatrix);

    if (currentFrustum)
    {
        currentFrustum->Build(viewProjMatrix, rhi::DeviceCaps().isZeroBaseClipRange);
    }

    projectionFlip = invertProjection ? -1.0f : 1.0f;
}

void Camera::SetupDynamicParameters(bool invertProjection, Vector4* externalClipPlane)
{
    PrepareDynamicParameters(invertProjection, externalClipPlane);

    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_VIEW, &viewMatrix, DynamicBindings::UPDATE_SEMANTIC_ALWAYS);
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_PROJ, &projMatrix, DynamicBindings::UPDATE_SEMANTIC_ALWAYS);
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_VIEW_PROJ, &viewProjMatrix, DynamicBindings::UPDATE_SEMANTIC_ALWAYS);
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_INV_VIEW, &invViewMatrix, DynamicBindings::UPDATE_SEMANTIC_ALWAYS);
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_INV_VIEW_PROJ, &invViewProjMatrix, DynamicBindings::UPDATE_SEMANTIC_ALWAYS);

    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_CAMERA_POS, &position, DynamicBindings::UPDATE_SEMANTIC_ALWAYS);
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_CAMERA_DIR, &direction, DynamicBindings::UPDATE_SEMANTIC_ALWAYS);
    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_CAMERA_UP, &up, DynamicBindings::UPDATE_SEMANTIC_ALWAYS);

    Renderer::GetDynamicBindings().SetDynamicParam(DynamicBindings::PARAM_PROJECTION_FLIP, &projectionFlip, DynamicBindings::UPDATE_SEMANTIC_ALWAYS);
}

BaseObject* Camera::Clone(BaseObject* dstNode)
{
    if (!dstNode)
    {
        DVASSERT(IsPointerToExactClass<Camera>(this), "Can clone only Camera");
        dstNode = new Camera();
    }

    Camera* cnd = static_cast<Camera*>(dstNode);
    cnd->znear = znear;
    cnd->zfar = zfar;
    cnd->aspect = aspect;
    cnd->fovX = fovX;
    cnd->ortho = ortho;
    cnd->orthoWidth = orthoWidth;

    cnd->position = position;
    cnd->target = target;
    cnd->up = up;
    cnd->left = left;
    cnd->cameraTransform = cameraTransform;
    cnd->flags = flags;
    cnd->zoomFactor = zoomFactor;

    cnd->Recalc();
    return dstNode;
}

Frustum* Camera::GetFrustum() const
{
    return currentFrustum;
}

void Camera::CalculateZoomFactor()
{
    zoomFactor = tanf(DegToRad(fovX * 0.5f));
}

float32 Camera::GetZoomFactor() const
{
    return zoomFactor;
}

void Camera::Draw()
{
}

Vector3 Camera::UnProject(float32 winx, float32 winy, float32 winz, const Rect& viewport)
{
    //	Matrix4 finalMatrix = modelMatrix * projMatrix;//RenderManager::Instance()->GetUniformMatrix(RenderManager::UNIFORM_MATRIX_MODELVIEWPROJECTION);

    Matrix4 finalMatrix = GetViewProjMatrix();
    finalMatrix.Inverse();

    Vector4 in(winx, winy, winz, 1.0f);

    /* Map x and y from window coordinates */

    in.x = (in.x - viewport.x) / viewport.dx;
    in.y = 1.0f - (in.y - viewport.y) / viewport.dy;

    /* Map to range -1 to 1 */
    in.x = in.x * 2 - 1;
    in.y = in.y * 2 - 1;
    if (!rhi::DeviceCaps().isZeroBaseClipRange)
        in.z = in.z * 2 - 1;

    Vector4 out = in * finalMatrix;

    Vector3 result(0, 0, 0);
    if (out.w == 0.0)
        return result;

    result.x = out.x / out.w;
    result.y = out.y / out.w;
    result.z = out.z / out.w;
    return result;
}

void Camera::SaveObject(KeyedArchive* archive)
{
    BaseObject::SaveObject(archive);

    archive->SetFloat("cam.orthoWidth", orthoWidth);
    archive->SetFloat("cam.znear", znear);
    archive->SetFloat("cam.zfar", zfar);
    archive->SetFloat("cam.aspect", aspect);
    archive->SetFloat("cam.fov", fovX);
    archive->SetBool("cam.isOrtho", ortho);
    archive->SetInt32("cam.flags", flags);

    archive->SetByteArrayAsType("cam.position", position);
    archive->SetByteArrayAsType("cam.target", target);
    archive->SetByteArrayAsType("cam.up", up);
    archive->SetByteArrayAsType("cam.left", left);
    archive->SetByteArrayAsType("cam.direction", direction);

    archive->SetByteArrayAsType("cam.cameraTransform", cameraTransform);

    archive->SetByteArrayAsType("cam.modelMatrix", viewMatrix);
    archive->SetByteArrayAsType("cam.projMatrix", projMatrix);
    archive->SetVector2("cam.projMatrixOffset", projectionMatrixOffset);
}

void Camera::LoadObject(KeyedArchive* archive)
{
    BaseObject::LoadObject(archive);

    // todo add default values
    orthoWidth = archive->GetFloat("cam.orthoWidth");
    znear = archive->GetFloat("cam.znear");
    zfar = archive->GetFloat("cam.zfar");
    aspect = archive->GetFloat("cam.aspect");
    fovX = archive->GetFloat("cam.fov");
    ortho = archive->GetBool("cam.isOrtho");
    flags = archive->GetInt32("cam.flags");

    position = archive->GetByteArrayAsType("cam.position", position);
    target = archive->GetByteArrayAsType("cam.target", target);
    up = archive->GetByteArrayAsType("cam.up", up);
    left = archive->GetByteArrayAsType("cam.left", left);
    direction = archive->GetByteArrayAsType("cam.direction", direction);

    cameraTransform = archive->GetByteArrayAsType("cam.cameraTransform", cameraTransform);
    viewMatrix = archive->GetByteArrayAsType("cam.modelMatrix", viewMatrix);
    projMatrix = archive->GetByteArrayAsType("cam.projMatrix", projMatrix);
    projectionMatrixOffset = archive->GetVector2("cam.projMatrixOffset", projectionMatrixOffset);

    Recalc();
}

void Camera::CopyMathOnly(const Camera& c)
{
    *currentFrustum = *c.currentFrustum;
    zoomFactor = c.zoomFactor;

    xmin = c.xmin;
    xmax = c.xmax;
    ymin = c.ymin;
    ymax = c.ymax;
    znear = c.znear;
    zfar = c.zfar;
    aspect = c.aspect;
    fovX = c.fovX;
    ortho = c.ortho;

    position = c.position;
    target = c.target;
    up = c.up;
    left = c.left;

    direction = c.direction;

    cameraTransform = c.cameraTransform;
    viewMatrix = c.viewMatrix;
    projMatrix = c.projMatrix;
    viewProjMatrix = c.viewProjMatrix;
    flags = c.flags;
}

void Camera::SetProjectionMatrixOffset(const Vector2& offset)
{
    projectionMatrixOffset = offset;
    Recalc();
}

} // ns
