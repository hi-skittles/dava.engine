#include "UIRenderSystem.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Render/2D/Systems/RenderSystem2D.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/2D/TextBlock.h"
#include "Render/2D/TextBlockSoftwareRender.h"
#include "Render/Renderer.h"
#include "UI/Render/UIClipContentComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"
#include "UI/Scene3D/UISceneComponent.h"
#include "UI/Text/Private/UITextSystemLink.h"
#include "UI/Text/UITextComponent.h"
#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"
#include "UI/UIScreen.h"
#include "UI/UIScreenTransition.h"
#include "UI/UIScreenshoter.h"
#include "Utils/StringUtils.h"

namespace DAVA
{
namespace RenderTextDetails
{
static void PrepareSprite(const UITextSystemLink* link);

#if defined(LOCALIZATION_DEBUG)
static void DrawDebug(const UITextSystemLink* link, const UIGeometricData& textGeomData);
static void DrawLocalizationDebug(const UITextSystemLink* link, const UIGeometricData& textGeomData);
static void DrawLocalizationErrors(const UITextSystemLink* link, const UIGeometricData& geometricData);
#endif
}

UIRenderSystem::UIRenderSystem(RenderSystem2D* renderSystem2D_)
    : renderSystem2D(renderSystem2D_)
    , screenshoter(std::make_unique<UIScreenshoter>())
{
    baseGeometricData.position = Vector2(0, 0);
    baseGeometricData.size = Vector2(0, 0);
    baseGeometricData.pivotPoint = Vector2(0, 0);
    baseGeometricData.scale = Vector2(1.0f, 1.0f);
    baseGeometricData.angle = 0;
}

UIRenderSystem::~UIRenderSystem() = default;

void UIRenderSystem::OnControlVisible(UIControl* control)
{
    if (control->GetComponentCount<UISceneComponent>() != 0)
    {
        ui3DViews.insert(control);
    }
}

void UIRenderSystem::OnControlInvisible(UIControl* control)
{
    if (control->GetComponentCount<UISceneComponent>() != 0)
    {
        ui3DViews.erase(control);
    }
}

void UIRenderSystem::RegisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UISceneComponent>() && control->IsVisible())
    {
        ui3DViews.insert(control);
    }
}

void UIRenderSystem::UnregisterComponent(UIControl* control, UIComponent* component)
{
    if (component->GetType() == Type::Instance<UISceneComponent>() && control->IsVisible())
    {
        ui3DViews.erase(control);
    }
}

void UIRenderSystem::Process(float32 elapsedTime)
{
    RenderSystem2D::RenderTargetPassDescriptor newDescr = renderSystem2D->GetMainTargetDescriptor();
    newDescr.clearTarget = ui3DViews.empty() && needClearMainPass;
    renderSystem2D->SetMainTargetDescriptor(newDescr);
}

void UIRenderSystem::Render()
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_RENDER_SYSTEM);

    if (currentScreen.Valid())
    {
        RenderControlHierarhy(currentScreen.Get(), baseGeometricData, nullptr);
    }

    if (popupContainer.Valid())
    {
        RenderControlHierarhy(popupContainer.Get(), baseGeometricData, nullptr);
    }

    screenshoter->OnFrame();
}

void UIRenderSystem::ForceRenderControl(UIControl* control)
{
    DAVA_PROFILER_CPU_SCOPE(ProfilerCPUMarkerName::UI_RENDER_SYSTEM);

    RenderControlHierarhy(control, baseGeometricData, nullptr);
}

const UIGeometricData& UIRenderSystem::GetBaseGeometricData() const
{
    return baseGeometricData;
}

UIScreenshoter* UIRenderSystem::GetScreenshoter() const
{
    return screenshoter.get();
}

int32 UIRenderSystem::GetUI3DViewCount() const
{
    return static_cast<int32>(ui3DViews.size());
}

RenderSystem2D* UIRenderSystem::GetRenderSystem2D() const
{
    return renderSystem2D;
}

void UIRenderSystem::SetClearColor(const Color& clearColor)
{
    RenderSystem2D::RenderTargetPassDescriptor newDescr = renderSystem2D->GetMainTargetDescriptor();
    newDescr.clearColor = clearColor;
    renderSystem2D->SetMainTargetDescriptor(newDescr);
}

void UIRenderSystem::SetUseClearPass(bool useClearPass)
{
    needClearMainPass = useClearPass;
}

void UIRenderSystem::SetCurrentScreen(const RefPtr<UIScreen>& _screen)
{
    currentScreen = _screen;
}

void UIRenderSystem::SetPopupContainer(const RefPtr<UIControl>& _popupContainer)
{
    popupContainer = _popupContainer;
}

void UIRenderSystem::RenderControlHierarhy(UIControl* control, const UIGeometricData& geometricData, const UIControlBackground* parentBackground)
{
    if (!control->GetVisibilityFlag() || control->IsHiddenForDebug())
        return;

    UIGeometricData drawData = control->GetLocalGeometricData();
    drawData.AddGeometricData(geometricData);

    const Color& parentColor = parentBackground ? parentBackground->GetDrawColor() : Color::White;

    control->SetParentColor(parentColor);

    const Rect& unrotatedRect = drawData.GetUnrotatedRect();

    UIClipContentComponent* clipContent = control->GetComponent<UIClipContentComponent>();
    bool clipContents = (clipContent != nullptr && clipContent->IsEnabled());
    if (clipContents)
    { //WARNING: for now clip contents don't work for rotating controls if you have any ideas you are welcome
        renderSystem2D->PushClip();
        renderSystem2D->IntersectClipRect(unrotatedRect); //anyway it doesn't work with rotation
    }

    control->Draw(drawData);
    const UITextComponent* txt = control->GetComponent<UITextComponent>();
    if (txt)
    {
        RenderText(control, txt, drawData, parentColor);
    }

    const UIControlBackground* bg = control->GetComponent<UIControlBackground>();
    const UIControlBackground* parentBgForChild = bg ? bg : parentBackground;
    control->isIteratorCorrupted = false;
    for (const auto& child : control->GetChildren())
    {
        RenderControlHierarhy(child.Get(), drawData, parentBgForChild);
        DVASSERT(!control->isIteratorCorrupted);
    }

    control->DrawAfterChilds(drawData);

    if (clipContents)
    {
        renderSystem2D->PopClip();
    }

    const UIDebugRenderComponent* debugRenderComponent = control->GetComponent<UIDebugRenderComponent>();
    if (debugRenderComponent && debugRenderComponent->IsEnabled())
    {
        DebugRender(debugRenderComponent, drawData);
    }
}

void UIRenderSystem::DebugRender(const UIDebugRenderComponent* component, const UIGeometricData& geometricData)
{
    renderSystem2D->PushClip();
    renderSystem2D->RemoveClip();
    RenderDebugRect(component, geometricData);
    UIDebugRenderComponent::ePivotPointDrawMode drawMode = component->GetPivotPointDrawMode();
    if (drawMode != UIDebugRenderComponent::DRAW_NEVER &&
        (drawMode != UIDebugRenderComponent::DRAW_ONLY_IF_NONZERO || !component->GetControl()->GetPivotPoint().IsZero()))
    {
        RenderPivotPoint(component, geometricData);
    }
    renderSystem2D->PopClip();
}

void UIRenderSystem::RenderDebugRect(const UIDebugRenderComponent* component, const UIGeometricData& geometricData)
{
    const Color& drawColor = component->GetDrawColor();

    if (geometricData.angle != 0.0f)
    {
        Polygon2 poly;
        geometricData.GetPolygon(poly);

        renderSystem2D->DrawPolygon(poly, true, drawColor);
    }
    else
    {
        renderSystem2D->DrawRect(geometricData.GetUnrotatedRect(), drawColor);
    }
}

void UIRenderSystem::RenderPivotPoint(const UIDebugRenderComponent* component, const UIGeometricData& geometricData)
{
    static const float32 PIVOT_POINT_MARK_RADIUS = 10.0f;
    static const float32 PIVOT_POINT_MARK_HALF_LINE_LENGTH = 13.0f;

    const Color& drawColor = component->GetDrawColor();

    Vector2 pivotPointCenter = geometricData.GetUnrotatedRect().GetPosition() + component->GetControl()->GetPivotPoint() * geometricData.scale;
    renderSystem2D->DrawCircle(pivotPointCenter, PIVOT_POINT_MARK_RADIUS, drawColor);

    // Draw the cross mark.
    Vector2 lineStartPoint = pivotPointCenter;
    Vector2 lineEndPoint = pivotPointCenter;
    lineStartPoint.y -= PIVOT_POINT_MARK_HALF_LINE_LENGTH;
    lineEndPoint.y += PIVOT_POINT_MARK_HALF_LINE_LENGTH;
    renderSystem2D->DrawLine(lineStartPoint, lineEndPoint, drawColor);

    lineStartPoint = pivotPointCenter;
    lineEndPoint = pivotPointCenter;
    lineStartPoint.x -= PIVOT_POINT_MARK_HALF_LINE_LENGTH;
    lineEndPoint.x += PIVOT_POINT_MARK_HALF_LINE_LENGTH;
    renderSystem2D->DrawLine(lineStartPoint, lineEndPoint, drawColor);
}

void UIRenderSystem::RenderText(const UIControl* control, const UITextComponent* component, const UIGeometricData& geometricData_, const Color& parentColor)
{
    UIGeometricData geometricData(geometricData_);
    geometricData.position += component->GetTextOffset() * geometricData.scale;

    UITextSystemLink* link = component->GetLink();
    DVASSERT(link, "Empty text comonent link!");

    UIControlBackground* textBg = link->GetTextBackground();
    UIControlBackground* shadowBg = link->GetShadowBackground();
    TextBlock* textBlock = link->GetTextBlock();

    shadowBg->SetParentColor(parentColor);
    textBg->SetParentColor(parentColor);

    if (component->GetText().empty())
    {
        return;
    }

    Rect textBlockRect(geometricData.position, geometricData.size);
    if (textBlock->GetFont() && textBlock->GetFont()->GetFontType() == Font::TYPE_DISTANCE)
    {
        // Correct rect and setup position and scale for distance fonts
        textBlockRect.dx *= geometricData.scale.dx;
        textBlockRect.dy *= geometricData.scale.dy;
        textBlock->SetScale(geometricData.scale);
        textBlock->SetAngle(geometricData.angle);
        textBlock->SetPivot(control->GetPivotPoint() * geometricData.scale);
    }
    textBlock->SetRectSize(textBlockRect.GetSize());
    textBlock->SetPosition(textBlockRect.GetPosition());
    textBlock->PreDraw();

    RenderTextDetails::PrepareSprite(link);

    textBg->SetAlign(textBlock->GetVisualAlign());

    UIGeometricData textGeomData;
    textGeomData.position = textBlock->GetSpriteOffset();
    textGeomData.size = control->GetSize();
    textGeomData.AddGeometricData(geometricData);

    Vector2 shadowOffset = component->GetShadowOffset();

    if (!FLOAT_EQUAL(shadowBg->GetDrawColor().a, 0.0f) && (!FLOAT_EQUAL(shadowOffset.dx, 0.0f) || !FLOAT_EQUAL(shadowOffset.dy, 0.0f)))
    {
        textBlock->Draw(shadowBg->GetDrawColor(), &shadowOffset);
        UIGeometricData shadowGeomData;
        shadowGeomData.position = shadowOffset;
        shadowGeomData.size = control->GetSize();
        shadowGeomData.AddGeometricData(textGeomData);

        shadowBg->SetAlign(textBg->GetAlign());
        shadowBg->Draw(shadowGeomData);
    }

    textBlock->Draw(textBg->GetDrawColor());

    textBg->Draw(textGeomData);
     
#if defined(LOCALIZATION_DEBUG)
    RenderTextDetails::DrawDebug(link, geometricData);
#endif
}

namespace RenderTextDetails
{
static void PrepareSprite(const UITextSystemLink* link)
{
    UIControlBackground* textBg = link->GetTextBackground();
    UIControlBackground* shadowBg = link->GetShadowBackground();
    TextBlock* textBlock = link->GetTextBlock();

    if (textBlock->IsSpriteReady())
    {
        Sprite* sprite = textBlock->GetSprite();
        shadowBg->SetSprite(sprite, 0);
        textBg->SetSprite(sprite, 0);

        Texture* tex = sprite->GetTexture();
        if (tex && tex->GetFormat() == FORMAT_A8)
        {
            textBg->SetMaterial(RenderSystem2D::DEFAULT_2D_TEXTURE_ALPHA8_MATERIAL);
            shadowBg->SetMaterial(RenderSystem2D::DEFAULT_2D_TEXTURE_ALPHA8_MATERIAL);
        }
        else
        {
            textBg->SetMaterial(RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL);
            shadowBg->SetMaterial(RenderSystem2D::DEFAULT_2D_TEXTURE_MATERIAL);
        }
    }
    else
    {
        shadowBg->SetSprite(NULL, 0);
        textBg->SetSprite(NULL, 0);
    }
}


#if defined(LOCALIZATION_DEBUG)
enum DebugHighliteColor
{
    RED = 0,
    BLUE,
    YELLOW,
    WHITE,
    MAGENTA,
    GREEN,
    NONE
};

static const float32 LOCALIZATION_RESERVED_PORTION = 0.6f;
static const Color HIGHLIGHT_COLORS[] = { DAVA::Color(1.0f, 0.0f, 0.0f, 0.4f),
                                          DAVA::Color(0.0f, 0.0f, 1.0f, 0.4f),
                                          DAVA::Color(1.0f, 1.0f, 0.0f, 0.4f),
                                          DAVA::Color(1.0f, 1.0f, 1.0f, 0.4f),
                                          DAVA::Color(1.0f, 0.0f, 1.0f, 0.4f),
                                          DAVA::Color(0.0f, 1.0f, 0.0f, 0.4f) };

static void DrawDebug(const UITextSystemLink* link, const UIGeometricData& geometricData)
{
    if (Renderer::GetOptions()->IsOptionEnabled(RenderOptions::DRAW_LINEBREAK_ERRORS) || Renderer::GetOptions()->IsOptionEnabled(RenderOptions::DRAW_LOCALIZATION_WARINGS))
    {
        DrawLocalizationDebug(link, geometricData);
    }
    if (Renderer::GetOptions()->IsOptionEnabled(RenderOptions::DRAW_LOCALIZATION_ERRORS))
    {
        DrawLocalizationErrors(link, geometricData);
    }
}

static void DrawLocalizationDebug(const UITextSystemLink* link, const UIGeometricData& textGeomData)
{
    TextBlock* textBlock = link->GetTextBlock();

    DebugHighliteColor warningColor = NONE;
    DebugHighliteColor lineBreakError = NONE;
    if (textBlock->GetFont() == NULL)
        return;

    if (textBlock->GetMultiline())
    {
        const Vector<WideString>& strings = textBlock->GetMultilineStrings();
        const WideString& text = textBlock->GetText();
        float32 accumulatedHeight = 0.0f;
        float32 maxWidth = 0.0f;

        if (!text.empty())
        {
            WideString textNoSpaces = StringUtils::RemoveNonPrintable(text, 1);
            // StringUtils::IsWhitespace function has 2 overloads and compiler cannot deduce predicate parameter for std::remove_if
            // So help compiler to choose correct overload of StringUtils::IsWhitespace function using static_cast
            auto res = remove_if(textNoSpaces.begin(), textNoSpaces.end(), static_cast<bool (*)(WideString::value_type)>(&StringUtils::IsWhitespace));
            textNoSpaces.erase(res, textNoSpaces.end());

            WideString concatinatedStringsNoSpaces = L"";
            for (Vector<WideString>::const_iterator string = strings.begin();
                 string != strings.end(); string++)
            {
                WideString toFilter = *string;
                toFilter.erase(remove_if(toFilter.begin(), toFilter.end(), static_cast<bool (*)(WideString::value_type)>(&StringUtils::IsWhitespace)), toFilter.end());
                concatinatedStringsNoSpaces += toFilter;
            }

            if (concatinatedStringsNoSpaces != textNoSpaces)
            {
                lineBreakError = RED;
            }
        }
    }

    if (warningColor != NONE && Renderer::GetOptions()->IsOptionEnabled(RenderOptions::DRAW_LOCALIZATION_WARINGS))
    {
        DAVA::Polygon2 polygon;
        textGeomData.GetPolygon(polygon);
        RenderSystem2D::Instance()->DrawPolygon(polygon, true, HIGHLIGHT_COLORS[warningColor]);
    }
    if (lineBreakError != NONE && Renderer::GetOptions()->IsOptionEnabled(RenderOptions::DRAW_LINEBREAK_ERRORS))
    {
        DAVA::Polygon2 polygon;
        textGeomData.GetPolygon(polygon);
        RenderSystem2D::Instance()->FillPolygon(polygon, HIGHLIGHT_COLORS[lineBreakError]);
    }
    if (textBlock->GetFittingOption() != 0 && Renderer::GetOptions()->IsOptionEnabled(RenderOptions::DRAW_LOCALIZATION_WARINGS))
    {
        Color color = HIGHLIGHT_COLORS[WHITE];
        if (textBlock->GetFittingOptionUsed() != 0)
        {
            if (textBlock->GetFittingOptionUsed() & TextBlock::FITTING_REDUCE)
                color = HIGHLIGHT_COLORS[RED];
            if (textBlock->GetFittingOptionUsed() & TextBlock::FITTING_ENLARGE)
                color = HIGHLIGHT_COLORS[YELLOW];
            if (textBlock->GetFittingOptionUsed() & TextBlock::FITTING_POINTS)
                color = HIGHLIGHT_COLORS[BLUE];
        }
        DAVA::Polygon2 polygon;
        textGeomData.GetPolygon(polygon);
        DVASSERT(polygon.GetPointCount() == 4);
        RenderSystem2D::Instance()->DrawLine(polygon.GetPoints()[0], polygon.GetPoints()[2], color);
    }
}

static void DrawLocalizationErrors(const UITextSystemLink* link, const UIGeometricData& geometricData)
{
    UIControlBackground* textBg = link->GetTextBackground();
    TextBlock* textBlock = link->GetTextBlock();

    UIGeometricData elementGeomData;
    const SpriteDrawState& lastDrawStae = textBg->GetLastDrawState();
    elementGeomData.position = lastDrawStae.position;
    elementGeomData.angle = lastDrawStae.angle;
    elementGeomData.scale = lastDrawStae.scale;
    elementGeomData.pivotPoint = lastDrawStae.pivotPoint;

    TextBlockSoftwareRender* rendereTextBlock = dynamic_cast<TextBlockSoftwareRender*>(textBlock->GetRenderer());
    if (rendereTextBlock != NULL)
    {
        DAVA::Matrix3 transform;
        elementGeomData.BuildTransformMatrix(transform);

        UIGeometricData textGeomData(elementGeomData);

        Vector3 x3 = Vector3(1.0f, 0.0f, 0.0f) * transform, y3 = Vector3(0.0f, 1.0f, 0.0f) * transform;
        Vector2 x(x3.x, x3.y), y(y3.x, y3.y);

        //reduce size by 1 pixel from each size for polygon to fit into control hence +1.0f and -1.0f
        //getTextOffsetTL and getTextOffsetBR are in physical coordinates but draw is still in virtual
        textGeomData.position += (x * GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualX(rendereTextBlock->getTextOffsetTL().x + 1.0f));
        textGeomData.position += (y * GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualY(rendereTextBlock->getTextOffsetTL().y + 1.0f));

        textGeomData.size = Vector2(0.0f, 0.0f);
        textGeomData.size.x += GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualX((rendereTextBlock->getTextOffsetBR().x - rendereTextBlock->getTextOffsetTL().x) - 1.0f);
        textGeomData.size.y += GetEngineContext()->uiControlSystem->vcs->ConvertPhysicalToVirtualY((rendereTextBlock->getTextOffsetBR().y - rendereTextBlock->getTextOffsetTL().y) - 1.0f);

        DAVA::Polygon2 textPolygon;
        textGeomData.GetPolygon(textPolygon);

        DAVA::Polygon2 controllPolygon;
        geometricData.GetPolygon(controllPolygon);

        //polygons will have te same transformation so just compare them
        if (!controllPolygon.IsPointInside(textPolygon.GetPoints()[0]) ||
            !controllPolygon.IsPointInside(textPolygon.GetPoints()[1]) ||
            !controllPolygon.IsPointInside(textPolygon.GetPoints()[2]) ||
            !controllPolygon.IsPointInside(textPolygon.GetPoints()[3]))
        {
            RenderSystem2D::Instance()->DrawPolygon(textPolygon, true, HIGHLIGHT_COLORS[MAGENTA]);
            RenderSystem2D::Instance()->FillPolygon(controllPolygon, HIGHLIGHT_COLORS[RED]);
        }
        if (textBlock->IsVisualTextCroped())
        {
            RenderSystem2D::Instance()->FillPolygon(textPolygon, HIGHLIGHT_COLORS[YELLOW]);
        }
    }
}

#endif
}
}
