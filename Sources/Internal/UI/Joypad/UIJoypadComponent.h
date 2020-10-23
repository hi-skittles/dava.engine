#pragma once

#include "UI/Components/UIComponent.h"
#include "Functional/Function.h"
#include "Math/Rect.h"

namespace DAVA
{
class UIJoypadComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIJoypadComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIJoypadComponent);

public:
    using CoordsTransformFn = Function<Vector2(Vector2)>;

    UIJoypadComponent() = default;
    UIJoypadComponent(const UIJoypadComponent& other);

    UIJoypadComponent* Clone() const override;

    /**
        Get dynamic flag. If this flag is set, on first touch on the control joypad will be moved to its (touch) position.
        On touch release joypad will be moved to initial position.
    */
    bool IsDynamic() const;

    /**
        Set dynamic flag. If this flag is set, on first touch on the control joypad will be moved to its (touch) position.
        On touch release joypad will be moved to initial position.
    */
    void SetDynamicFlag(bool dynamic);

    /**
        Get stick area control path. Empty string is returned if area control is not set.
    */
    const String& GetStickAreaControlPath() const;

    /**
        Set stick area control. Input for this control will be disabled, pivot will be set to {0.5, 0.5}.
    */
    void SetStickAreaControlPath(const String& areaControlPath);

    /**
        Get stick arm control path. Empty string is returned if area control is not set.
    */
    const String& GetStickArmControlPath() const;

    /**
        Set stick arm control. Input for this control will be disabled, pivot will be set to {0.5, 0.5}.
    */
    void SetStickArmControlPath(const String& armControlPath);

    /**
        Get stick area control path. Empty string is returned if area control is not set.
    */
    const String& GetStickArrowControlPath() const;

    /**
        Set stick arrow control. Input for this control will be disabled, pivot will be set to {0.5, 0.5}.
    */
    void SetStickArrowControlPath(const String& arrowControlPath);

    /**
        Get coords transform function.
    */
    const CoordsTransformFn& GetCoordsTransformFunction();

    /**
        Set function to transform coords. This function should return Vector2 with coords in range [-1, 1].
    */
    void SetCoordsTransformFunction(CoordsTransformFn fn);

    /**
        Get stick area radius.
    */
    float32 GetStickAreaRadius() const;

    /**
        Set stick area radius. Should be in virtual coords.
    */
    void SetStickAreaRadius(float32 radius);

    /**
        Get active flag. This flag is set when joypad is being touched.
    */
    bool IsActive() const;

    /**
        Set active flag. Used by UIJoypadSystem, don't use it manually.
    */
    void SetActiveFlag(bool active);

    /**
        Get original stick coords (not transformed by coords transform function).
    */
    Vector2 GetOriginalCoords() const;

    /**
        Set original stick coords. Used by UIJoypadSystem, don't use it manually.
    */
    void SetOriginalCoords(Vector2 coords);

    /**
        Get initial position.
    */
    Vector2 GetInitialPosition() const;

    /**
        Set initial position. Should be in virtual coords.
    */
    void SetInitialPosition(Vector2 position);

    /**
        Get activation threshold. 1e-5 is a default one.
    */
    float32 GetActivationThreshold() const;

    /**
        Set activation threshold. 1e-5 is a default one. This is basically just an epsilon to compare initial stick coords delta.
        `active` flag will be set only when transformed coords length > threshold.
    */
    void SetActivationThreshold(float32 threshold);

    /**
        Get cancel radius.
    */
    float32 GetCancelRadius() const;

    /**
        Set cancel radius. Radius from initial/first to current touch position to cancel active faze.
        Should be in virtual coords.
    */
    void SetCancelRadius(float32 radius);

    /**
        Get cancel zone.
    */
    const Vector4& GetCancelZone() const;

    /**
        Set cancel zone. If stick in a active faze and touch position will cross this zone, active faze will be canceled.
        Should be in virtual coords.
    */
    void SetCancelZone(const Vector4& zone);

    /**
        Get transformed coords. Return result of transform function applied to original coords.
    */
    Vector2 GetTransformedCoords() const;

    /** Get stick area control */
    UIControl* GetStickArea();

    /** Get stick arm control */
    UIControl* GetStickArm();

    /** Get stick arrow control */
    UIControl* GetStickArrow();

protected:
    ~UIJoypadComponent() = default;

private:
    String stickAreaControlPath;
    String stickArmControlPath;
    String stickArrowControlPath;

    CoordsTransformFn coordsTransformFn;

    float32 stickAreaRadius = 0.f;

    bool isDynamic = false;
    bool isActive = false;

    Vector2 coords = { 0.f, 0.f };
    Vector2 initialPosition = { 0.f, 0.f };

    float32 activationThreshold = 1e-5f;

    Vector4 cancelZone = Vector4::Zero;
    float32 cancelRadius = 1e9;
};
}