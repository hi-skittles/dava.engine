#include "OverdrawTestingScreen.h"

#include "ChartPainterSystem.h"
#include "OverdrawTestConfig.h"
#include "OverdrawTesterComponent.h"
#include "OverdrawTesterSystem.h"
#include "Infrastructure/TestBed.h"

#include "Base/Message.h"
#include "Base/String.h"
#include "Base/TemplateHelpers.h"
#include "Engine/Engine.h"
#include "Entity/ComponentUtils.h"
#include "Render/2D/FTFont.h"
#include "Render/2D/Systems/VirtualCoordinatesSystem.h"
#include "Render/Highlevel/Camera.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Components/CameraComponent.h"
#include "Scene3D/Systems/Controller/RotationControllerSystem.h"
#include "Scene3D/Systems/Controller/WASDControllerSystem.h"
#include "Math/Color.h"
#include "UI/UIScreen.h"
#include "UI/UI3DView.h"
#include "UI/UIControlSystem.h"
#include "UI/UIButton.h"
#include "UI/Layouts/UIAnchorComponent.h"
#include "UI/Render/UIDebugRenderComponent.h"

namespace OverdrawPerformanceTester
{
using OverdrawPerformanceTester::OverdrawTesterComponent;
using OverdrawPerformanceTester::OverdrawTesterSystem;
using OverdrawPerformanceTester::ChartPainterSystem;
using OverdrawPerformanceTester::FrameData;
using DAVA::Entity;
using DAVA::Camera;
using DAVA::Scene;
using DAVA::float32;
using DAVA::Color;
using DAVA::Vector3;
using DAVA::UI3DView;
using DAVA::UIControlSystem;
using DAVA::VirtualCoordinatesSystem;
using DAVA::CameraComponent;
using DAVA::Size2i;
using DAVA::Rect;
using DAVA::ScopedPtr;
using DAVA::FilePath;
using DAVA::Rect;
using DAVA::UIScreen;
using DAVA::UIAnchorComponent;
using DAVA::UIDebugRenderComponent;
using DAVA::UIButton;
using DAVA::Message;
using DAVA::FTFont;
using DAVA::WideString;

const float32 OverdrawTestingScreen::buttonWidth = 100.0f;
const float32 OverdrawTestingScreen::buttonHeight = 40.0f;
const float32 OverdrawTestingScreen::minFrametimeThreshold = 0.025f;
const float32 OverdrawTestingScreen::frametimeIncreaseStep = 0.008f;

OverdrawTestingScreen::OverdrawTestingScreen(TestBed& app_)
    : app(app_)
{
}

void OverdrawTestingScreen::LoadResources()
{
    scene = new Scene();
    scene->LoadScene(FilePath("~res:/TestBed/3d/Maps/overdraw_test/TestingScene.sc2"));

    if (font == nullptr)
    {
        font = FTFont::Create("~res:/TestBed/Fonts/korinna.ttf");
        DVASSERT(font);
    }

    testerSystem = new OverdrawTesterSystem(scene, OverdrawTestConfig::pixelFormat, OverdrawTestConfig::textureResolution,
                                            [this](DAVA::Array<DAVA::Vector<FrameData>, 6>* performanceData)
                                            {
                                                chartPainterSystem->ProcessPerformanceData(performanceData);
                                                AddButtons();
                                            });

    scene->AddSystem(testerSystem, DAVA::ComponentUtils::MakeMask<OverdrawTesterComponent>(), Scene::SCENE_SYSTEM_REQUIRE_PROCESS);

    chartPainterSystem = new ChartPainterSystem(scene, OverdrawTestConfig::chartHeight);
    scene->AddSystem(chartPainterSystem, DAVA::ComponentUtils::MakeMask<OverdrawTesterComponent>(), Scene::SCENE_SYSTEM_REQUIRE_PROCESS);

    ScopedPtr<Camera> camera(new Camera());

    VirtualCoordinatesSystem* vcs = DAVA::GetEngineContext()->uiControlSystem->vcs;

    float32 aspect = static_cast<float32>(vcs->GetVirtualScreenSize().dy) / static_cast<float32>(vcs->GetVirtualScreenSize().dx);
    camera->SetupPerspective(70.f, aspect, 0.5f, 2500.f);
    camera->SetLeft(Vector3(1, 0, 0));
    camera->SetUp(Vector3(0, 0, 1.f));
    camera->SetTarget(Vector3(0, 0, 0));
    camera->SetPosition(Vector3(0, -45, 10));

    scene->AddCamera(camera);
    scene->SetCurrentCamera(camera);

    Entity* cameraEntity = new Entity();
    cameraEntity->AddComponent(new CameraComponent(camera));
    scene->AddNode(cameraEntity);
    cameraEntity->Release();

    Entity* overdrawTesterEntity = new Entity();
    overdrawTesterEntity->AddComponent(new OverdrawPerformanceTester::OverdrawTesterComponent(OverdrawTestConfig::textureResolution, OverdrawTestConfig::overdrawScreensCount));
    scene->AddNode(overdrawTesterEntity);
    overdrawTesterEntity->Release();

    Rect screenRect = GetRect();
    Size2i screenSize = DAVA::GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();
    screenRect.dx = static_cast<float32>(screenSize.dx);
    screenRect.dy = static_cast<float32>(screenSize.dy);
    SetRect(screenRect);
    ScopedPtr<UI3DView> sceneView(new UI3DView(screenRect));
    sceneView->SetScene(scene);
    AddControl(sceneView);

    exitButton = CreateButton({ static_cast<float32>(screenSize.dx - 300), static_cast<float32>(screenSize.dy - 30), 300.0, 30.0 }, Message(this, &OverdrawTestingScreen::OnExitButton), L"Exit From Screen", 0);

    // Stick button to bottom right corner
    UIAnchorComponent* anchor = exitButton->GetOrCreateComponent<UIAnchorComponent>();
    anchor->SetBottomAnchorEnabled(true);
    anchor->SetRightAnchorEnabled(true);

    AddControl(exitButton);
}

void OverdrawTestingScreen::UnloadResources()
{
    RemoveAllControls();
    UIScreen::UnloadResources();

    scene->RemoveSystem(chartPainterSystem);
    SafeDelete(chartPainterSystem);

    scene->RemoveSystem(testerSystem);
    SafeDelete(testerSystem);

    SafeRelease(exitButton);
    SafeRelease(scene);

    SafeRelease(font);
}

void OverdrawTestingScreen::OnExitButton(DAVA::BaseObject* obj, void* data, void* callerData)
{
    app.ShowStartScreen();
}

void OverdrawTestingScreen::OnChangeChartHeightButtonClick(BaseObject* sender, void* data, void* callerData)
{
    UIButton* pickedButton = DAVA::DynamicTypeCheck<UIButton*>(sender);

    OverdrawTestConfig::chartHeight = DAVA::Max(minFrametimeThreshold, OverdrawTestConfig::chartHeight + pickedButton->GetTag() * frametimeIncreaseStep);
    chartPainterSystem->SetMaxFrametime(OverdrawTestConfig::chartHeight);
}

void OverdrawTestingScreen::AddButtons()
{
    Size2i size = DAVA::GetEngineContext()->uiControlSystem->vcs->GetVirtualScreenSize();
    frameTimeMinusButton = CreateButton({ 0, size.dy - buttonHeight, buttonWidth, buttonHeight }, Message(this, &OverdrawTestingScreen::OnChangeChartHeightButtonClick), L"-", -1);
    frameTimePlusButton = CreateButton({ buttonWidth, size.dy - buttonHeight, buttonWidth, buttonHeight }, Message(this, &OverdrawTestingScreen::OnChangeChartHeightButtonClick), L"+", 1);
}

UIButton* OverdrawTestingScreen::CreateButton(const Rect&& rect, const Message&& msg, const WideString&& caption, const DAVA::int32 tag)
{
    UIButton* button = new UIButton(rect);
    button->SetStateText(UIControl::STATE_NORMAL, caption);
    button->SetStateTextAlign(UIControl::STATE_NORMAL, DAVA::ALIGN_HCENTER | DAVA::ALIGN_VCENTER);
    button->SetStateFont(UIControl::STATE_NORMAL, font);
    button->SetStateFontColor(UIControl::STATE_NORMAL, Color::White);
    button->SetStateFontColor(UIControl::STATE_PRESSED_INSIDE, Color(0.7f, 0.7f, 0.7f, 1.f));
    button->AddEvent(UIControl::EVENT_TOUCH_UP_INSIDE, msg);
    button->GetOrCreateComponent<UIDebugRenderComponent>();
    button->SetTag(tag);
    AddControl(button);
    return button;
}
}
