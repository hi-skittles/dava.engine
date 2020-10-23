#pragma once

#include "Base/Vector.h"
#include "Base/RefPtr.h"
#include "UI/UISystem.h"

namespace DAVA
{
class UIFlowContext;
class UIControl;
class UIFlowViewComponent;

/**
    Manage all UIFlowViewComponents and control screens.
*/
class UIFlowViewSystem final : public UISystem
{
public:
    ~UIFlowViewSystem() override;

    /** Return pointer to UIFlowViewComponent by specified UIControl view. */
    UIFlowViewComponent* GetLinkedComponent(UIControl* view) const;
    /** Return pointer to UIControl view by specified UIFlowViewComponent. */
    UIControl* GetLinkedView(UIFlowViewComponent* component) const;

protected:
    void RegisterControl(UIControl* control) override;
    void UnregisterControl(UIControl* control) override;
    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void Process(float32 elapsedTime) override;

private:
    /** Describe link between UIFlowViewComponent and UIControl. */
    struct ViewLink
    {
        UIFlowViewComponent* component;
        RefPtr<UIControl> view;
    };
    Vector<ViewLink> links;

    /** Add new link with specified UIFlowViewComponent to system. */
    void AddViewLink(UIFlowViewComponent* component);
    /** Remove link with specified UIFlowViewComponent to system. */
    void RemoveViewLink(UIFlowViewComponent* component);

    /** Load new UIControl using information from UIFlowViewComponent. */
    UIControl* InitView(UIFlowViewComponent* component, UIFlowContext* context);
    /** Release UIControl linked with specified UIFlowViewComponent. */
    void ReleaseView(UIFlowViewComponent* component);
    /** Append UIControl linked with specified UIFlowViewComponent to UI hierarchy. */
    UIControl* ActivateView(UIFlowViewComponent* component, UIFlowContext* context);
    /** Remove UIControl linked with specified UIFlowViewComponent from UI hierarchy. */
    void DeactivateView(UIFlowViewComponent* component);

    friend class UIFlowStateSystem;
};
}
