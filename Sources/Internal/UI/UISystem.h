#pragma once

#include "Base/BaseTypes.h"

namespace DAVA
{
class UIComponent;
class UIControl;
class UIControlSystem;

class UISystem
{
    friend class UIControlSystem;

public:
    virtual ~UISystem() = default;

protected:
    /** Called after adding this system to scene. */
    virtual void RegisterSystem(){};
    /** Called before removing this system from scene. */
    virtual void UnregisterSystem(){};

    virtual void RegisterControl(UIControl* control){};
    virtual void UnregisterControl(UIControl* control){};
    virtual void RegisterComponent(UIControl* control, UIComponent* component){};
    virtual void UnregisterComponent(UIControl* control, UIComponent* component){};

    virtual void OnControlVisible(UIControl* control){};
    virtual void OnControlInvisible(UIControl* control){};

    virtual void Process(float32 elapsedTime){};
    virtual void ForceProcessControl(float32 elapsedTime, UIControl* control){};

    void SetScene(UIControlSystem* scene);
    UIControlSystem* GetScene() const;

private:
    UIControlSystem* scene = nullptr;
};

inline UIControlSystem* UISystem::GetScene() const
{
    return scene;
}
}
