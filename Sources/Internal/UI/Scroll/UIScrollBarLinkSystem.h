#pragma once

#include "UI/UISystem.h"

namespace DAVA
{
class UIScrollBarDelegateComponent;

class UIScrollBarLinkSystem : public UISystem
{
public:
    UIScrollBarLinkSystem();
    ~UIScrollBarLinkSystem() override;

    void RegisterControl(UIControl* control) override;
    void UnregisterControl(UIControl* control) override;
    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void Process(DAVA::float32 elapsedTime) override;

    bool IsRestoreLinks() const;
    void SetRestoreLinks(bool restoring);

private:
    struct Link
    {
        enum Type
        {
            SCROLL_BAR_LINKED = 0x01,
            DELEGATE_LINKED = 0x02
        };
        UIScrollBarDelegateComponent* component = nullptr;
        UIControl* linkedControl = nullptr;
        int32 type = 0;

        Link(UIScrollBarDelegateComponent* component_)
            : component(component_)
            , type(SCROLL_BAR_LINKED)
        {
        }
    };

    void SetupLink(Link* link, UIControl* control);
    void BreakLink(Link* link);

    template <typename Predicate>
    bool RestoreLink(int32 linkType, Predicate);

    void LinkScrollBar(UIScrollBarDelegateComponent* component);
    void LinkDelegate(UIControl* linkedControl);

    template <typename Predicate>
    void Unlink(int32 linkType, Predicate);

    void UnlinkScrollBar(UIScrollBarDelegateComponent* component);
    void UnlinkDelegate(UIControl* linkedControl);

    Vector<Link> links;

    bool restoreLinks = false;
};
}
