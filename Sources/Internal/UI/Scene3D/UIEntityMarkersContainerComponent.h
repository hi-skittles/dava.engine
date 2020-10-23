#pragma once

#include "Base/RefPtr.h"
#include "Functional/Function.h"
#include "Math/Vector.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
class Entity;
class ScreenPositionComponent;
class UIEntityMarkerComponent;

/** Component for setup synchronization params between UIControl and Entity. */
class UIEntityMarkersContainerComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIEntityMarkersContainerComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIEntityMarkersContainerComponent);

public:
    /** Describe ordering mode. */
    enum class OrderMode : int32
    {
        NearFront = 0, //!< Marker for near Entity will be on front.
        NearBack //!< marker for near Entity will be on back.
    };

    /** Function declaration using for custom strategies. */
    using CustomStrategy = Function<void(UIControl*, UIEntityMarkersContainerComponent*, UIEntityMarkerComponent*)>;

    UIEntityMarkersContainerComponent();
    UIEntityMarkersContainerComponent(const UIEntityMarkersContainerComponent& src);
    UIEntityMarkersContainerComponent& operator=(const UIEntityMarkersContainerComponent&) = delete;

    UIEntityMarkersContainerComponent* Clone() const override;

    /** Return enabled flag. */
    bool IsEnabled() const;
    /** Setup enabled flag. */
    void SetEnabled(bool enable);

    /** Return visibility synchronization flag. */
    bool IsSyncVisibilityEnabled() const;
    /** Setup visibility synchronization flag. */
    void SetSyncVisibilityEnabled(bool enable);

    /** Return position synchronization flag. */
    bool IsSyncPositionEnabled() const;
    /** Setup position synchronization flag. */
    void SetSyncPositionEnabled(bool enable);

    /** Return scale synchronization flag. */
    bool IsSyncScaleEnabled() const;
    /** Setup scale synchronization flag. */
    void SetSyncScaleEnabled(bool enable);
    /** Return scale factor value. */
    const Vector2& GetScaleFactor() const;
    /** Setup scale factor value. New scale will calculated as `factor / distance`. */
    void SetScaleFactor(const Vector2& factor);
    /** Return maximum scale value. */
    const Vector2& GetMaxScale() const;
    /** Setup maximum scale value. */
    void SetMaxScale(const Vector2& s);
    /** Return minimum scale value. */
    const Vector2& GetMinScale() const;
    /** Setup minimum scale value. */
    void SetMinScale(const Vector2& s);

    /** Return order synchronization flag. */
    bool IsSyncOrderEnabled() const;
    /** Setup order synchronization flag. */
    void SetSyncOrderEnabled(bool enable);
    /** Return ordering mode. */
    OrderMode GetOrderMode() const;
    /** Setup ordering mode. */
    void SetOrderMode(OrderMode mode);

    /** Return using custom strategy flag. */
    bool IsUseCustomStrategy() const;
    /** Setup using custom strategy flag. */
    void SetUseCustomStrategy(bool enable);
    /** Return custom strategy function. */
    const CustomStrategy& GetCustomStrategy() const;
    /** Setup custom strategy function. */
    void SetCustomStrategy(const CustomStrategy& fn);

protected:
    ~UIEntityMarkersContainerComponent();

private:
    bool enabled = true;
    // Visibility
    bool syncVisibilityEnabled = false;
    // Position
    bool syncPositionEnabled = false;
    // Scale
    bool syncScaleEnabled = false;
    Vector2 scaleFactor = Vector2(1.f, 1.f);
    Vector2 maxScale = Vector2(2.f, 2.f);
    Vector2 minScale = Vector2(.1f, .1f);
    // Order
    bool syncOrderEnabled = false;
    OrderMode orderMode = OrderMode::NearFront;
    // Custom strategy
    bool useCustomStrategy = false;
    CustomStrategy customStrategy;
};

inline bool UIEntityMarkersContainerComponent::IsEnabled() const
{
    return enabled;
}

inline bool UIEntityMarkersContainerComponent::IsSyncVisibilityEnabled() const
{
    return syncVisibilityEnabled;
}

inline bool UIEntityMarkersContainerComponent::IsSyncPositionEnabled() const
{
    return syncPositionEnabled;
}

inline bool UIEntityMarkersContainerComponent::IsSyncScaleEnabled() const
{
    return syncScaleEnabled;
}

inline const Vector2& UIEntityMarkersContainerComponent::GetScaleFactor() const
{
    return scaleFactor;
}

inline const Vector2& UIEntityMarkersContainerComponent::GetMaxScale() const
{
    return maxScale;
}

inline const Vector2& UIEntityMarkersContainerComponent::GetMinScale() const
{
    return minScale;
}

inline bool UIEntityMarkersContainerComponent::IsSyncOrderEnabled() const
{
    return syncOrderEnabled;
}

inline UIEntityMarkersContainerComponent::OrderMode UIEntityMarkersContainerComponent::GetOrderMode() const
{
    return orderMode;
}

inline bool UIEntityMarkersContainerComponent::IsUseCustomStrategy() const
{
    return useCustomStrategy;
}

inline const UIEntityMarkersContainerComponent::CustomStrategy& UIEntityMarkersContainerComponent::GetCustomStrategy() const
{
    return customStrategy;
}
}
