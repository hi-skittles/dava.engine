#include "Tests/ClipTest.h"
#include <UI/Render/UIClipContentComponent.h>
#include <UI/Render/UIDebugRenderComponent.h>

using namespace DAVA;

ClipTest::ClipTest(TestBed& app)
    : BaseScreen(app, "ClipTest")
{
}

void ClipTest::ClipPressed(BaseObject* obj, void* data, void* callerData)
{
    enableClip = !enableClip;
    clip->GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(enableClip);

    if (enableClip)
    {
        fullSizeWgt->GetOrCreateComponent<UIClipContentComponent>();
        parent1->GetOrCreateComponent<UIClipContentComponent>();
        parent2->GetOrCreateComponent<UIClipContentComponent>();
        child1->GetOrCreateComponent<UIClipContentComponent>();
        child2->GetOrCreateComponent<UIClipContentComponent>();
    }
    else
    {
        fullSizeWgt->RemoveComponent<UIClipContentComponent>();
        parent1->RemoveComponent<UIClipContentComponent>();
        parent2->RemoveComponent<UIClipContentComponent>();
        child1->RemoveComponent<UIClipContentComponent>();
        child2->RemoveComponent<UIClipContentComponent>();
    }
}

void ClipTest::DebugDrawPressed(BaseObject* obj, void* data, void* callerData)
{
    enableDebugDraw = !enableDebugDraw;

    debugDraw->GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(enableDebugDraw);

    fullSizeWgt->GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(enableDebugDraw);
    parent1->GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(enableDebugDraw);
    parent2->GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(enableDebugDraw);
    child1->GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(enableDebugDraw);
    child2->GetOrCreateComponent<UIDebugRenderComponent>()->SetEnabled(enableDebugDraw);
}

WideString ConvertToWString(const Rect& rect)
{
    std::wstringstream str;
    str << L"x = " << rect.x << L", y = " << rect.y << L", dx = " << rect.dx << L", dy =" << rect.dy << L".";
    return str.str();
}

void ClipTest::StartPos(BaseObject* obj, void* data, void* callerData)
{
    fullSizeWgt->SetRect(defaultRect);
    startPos->SetStateText(0xFF, ConvertToWString(defaultRect));
}

void ClipTest::MoveDown(BaseObject* obj, void* data, void* callerData)
{
    Rect rect = fullSizeWgt->GetRect();
    rect.y++;
    fullSizeWgt->SetRect(rect);
    startPos->SetStateText(0xFF, ConvertToWString(rect));
}

void ClipTest::MoveUp(BaseObject* obj, void* data, void* callerData)
{
    Rect rect = fullSizeWgt->GetRect();
    rect.y--;
    fullSizeWgt->SetRect(rect);
    startPos->SetStateText(0xFF, ConvertToWString(rect));
}

void ClipTest::MoveRight(BaseObject* obj, void* data, void* callerData)
{
    Rect rect = fullSizeWgt->GetRect();
    rect.x++;
    fullSizeWgt->SetRect(rect);
    startPos->SetStateText(0xFF, ConvertToWString(rect));
}

void ClipTest::MoveLeft(BaseObject* obj, void* data, void* callerData)
{
    Rect rect = fullSizeWgt->GetRect();
    rect.x--;
    fullSizeWgt->SetRect(rect);
    startPos->SetStateText(0xFF, ConvertToWString(rect));
}

void ClipTest::LoadResources()
{
    Font* font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
    DVASSERT(font);

    //start points
    Size2i screenSize = GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();
    float32 startX = 0.f, startY = 0.f, w = 0.f, h = 0.f;
    w = screenSize.dx * 0.25f;
    h = screenSize.dy * 0.05f;
    startX = screenSize.dx * 0.5f - w * 0.5f;
    startY = h;
    //full-size widget
    defaultRect = Rect(0.f, 0.f, static_cast<float32>(screenSize.dx), static_cast<float32>(screenSize.dy));
    fullSizeWgt = new UIControl(defaultRect);
    UIControlBackground* fullSizeWgtBg = fullSizeWgt->GetOrCreateComponent<UIControlBackground>();
    fullSizeWgtBg->SetColor(Color(0.f, 0.5f, 0.1f, 1.f));
    fullSizeWgtBg->SetDrawType(UIControlBackground::DRAW_FILL);
    fullSizeWgt->SetName("fullSizeWgt");
    AddControl(fullSizeWgt);

    float32 parentX(0.05f * startX), parentY(parentX), parentW(0.6f * startX), parentH(parentW);
    parent1 = new UIControl(Rect(parentX, parentY, parentW, parentH));
    UIControlBackground* parent1Bg = parent1->GetOrCreateComponent<UIControlBackground>();
    parent1Bg->SetColor(Color(0.5f, 0.f, 0.1f, 1.f));
    parent1Bg->SetDrawType(UIControlBackground::DRAW_FILL);
    parent1->SetName("parent1");
    child1 = new UIControl(Rect(parentW * 0.5f, parentH * 0.5f, parentW, parentH));
    UIControlBackground* child1Bg = child1->GetOrCreateComponent<UIControlBackground>();
    child1Bg->SetColor(Color(0.1f, 0.f, 0.5f, 1.f));
    child1Bg->SetDrawType(UIControlBackground::DRAW_FILL);
    child1->SetName("child1");
    parent1->AddControl(child1);
    AddControl(parent1);

    parentX = screenSize.dx - parentX - parentW;
    parentY += parentH * 0.5f;
    parent2 = new UIControl(Rect(parentX, parentY, parentW, parentH));
    UIControlBackground* parent2Bg = parent2->GetOrCreateComponent<UIControlBackground>();
    parent2Bg->SetColor(Color(0.1f, 0.f, 0.5f, 1.f));
    parent2Bg->SetDrawType(UIControlBackground::DRAW_FILL);
    parent2->SetName("parent2");
    child2 = new UIControl(Rect(-1.f * parentW * 0.5f, -1.f * parentH * 0.5f, parentW, parentH));
    UIControlBackground* child2Bg = child2->GetOrCreateComponent<UIControlBackground>();
    child2Bg->SetColor(Color(0.5f, 0.f, 0.1f, 1.f));
    child2Bg->SetDrawType(UIControlBackground::DRAW_FILL);
    child2->SetName("child2");
    parent2->AddControl(child2);
    AddControl(parent2);
    // button Clip
    clip = new UIButton(Rect(startX, startY, w, h));
    clip->SetStateFont(0xFF, font);
    clip->SetStateFontSize(0xFF, 13.f);
    clip->SetStateFontColor(0xFF, Color(1.0, 0.0, 0.0, 0.75));
    clip->SetStateText(0xFF, L"clip");
    clip->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &ClipTest::ClipPressed));
    AddControl(clip);
    startY += h + h * 0.5f;
    // button DebugDraw
    debugDraw = new UIButton(Rect(startX, startY, w, h));
    debugDraw->SetStateFont(0xFF, font);
    debugDraw->SetStateFontSize(0xFF, 13.f);
    debugDraw->SetStateFontColor(0xFF, Color(1.0, 0.0, 0.0, 0.75));
    debugDraw->SetStateText(0xFF, L"debugDraw");
    debugDraw->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &ClipTest::DebugDrawPressed));
    AddControl(debugDraw);
    startY += h + h * 0.5f;
    // button startPos
    startPos = new UIButton(Rect(startX, startY, w, h));
    startPos->SetStateFont(0xFF, font);
    startPos->SetStateFontSize(0xFF, 13.f);
    startPos->GetOrCreateComponent<UIDebugRenderComponent>();
    startPos->SetStateFontColor(0xFF, Color(1.0, 0.0, 0.0, 0.75));
    startPos->SetStateText(0xFF, L"startPos");
    startPos->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &ClipTest::StartPos));
    AddControl(startPos);
    startY += h + h * 0.5f;
    // button moveLeft
    moveLeft = new UIButton(Rect(startX, startY, w, h));
    moveLeft->SetStateFont(0xFF, font);
    moveLeft->SetStateFontSize(0xFF, 13.f);
    moveLeft->GetOrCreateComponent<UIDebugRenderComponent>();
    moveLeft->SetStateFontColor(0xFF, Color(1.0, 0.0, 0.0, 0.75));
    moveLeft->SetStateText(0xFF, L"moveLeft");
    moveLeft->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &ClipTest::MoveLeft));
    AddControl(moveLeft);
    startY += h + h * 0.5f;
    // button moveRight
    moveRight = new UIButton(Rect(startX, startY, w, h));
    moveRight->SetStateFont(0xFF, font);
    moveRight->SetStateFontSize(0xFF, 13.f);
    moveRight->GetOrCreateComponent<UIDebugRenderComponent>();
    moveRight->SetStateFontColor(0xFF, Color(1.0, 0.0, 0.0, 0.75));
    moveRight->SetStateText(0xFF, L"moveRight");
    moveRight->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &ClipTest::MoveRight));
    AddControl(moveRight);
    startY += h + h * 0.5f;
    // button moveUp
    moveUp = new UIButton(Rect(startX, startY, w, h));
    moveUp->SetStateFont(0xFF, font);
    moveUp->SetStateFontSize(0xFF, 13.f);
    moveUp->GetOrCreateComponent<UIDebugRenderComponent>();
    moveUp->SetStateFontColor(0xFF, Color(1.0, 0.0, 0.0, 0.75));
    moveUp->SetStateText(0xFF, L"moveUp");
    moveUp->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &ClipTest::MoveUp));
    AddControl(moveUp);
    startY += h + h * 0.5f;
    // button moveDown
    moveDown = new UIButton(Rect(startX, startY, w, h));
    moveDown->SetStateFont(0xFF, font);
    moveDown->SetStateFontSize(0xFF, 13.f);
    moveDown->GetOrCreateComponent<UIDebugRenderComponent>();
    moveDown->SetStateFontColor(0xFF, Color(1.0, 0.0, 0.0, 0.75));
    moveDown->SetStateText(0xFF, L"moveDown");
    moveDown->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, Message(this, &ClipTest::MoveDown));
    AddControl(moveDown);

    SafeRelease(font);
    BaseScreen::LoadResources();

    ClipPressed(nullptr, nullptr, nullptr);
    DebugDrawPressed(nullptr, nullptr, nullptr);
}

void ClipTest::UnloadResources()
{
    BaseScreen::UnloadResources();
    RemoveAllControls();
    SafeRelease(fullSizeWgt);
    SafeRelease(parent1);
    SafeRelease(child1);
    SafeRelease(parent2);
    SafeRelease(child2);

    SafeRelease(clip);
    SafeRelease(debugDraw);
    SafeRelease(startPos);
    SafeRelease(moveLeft);
    SafeRelease(moveRight);
    SafeRelease(moveUp);
    SafeRelease(moveDown);
    SafeRelease(fullSizeWgt);
}
