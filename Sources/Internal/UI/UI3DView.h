#ifndef __DAVAENGINE_UI_3D_VIEW__
#define __DAVAENGINE_UI_3D_VIEW__

#include "Base/BaseTypes.h"
#include "UI/UIControl.h"
#include "Render/RenderBase.h"
#include "Render/RHI/rhi_Type.h"

namespace DAVA
{
/**
    \ingroup controlsystem
    \brief This control allow to put 3D View into any place of 2D hierarchy
 */

class Scene;
class Texture;
class UI3DView : public UIControl
{
    DAVA_VIRTUAL_REFLECTION(UI3DView, UIControl);

public:
    UI3DView(const Rect& rect = Rect());

protected:
    virtual ~UI3DView();

public:
    void SetScene(Scene* scene);
    Scene* GetScene() const;

    inline const Rect& GetLastViewportRect()
    {
        return viewportRc;
    }

    void AddControl(UIControl* control) override;
    void Update(float32 timeElapsed) override;
    void Draw(const UIGeometricData& geometricData) override;

    void SetSize(const Vector2& newSize) override;
    UI3DView* Clone() override;
    void CopyDataFrom(UIControl* srcControl) override;

    void Input(UIEvent* currentInput) override;
    void InputCancelled(UIEvent* currentInput) override;

    void SetDrawToFrameBuffer(bool enable);
    bool GetDrawToFrameBuffer() const;
    void SetFrameBufferScaleFactor(float32 scale);
    float32 GetFrameBufferScaleFactor() const;
    const Vector2& GetFrameBufferRenderSize() const;

    int32 GetBasePriority();
    void SetBasePriority(int32 priority);

    bool IsClearRequested() const;
    void SetClearRequested(bool requested);

protected:
    Scene* scene;
    Rect viewportRc;

private:
    void PrepareFrameBuffer();

    bool drawToFrameBuffer;

    float32 fbScaleFactor;
    Vector2 fbRenderSize;
    Vector2 fbTexSize = Vector2(0.f, 0.f);
    Texture* frameBuffer = nullptr;

    int32 basePriority = PRIORITY_MAIN_3D;

    rhi::LoadAction colorLoadAction = rhi::LOADACTION_CLEAR;
};

inline bool UI3DView::GetDrawToFrameBuffer() const
{
    return drawToFrameBuffer;
}

inline float32 UI3DView::GetFrameBufferScaleFactor() const
{
    return fbScaleFactor;
}

inline const Vector2& UI3DView::GetFrameBufferRenderSize() const
{
    return fbRenderSize;
}

inline int32 UI3DView::GetBasePriority()
{
    return basePriority;
}
inline void UI3DView::SetBasePriority(int32 priority)
{
    basePriority = priority;
}
};

#endif
