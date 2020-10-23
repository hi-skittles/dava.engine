#pragma once

#include "Base/BaseTypes.h"
#include "Base/Observer.h"
#include "Base/RefPtr.h"
#include "UI/UISystem.h"
#include "Functional/Signal.h"

namespace DAVA
{
class UIControl;
class UIRichContentAliasesComponent;
class UIRichContentComponent;
struct RichContentLink;

class UIRichContentSystem final : public UISystem, public Observer
{
public:
    UIRichContentSystem();
    ~UIRichContentSystem() override;

    void SetEditorMode(bool editorMode);
    bool IsEditorMode() const;
    bool IsDebugDraw() const;

    void RegisterControl(UIControl* control) override;
    void UnregisterControl(UIControl* control) override;
    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void Process(float32 elapsedTime) override;

    // Error issues handling signals
    Signal<UIRichContentComponent* /* component */, const String& /* error message */> onTextXMLParsingError;
    Signal<UIRichContentAliasesComponent* /* component */, const String& /* alias name */, const String& /* error message */> onAliasXMLParsingError;
    Signal<UIComponent* /* component */> onBeginProcessComponent;
    Signal<UIComponent* /* component */> onEndProcessComponent;
    Signal<UIComponent* /* component */> onRemoveComponent;

private:
    void HandleEvent(Observable* observable) override;

    void AddLink(UIRichContentComponent* component);
    void RemoveLink(UIRichContentComponent* component);
    void AddAliases(UIControl* control, UIRichContentAliasesComponent* component);
    void RemoveAliases(UIControl* control, UIRichContentAliasesComponent* component);

    Vector<std::shared_ptr<RichContentLink>> links;
    Vector<std::shared_ptr<RichContentLink>> appendLinks;
    bool isEditorMode = false;
    bool isDebugDraw = false;
};

inline bool UIRichContentSystem::IsEditorMode() const
{
    return isEditorMode;
}

inline bool UIRichContentSystem::IsDebugDraw() const
{
    return isDebugDraw;
}
}
