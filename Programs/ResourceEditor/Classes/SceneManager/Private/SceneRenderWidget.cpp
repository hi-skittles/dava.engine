#include "Classes/SceneManager/Private/SceneRenderWidget.h"

#include "Classes/Qt/Scene/SceneSignals.h"

#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/Scene/Systems/ModifSystem.h>

#include <TArc/Controls/SceneTabbar.h>

#include <Base/RefPtrUtils.h>
#include <Base/StaticSingleton.h>
#include <Engine/EngineContext.h>
#include <Engine/Qt/RenderWidget.h>
#include <Functional/Function.h>
#include <Math/Vector.h>
#include <Reflection/ReflectedType.h>
#include <Render/2D/Systems/VirtualCoordinatesSystem.h>
#include <UI/Focus/UIFocusComponent.h>
#include <UI/UIControlSystem.h>
#include <UI/UIScreenManager.h>

#include <QVBoxLayout>

namespace SceneRenderWidgetDetails
{
bool CustomInput(DAVA::UIControl* control, DAVA::UIEvent* event)
{
    if (event->phase == DAVA::UIEvent::Phase::GESTURE)
    {
        control->Input(event);
    }
    return false;
};
}

SceneRenderWidget::SceneRenderWidget(DAVA::ContextAccessor* accessor_, DAVA::RenderWidget* renderWidget_, IWidgetDelegate* widgetDelegate_)
    : accessor(accessor_)
    , renderWidget(renderWidget_)
    , widgetDelegate(widgetDelegate_)
{
    using namespace DAVA;
    activeSceneWrapper = accessor->CreateWrapper(DAVA::ReflectedTypeDB::Get<SceneData>());
    activeSceneWrapper.SetListener(this);

    SceneTabbar* tabBar = new SceneTabbar(accessor, DAVA::Reflection::Create(&accessor), this);
    tabBar->setAcceptDrops(true);
    tabBar->setFocusPolicy(Qt::StrongFocus);
    tabBar->setTabsClosable(true);
    tabBar->setMovable(true);
    tabBar->setUsesScrollButtons(true);
    tabBar->setExpanding(false);
    tabBar->installEventFilter(this);

    setMinimumSize(renderWidget->minimumSize());
    renderWidget->SetClientDelegate(this);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(tabBar);
    layout->addWidget(renderWidget);
    layout->setMargin(1);
    layout->setSpacing(0);
    setLayout(layout);

    InitDavaUI();

    renderWidget->resized.Connect(this, &SceneRenderWidget::OnRenderWidgetResized);
    QObject::connect(SceneSignals::Instance(), &SceneSignals::MouseOverSelection, this, &SceneRenderWidget::OnMouseOverSelection);

    tabBar->closeTab.Connect(this, &SceneRenderWidget::OnCloseTab);
    addActions(tabBar->actions());
}

SceneRenderWidget::~SceneRenderWidget()
{
    UninitDavaUI();
    renderWidget->SetClientDelegate(nullptr);
}

void SceneRenderWidget::OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
{
    using namespace DAVA;

    auto iter = std::find(fields.begin(), fields.end(), DAVA::Any(DAVA::FastName(SceneData::scenePropertyName)));
    if (iter == fields.end() && !fields.empty() && wrapper.HasData())
    {
        return;
    }

    DAVA::RefPtr<SceneEditor2> currentScene;
    DataContext* activeContext = accessor->GetActiveContext();
    if (activeContext != nullptr)
    {
        SceneData* data = activeContext->GetData<SceneData>();
        if (data != nullptr)
        {
            currentScene = data->GetScene();
        }
    }

    if (currentScene.Get() != nullptr)
    {
        if (dava3DView->GetParent() == nullptr)
        {
            const auto& children = davaUIScreen->GetChildren();
            if (children.empty())
            {
                davaUIScreen->AddControl(dava3DView.Get());
            }
            else
            {
                davaUIScreen->InsertChildBelow(dava3DView.Get(), children.front().Get());
            }
        }

        dava3DView->SetScene(currentScene.Get());
        currentScene->SetViewportRect(dava3DView->GetRect());
    }
    else
    {
        dava3DView->SetScene(NULL);
        davaUIScreen->RemoveControl(dava3DView.Get());
    }
}

void SceneRenderWidget::InitDavaUI()
{
    dava3DView.Set(new DAVA::UI3DView(DAVA::Rect(dava3DViewMargin, dava3DViewMargin, 0, 0)));
    dava3DView->customSystemProcessInput = &SceneRenderWidgetDetails::CustomInput;
    dava3DView->SetInputEnabled(true, true);
    dava3DView->GetOrCreateComponent<DAVA::UIFocusComponent>();
    dava3DView->SetName(DAVA::FastName("Scene_Tab_3D_View"));

    davaUIScreen = DAVA::MakeRef<DAVA::UIScreen>();

    const DAVA::EngineContext* engineCtx = accessor->GetEngineContext();

    engineCtx->uiScreenManager->RegisterScreen(davaUIScreenID, davaUIScreen.Get());
    engineCtx->uiScreenManager->SetScreen(davaUIScreenID);
}

void SceneRenderWidget::UninitDavaUI()
{
    const DAVA::EngineContext* engineCtx = accessor->GetEngineContext();

    engineCtx->uiScreenManager->ResetScreen();
    engineCtx->uiScreenManager->UnregisterScreen(davaUIScreenID);

    davaUIScreen = nullptr;
}

void SceneRenderWidget::OnRenderWidgetResized(DAVA::uint32 w, DAVA::uint32 h)
{
    using namespace DAVA;

    DAVA::VirtualCoordinatesSystem* vcs = accessor->GetEngineContext()->uiControlSystem->vcs;
    vcs->SetVirtualScreenSize(w, h);
    vcs->UnregisterAllAvailableResourceSizes();
    vcs->RegisterAvailableResourceSize(w, h, "Gfx");

    davaUIScreen->SetSize(DAVA::Vector2(w, h));
    dava3DView->SetSize(DAVA::Vector2(w - 2 * dava3DViewMargin, h - 2 * dava3DViewMargin));

    DataContext* ctx = accessor->GetActiveContext();
    if (ctx == nullptr)
    {
        return;
    }

    SceneData* data = ctx->GetData<SceneData>();
    if (data == nullptr)
    {
        return;
    }

    DAVA::RefPtr<SceneEditor2> scene = data->GetScene();
    if (scene.Get() == nullptr)
    {
        return;
    }

    scene->SetViewportRect(dava3DView->GetRect());
}

void SceneRenderWidget::OnCloseTab(DAVA::uint64 id)
{
    DVASSERT(widgetDelegate);
    widgetDelegate->OnCloseSceneRequest(id);
}

void SceneRenderWidget::OnMouseOverSelection(DAVA::SceneEditor2* scene, const DAVA::SelectableGroup* objects)
{
    using namespace DAVA;

    static QCursor cursorMove(QPixmap(":/QtIcons/curcor_move.png"));
    static QCursor cursorRotate(QPixmap(":/QtIcons/curcor_rotate.png"));
    static QCursor cursorScale(QPixmap(":/QtIcons/curcor_scale.png"));

    DataContext* ctx = accessor->GetActiveContext();
    if (ctx == nullptr)
    {
        renderWidget->unsetCursor();
        return;
    }

    SceneData* data = ctx->GetData<SceneData>();

    if ((data->GetScene() == scene) && (objects != nullptr))
    {
        switch (scene->GetSystem<DAVA::EntityModificationSystem>()->GetTransformType())
        {
        case Selectable::TransformType::Translation:
            renderWidget->setCursor(cursorMove);
            break;
        case Selectable::TransformType::Rotation:
            renderWidget->setCursor(cursorRotate);
            break;
        case Selectable::TransformType::Scale:
            renderWidget->setCursor(cursorScale);
            break;
        case Selectable::TransformType::Disabled:
        default:
            renderWidget->unsetCursor();
            break;
        }
    }
    else
    {
        renderWidget->unsetCursor();
    }
}

bool SceneRenderWidget::eventFilter(QObject* object, QEvent* event)
{
    DVASSERT(widgetDelegate != nullptr);
    QEvent::Type eventType = event->type();
    switch (eventType)
    {
    case QEvent::DragEnter:
        widgetDelegate->OnDragEnter(object, static_cast<QDragEnterEvent*>(event));
        return true;
    case QEvent::DragMove:
        widgetDelegate->OnDragMove(object, static_cast<QDragMoveEvent*>(event));
        return true;
    case QEvent::Drop:
        widgetDelegate->OnDrop(object, static_cast<QDropEvent*>(event));
        return true;
    default:
        return false;
    }
}

void SceneRenderWidget::OnDragEntered(QDragEnterEvent* e)
{
    DVASSERT(widgetDelegate != nullptr);
    widgetDelegate->OnDragEnter(renderWidget, e);
}

void SceneRenderWidget::OnDragMoved(QDragMoveEvent* e)
{
    DVASSERT(widgetDelegate != nullptr);
    widgetDelegate->OnDragMove(renderWidget, e);
}

void SceneRenderWidget::OnDrop(QDropEvent* e)
{
    DVASSERT(widgetDelegate != nullptr);
    widgetDelegate->OnDrop(renderWidget, e);
}
