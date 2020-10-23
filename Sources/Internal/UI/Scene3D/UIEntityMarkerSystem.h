#pragma once

#include "Base/BaseTypes.h"
#include "Base/Vector.h"
#include "UI/UISystem.h"

namespace DAVA
{
class UIComponent;
class UIControl;
class UIEntityMarkerComponent;
class UIEntityMarkersContainerComponent;

/** System for synchronization params between UIControl and Entity. */
class UIEntityMarkerSystem : public UISystem
{
public:
    UIEntityMarkerSystem();
    ~UIEntityMarkerSystem();

protected:
    void RegisterControl(UIControl* control) override;
    void UnregisterControl(UIControl* control) override;
    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void Process(float32 elapsedTime) override;

private:
    struct Link
    {
        UIEntityMarkersContainerComponent* container = nullptr;
        Vector<UIEntityMarkerComponent*> children;
        UnorderedMap<UIControl*, float32> orderMap;
    };

    Vector<Link> links;
};
}