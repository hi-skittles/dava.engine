#pragma once

#include "Base/BaseTypes.h"
#include "Base/BaseMath.h"
#include "Base/BaseObject.h"
#include "Reflection/Reflection.h"
#include "Render/Highlevel/Frustum.h"

namespace DAVA
{
/**
    \ingroup scene3d
    \brief this class is main class to perform camera transformations in our 3D engine.
    This class creates standard matrix-based view-directional camera. Right now engine do not have quaternion camera, and
    you can always add it support if it will be required.

    TODO: Should it be a SceneNode? It doesn't use inherited transofrmations!
    TODO: Move math to a separate class, use composition, see CopyMathOnly
 */
class Camera : public BaseObject
{
protected:
    virtual ~Camera();

public:
    Camera();
    Camera(const Camera& c);

    /**
        \brief Setup perspective camera with basic camera params.
        This function set all parameters for the camera. All these parameters will be applied in case if camera Set function will be called. 
     
        \param[in] fovxInDegrees horizontal FOV of camera in degrees.
        \param[in] aspectYdivX proportion between y and x. So if you want to setup camera manually pass y / x
        \param[in] zNear near clipping distance of camera
        \param[in] zFar far clipping distance of camera
     */
    void SetupPerspective(float32 fovxInDegrees, float32 aspectYdivX, float32 zNear, float32 zFar);

    /**
        \brief Setup ortho camera with basic camera params.
        This function set all parameters for the camera. All these parameters will be applied in case if camera Set function will be called. 
     
        \param[in] width frustum width
        \param[in] aspectYdivX proportion between y and x. So if you want to setup camera manually pass y / x
        \param[in] zNear near clipping distance of camera
        \param[in] zFar far clipping distance of camera
     */
    void SetupOrtho(float32 width, float32 aspectYdivX, float32 zNear, float32 zFar);

    /**
        \brief SetupPerspective camera with basic camera params.
		This function set all parameters for the camera. All these parameters will be applied in case if camera Set function will be called.
	*/
    void Setup(float32 xmin, float32 xmax, float32 ymin, float32 ymax, float32 znear, float32 zfar);

    /**
        \brief Function change fov in perspective camera.
        You can use this function in many cases. For example you can use it when you want to change zoom of camera in your game. 
        
        \param[in] fovInDegrees new for in degrees for the camera
     */
    virtual void SetFOV(const float32& fovxInDegrees);

    /**
        \brief Function change width in ortho camera.
        
        \param[in] width new width for the camera
     */
    void SetOrthoWidth(const float32& width);

    float32 GetOrthoWidth() const;

    /**
        \brief Set camera aspect ratio
        \param[in] aspectYdivX Aspect ratio is viewport height / viewport width
     */
    virtual void SetAspect(const float32& aspectYdivX);

    /**
         \brief Function change zNear in camera.
         \param[in] zNear near clipping distance of camera
     */
    void SetZNear(const float32& _zNear);

    /**
         \brief Function change zFar in camera.
         \param[in] zFar far clipping distance of camera
     */
    void SetZFar(const float32& _zFar);

    /**
         \brief Set camera ortho flag
         \param[in] _ortho is camera will be with orthographic projection or perspective. By default it's false so camera will be with perspective projection.
     */
    void SetIsOrtho(const bool& _ortho);

    /** 
        \brief Function prepares camera transformations (projection, model-view matrices) without submitting to RenderManager
        This function normally is called internally from RenderPass class (or from SetupDynamicParameters). In most cases you'll not need it. 
         \param[in] externalClipPlane - if not NULL replaces near clipping plane with this and build projection matrix accordingly
     */
    void PrepareDynamicParameters(bool invertProjection, Vector4* externalClipPlane = NULL);
    /** 
        \brief Function applies camera transformations (projection, model-view matrices) to RenderManager
        This function normally is called internally from RenderPass class. In most cases you'll not need it. 
        \param[in] externalClipPlane - if not NULL replaces near clipping plane with this and build projection matrix accordingly
     */
    void SetupDynamicParameters(bool invertProjection, Vector4* externalClipPlane = NULL);

    /**
         \brief return current xmin of this camera
         \returns xmin for this camera
     */
    float32 GetXMin() const;

    /**
         \brief Set camera xmin
     */
    void SetXMin(const float32& _xmin);

    /**
         \brief return current xmax of this camera
         \returns xmax for this camera
     */
    float32 GetXMax() const;

    /**
        \brief Set camera xmax
     */
    void SetXMax(const float32& _xmax);

    /**
         \brief return current ymin of this camera
         \returns ymin for this camera
     */
    float32 GetYMin() const;

    /**
        \brief Set camera ymin
     */
    void SetYMin(const float32& _ymin);

    /**
         \brief return current ymax of this camera
         \returns ymax for this camera
     */
    float32 GetYMax() const;

    /**
        \brief Set camera ymax
     */
    void SetYMax(const float32& _ymax);

    /**
        \brief return current Field Of View of this camera
        \returns FOV for this camera
     */
    virtual float32 GetFOV() const;

    /** 
        \brief return current aspect for this camera
        \returns current aspect ratio for the camera
     */
    float32 GetAspect() const;

    /**
        \brief return znear value for this camera
        \returns current znear value
     */
    float32 GetZNear() const;

    /**
        \brief return zfar value for this camera
        \returns current zfar value
     */
    float32 GetZFar() const;

    /**
     \brief return ortho value for this camera
     \returns current ortho value
     */
    bool GetIsOrtho() const;

    /**
        \brief Function change camera position.
        \param[in] position new camera position
     */
    virtual void SetPosition(const Vector3& position);
    /**
        \brief Function change camera direction.
        Be carefull with this function. It changing target of the camera because in calculations we use pair: position, target. 
        \param[in] direction new camera direction
     */
    virtual void SetDirection(const Vector3& direction);

    /**
        \brief Function to change camera target
        \param[in] target new camera target
     */
    virtual void SetTarget(const Vector3& target);

    /**
        \brief Function to change camera up vector
        \param[in] up new camera up vector
     */
    void SetUp(const Vector3& up);

    /**
        \brief Function to change left camera vector
        \param[in] left new camera left vector
     */
    void SetLeft(const Vector3& left);

    /**
        \brief Function returns current camera target vector
        Be carefull if you using SetDirection, target will not give you a vector that will make any sense. 
        It'll be just a point that located on (position, direction) ray distant on 1 from position.
        \returns target vector
     */
    virtual const Vector3& GetTarget() const;
    /**
        \brief Function returns position of camera
        \returns current position
     */
    virtual const Vector3& GetPosition() const;
    /**
        \brief Function returns normalized direction of camera
        \returns current normalized direction
     */
    virtual const Vector3& GetDirection(); // camera forward direction
    /**
        \brief Function returns current normalized up vector of camera
        \returns current normalized up vector
     */
    const Vector3& GetUp() const;
    /**
        \brief Function returns current normalized left vector of camera
        \returns current normalized left vector
     */
    const Vector3& GetLeft() const; // camera left direction
    /**
        \brief Function returns current camera matrix
        This function returns camera matrix without multiplication of this matrix to viewport rotation matrix according to RenderManager and Core Interface Orientation.
        This matrix is right matrix that should be used in all 3D computations, that depends from camera, but final multiplication should be done to model view matrix.
        \returns current camera matrix
     */
    const Matrix4& GetMatrix() const;

    const Matrix4& GetProjectionMatrix() const;

    /**
        \brief Rebuild camera from all values that set inside it.
     */
    void RebuildCameraFromValues();

    /**
        \brief Extract values from matrices to all vectors
        This function is called when you load camera from file, using camera matrix
     */
    void ExtractCameraToValues();

    void RebuildProjectionMatrix(bool invertProjection = false);
    void RebuildViewMatrix();

    /**
        \brief Clone current camera
        TODO: remove, make adjustments in copy constructor instead. Clone() is evil, see Effective Java for details.
     */
    virtual BaseObject* Clone(BaseObject* dstNode = NULL);

    /**
        \brief Get project * camera matrix
        \returns matrix 
     */
    const Matrix4& GetViewProjMatrix(bool invertProjection = false);

    /**
        \brief Function to return 2D position of 3D point that is transformed to screen. 
        \returns 2D point on screen.
     */
    Vector2 GetOnScreenPosition(const Vector3& forPoint, const Rect& viewport);
    Vector3 GetOnScreenPositionAndDepth(const Vector3& forPoint, const Rect& viewport);

    /**
        \brief Function to return 3D position of 2D point that was transformed before. 
        \param[in] win windows coords of the point
        \param[in] viewport viewport coords
        \returns point position in 3D world space, before modelview & projection
     */
    Vector3 UnProject(float32 winx, float32 winy, float32 winz, const Rect& viewport);

    /**
        \brief Get frustum object for this camera.
        This function is widely used everywhere where you need object clipping.
     
        \returns pointer to frustum object
     */
    Frustum* GetFrustum() const;

    /**
        \brief Get camera zoom factor. 
        You can use zoom factor to have dependencies between camera zoom and visualization of object
        \returns tanf(fov / 2). 
     */
    float32 GetZoomFactor() const;

    /**
        \brief Draw debug camera information if debug flags enabled
     */
    void Draw();

    void SaveObject(KeyedArchive* archive) override;
    void LoadObject(KeyedArchive* archive) override;

    /// Overwrites frustum data (not pointer) and other math data only, no SceneNode etc. stuff here.
    /// Added to support some math caching. Future versions of this library should provide a separate class
    /// for camera math and use composition to merge math and rendering scene node.
    void CopyMathOnly(const Camera& c);

    /*
     * Projection matrix offset could be used for half-pixel offset in certain APIs
     */
    const Vector2& GetProjectionMatrixOffset() const;
    void SetProjectionMatrixOffset(const Vector2& offset);

    enum eFlags
    {
        REQUIRE_REBUILD = 1,
        REQUIRE_REBUILD_MODEL = 1 << 1,
        REQUIRE_REBUILD_PROJECTION = 1 << 2,
        REQUIRE_REBUILD_UNIFORM_PROJ_MODEL = 1 << 3
    };

protected:
    //    virtual SceneNode* CopyDataTo(SceneNode *dstNode);
    float32 xmin, xmax, ymin, ymax, znear, zfar, aspect;
    float32 fovX;
    float32 orthoWidth;
    bool ortho;

    Vector3 position; //
    Vector3 target; //
    Vector3 up;
    Vector3 left;
    Vector2 projectionMatrixOffset;
    Vector3 direction; // right now this variable updated only when you call GetDirection.

    //Quaternion rotation;	//
    Matrix4 cameraTransform;

    Matrix4 viewMatrix;
    Matrix4 projMatrix;
    Matrix4 viewProjMatrix;
    Matrix4 invViewMatrix;
    Matrix4 invViewProjMatrix;

    float32 projectionFlip;

    uint32 flags;

    // TODO: not necessary to be a pointer here.
    Frustum* currentFrustum;

    void Recalc();
    void ValidateProperties();

    void CalculateZoomFactor();

    float32 zoomFactor;

    DAVA_VIRTUAL_REFLECTION(Camera, BaseObject);
};

inline const Vector2& Camera::GetProjectionMatrixOffset() const
{
    return projectionMatrixOffset;
}
}
