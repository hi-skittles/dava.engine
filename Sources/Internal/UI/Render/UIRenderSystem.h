#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "UI/UISystem.h"
#include "UI/UIGeometricData.h"

namespace DAVA
{
class Color;
class RenderSystem2D;
class UIControlBackground;
class UIDebugRenderComponent;
class UITextComponent;
class UIScreen;
class UIScreenTransition;
class UIScreenshoter;

class UIRenderSystem final
: public UISystem
{
    friend class UIControlSystem;

public:
    UIRenderSystem(RenderSystem2D* renderSystem2D);
    ~UIRenderSystem() override;

    const UIGeometricData& GetBaseGeometricData() const;
    UIScreenshoter* GetScreenshoter() const;
    int32 GetUI3DViewCount() const;
    RenderSystem2D* GetRenderSystem2D() const;

    void SetClearColor(const Color& clearColor);
    void SetUseClearPass(bool useClearPass);

    void SetCurrentScreen(const RefPtr<UIScreen>& screen);
    void SetPopupContainer(const RefPtr<UIControl>& popupContainer);

protected:
    void OnControlVisible(UIControl* control) override;
    void OnControlInvisible(UIControl* control) override;

    void RegisterComponent(UIControl* control, UIComponent* component) override;
    void UnregisterComponent(UIControl* control, UIComponent* component) override;

    void Process(float32 elapsedTime) override;
    void Render();

private:
    void ForceRenderControl(UIControl* control);

    void RenderControlHierarhy(UIControl* control, const UIGeometricData& geometricData, const UIControlBackground* parentBackground);

    void DebugRender(const UIDebugRenderComponent* component, const UIGeometricData& geometricData);
    void RenderDebugRect(const UIDebugRenderComponent* component, const UIGeometricData& geometricData);
    void RenderPivotPoint(const UIDebugRenderComponent* component, const UIGeometricData& geometricData);

    void RenderText(const UIControl* control, const UITextComponent* component, const UIGeometricData& geometricData, const Color& parentColor);

    RenderSystem2D* renderSystem2D = nullptr;
    UIGeometricData baseGeometricData;

    std::unique_ptr<UIScreenshoter> screenshoter;

    RefPtr<UIScreen> currentScreen;
    RefPtr<UIControl> popupContainer;

    Set<UIControl*> ui3DViews;
    bool needClearMainPass = true;
};
}
