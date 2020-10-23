#include "UI/Preview/Guides/GuidesController.h"
#include "UI/Preview/Guides/GuideLabel.h"
#include "Modules/CanvasModule/CanvasData.h"
#include "UI/Preview/Data/CentralWidgetData.h"

#include "Modules/DocumentsModule/DocumentData.h"
#include "Modules/PreferencesModule/PreferencesData.h"
#include "Modules/UpdateViewsSystemModule/UpdateViewsSystem.h"

#include "QECommands/SetGuidesCommand.h"

#include <TArc/Core/FieldBinder.h>

#include <QtTools/Updaters/LazyUpdater.h>

#include <Engine/Engine.h>
#include <UI/UIControlSystem.h>
#include <Reflection/ReflectedTypeDB.h>
#include <Logger/Logger.h>
#include "EditorSystems/UserAssetsSettings.h"

GuidesController::GuidesController(DAVA::Vector2::eAxis orientation_, DAVA::ContextAccessor* accessor_, QWidget* container_)
    : orientation(orientation_)
    , accessor(accessor_)
    , fieldBinder(new DAVA::FieldBinder(accessor))
    , container(container_)
    , canvasDataAdapter(accessor)
{
    UpdateViewsSystem* updateSystem = DAVA::GetEngineContext()->uiControlSystem->GetSystem<UpdateViewsSystem>();
    updateSystem->beforeRender.Connect(this, &GuidesController::SyncGuidesWithValues);

    documentDataWrapper = accessor->CreateWrapper(DAVA::ReflectedTypeDB::Get<DocumentData>());
    preferencesDataWrapper = accessor->CreateWrapper(DAVA::ReflectedTypeDB::Get<PreferencesData>());
    canvasDataAdapterWrapper = accessor->CreateWrapper([this](const DAVA::DataContext*) { return DAVA::Reflection::Create(&canvasDataAdapter); });
    canvasDataAdapterWrapper.SetListener(this);

    BindFields();
    CreatePreviewGuide();
}

void GuidesController::BindFields()
{
    using namespace DAVA;

    {
        FieldDescriptor fieldDescr;
        fieldDescr.type = ReflectedTypeDB::Get<DocumentData>();
        fieldDescr.fieldName = DocumentData::displayedRootControlsPropertyName;
        fieldBinder->BindField(fieldDescr, MakeFunction(this, &GuidesController::OnRootControlsChanged));
    }
    {
        FieldDescriptor fieldDescr;
        fieldDescr.type = ReflectedTypeDB::Get<CentralWidgetData>();
        fieldDescr.fieldName = CentralWidgetData::guidesPosPropertyName;
        fieldBinder->BindField(fieldDescr, MakeFunction(this, &GuidesController::OnCanvasParametersChanged));
    }
    {
        FieldDescriptor fieldDescr;
        fieldDescr.type = ReflectedTypeDB::Get<CentralWidgetData>();
        fieldDescr.fieldName = CentralWidgetData::guidesSizePropertyName;
        fieldBinder->BindField(fieldDescr, MakeFunction(this, &GuidesController::OnCanvasParametersChanged));
    }
    {
        FieldDescriptor fieldDescr;
        fieldDescr.type = ReflectedTypeDB::Get<CentralWidgetData>();
        fieldDescr.fieldName = CentralWidgetData::guidesRelativePosPropertyName;
        fieldBinder->BindField(fieldDescr, MakeFunction(this, &GuidesController::OnCanvasParametersChanged));
    }
    {
        FieldDescriptor fieldDescr;
        fieldDescr.type = ReflectedTypeDB::Get<CanvasData>();
        fieldDescr.fieldName = CanvasData::scalePropertyName;
        fieldBinder->BindField(fieldDescr, MakeFunction(this, &GuidesController::OnCanvasParametersChanged));
    }

    {
        FieldDescriptor fieldDescr;
        fieldDescr.type = ReflectedTypeDB::Get<UserAssetsSettings>();
        fieldDescr.fieldName = FastName("guidesColor");
        fieldBinder->BindField(fieldDescr, MakeFunction(this, &GuidesController::OnGuidesColorChanged));
    }

    {
        FieldDescriptor fieldDescr;
        fieldDescr.type = ReflectedTypeDB::Get<UserAssetsSettings>();
        fieldDescr.fieldName = FastName("previewGuideColor");
        fieldBinder->BindField(fieldDescr, MakeFunction(this, &GuidesController::OnPreviewGuideColorChanged));
    }
}

void GuidesController::OnCanvasParametersChanged(const DAVA::Any&)
{
    //canvas parameters can be changed during animation process and guides will not work correctly
    SetDisplayState(NO_DISPLAY);
    DisableDrag();
}

void GuidesController::OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
{
    bool startValueChanged = std::find(fields.begin(), fields.end(), CanvasDataAdapter::startValuePropertyName) != fields.end();
    bool lastValueChanged = std::find(fields.begin(), fields.end(), CanvasDataAdapter::lastValuePropertyName) != fields.end();
    if (startValueChanged || lastValueChanged)
    {
        OnCanvasParametersChanged(DAVA::Any());
    }
}

void GuidesController::OnMousePress(DAVA::float32 position)
{
    if (IsEnabled() == false || displayState == NO_DISPLAY)
    {
        return;
    }

    //if guides was disabled - enable guides and create new one
    if (IsGuidesEnabled() == false)
    {
        SetGuidesEnabled(true);
        SetDisplayState(DISPLAY_PREVIEW);
    }

    if (displayState == DISPLAY_PREVIEW)
    {
        PackageNode::AxisGuides values = GetValues();

        values.push_back(PositionToValue(position));

        SetValues(values);

        SyncGuidesWithValues();

        previewGuide.Raise();

        SetDisplayState(DISPLAY_DRAG);
    }

    EnableDrag(position);
}

void GuidesController::OnMouseMove(DAVA::float32 position)
{
    if (IsEnabled() == false || IsGuidesEnabled() == false)
    {
        return;
    }

    if (dragState == DRAG)
    {
        DragGuide(position);
    }
    else
    {
        PackageNode::AxisGuides::iterator valuePtr = GetNearestValuePtr(position);
        if (valuePtr != cachedValues.end())
        {
            SetDisplayState(DISPLAY_DRAG);
        }
        else
        {
            SetDisplayState(DISPLAY_PREVIEW);
            SetupPreviewGuide(position);
        }
    }
}

void GuidesController::OnMouseRelease(DAVA::float32 position)
{
    if (IsEnabled() == false || IsGuidesEnabled() == false)
    {
        return;
    }

    //copy  pointer to dragged item to remove it if we drag outside of screen
    DisableDrag();

    if (displayState == DISPLAY_REMOVE)
    {
        RemoveGuide(GetValues().back());
        SetDisplayState(NO_DISPLAY);
    }
}

void GuidesController::OnMouseLeave()
{
    if (IsEnabled() == false || IsGuidesEnabled() == false)
    {
        return;
    }
    SetDisplayState(NO_DISPLAY);
}

QList<QAction*> GuidesController::GetActions(DAVA::float32 position, QObject* parent)
{
    //we can call context menu when we dragging guide
    DisableDrag();

    QList<QAction*> actions;

    QAction* guidesEnabledAction = new QAction("Show Alignment Guides", parent);
    guidesEnabledAction->setCheckable(true);
    guidesEnabledAction->setChecked(IsGuidesEnabled());
    connect(guidesEnabledAction, &QAction::toggled, this, &GuidesController::SetGuidesEnabled);

    actions << guidesEnabledAction;

    if (IsEnabled() == false || IsGuidesEnabled() == false)
    {
        return actions;
    }

    PackageNode::AxisGuides::iterator valuePtr = GetNearestValuePtr(position);
    if (valuePtr != cachedValues.end())
    {
        QAction* removeGuideAction = new QAction("Remove Guide", parent);
        connect(removeGuideAction, &QAction::triggered, std::bind(&GuidesController::RemoveGuide, this, *valuePtr));
        actions << removeGuideAction;
    }

    QAction* removeAllGuidesAction = new QAction(QString("Remove All %1 Guides").arg(orientation == DAVA::Vector2::AXIS_X ? "Vertical" : "Horizontal"), parent);
    connect(removeAllGuidesAction, &QAction::triggered, this, &GuidesController::RemoveAllGuides);
    if (GetValues().empty())
    {
        removeAllGuidesAction->setEnabled(false);
    }
    actions << removeAllGuidesAction;

    return actions;
}

void GuidesController::SetDisplayState(eDisplayState state)
{
    if (displayState == state)
    {
        return;
    }

    //leave current state
    switch (displayState)
    {
    case DISPLAY_PREVIEW:
        previewGuide.Hide();
        break;
    case DISPLAY_DRAG:
    case DISPLAY_REMOVE:
        container->unsetCursor();
        break;
    default:
        break;
    }

    displayState = state;

    switch (displayState)
    {
    case DISPLAY_PREVIEW:
        previewGuide.Show();
        previewGuide.Raise();
        break;
    case DISPLAY_DRAG:
        container->setCursor(orientation == DAVA::Vector2::AXIS_X ? Qt::SplitHCursor : Qt::SplitVCursor);
        break;
    case DISPLAY_REMOVE:
        container->setCursor(QCursor(QPixmap(":/Cursors/trashCursor.png")));
    default:
        break;
    }
}

void GuidesController::EnableDrag(DAVA::float32 position)
{
    DVASSERT(IsEnabled());
    if (dragState == DRAG)
    {
        return;
    }

    valuePtr = GetNearestValuePtr(position);

    DVASSERT(valuePtr != cachedValues.end());
    if (valuePtr != cachedValues.end())
    {
        dragState = DRAG;

        DAVA::DataContext* active = accessor->GetActiveContext();
        DocumentData* data = active->GetData<DocumentData>();
        data->BeginBatch("Dragging guide");

        //move dragged guide to the end of list to display it under all other guides
        cachedValues.splice(cachedValues.end(), cachedValues, valuePtr);
    }
}

void GuidesController::DisableDrag()
{
    if (dragState != DRAG)
    {
        return;
    }

    DAVA::DataContext* active = accessor->GetActiveContext();
    DocumentData* data = active->GetData<DocumentData>();
    data->EndBatch();

    dragState = NO_DRAG;
}

void GuidesController::OnRootControlsChanged(const DAVA::Any& rootControls)
{
    //this is not good situation, but we can reload or close document by shortcut while we dragging guide
    SetDisplayState(NO_DISPLAY);
    DisableDrag();
    SyncGuidesWithValues();
}

void GuidesController::SyncGuidesWithValues()
{
    using namespace DAVA;

    if (IsEnabled() == false || IsGuidesEnabled() == false)
    {
        while (guides.empty() == false)
        {
            if (RemoveLastGuideWidget() == false)
            {
                //whoops, we trying to remove guide while it calling show() inside;
                //will try to remove it next frame
                return;
            }
        }
        return;
    }

    float32 scale = canvasDataAdapter.GetScale();
    float32 minValue = canvasDataAdapter.GetStartValue()[orientation] / scale;
    float32 maxValue = canvasDataAdapter.GetLastValue()[orientation] / scale;

    PackageNode::AxisGuides values = GetValues();
    PackageNode::AxisGuides visibleValues;
    for (float32 value : values)
    {
        if (value > minValue && value < maxValue)
        {
            visibleValues.push_back(value);
        }
    }

    std::size_t size = visibleValues.size();

    UserAssetsSettings* settings = accessor->GetGlobalContext()->GetData<UserAssetsSettings>();

    while (guides.size() > size)
    {
        if (RemoveLastGuideWidget() == false)
        {
            //whoops, we trying to remove guide while it calling show() inside;
            //will try to remove it next frame
            return;
        }
    }
    while (guides.size() < size)
    {
        Guide guide = CreateGuide(settings->guidesColor);
        guides.append(guide);
    }

    DVASSERT(size == guides.size());
    int index = 0;
    for (float32 value : visibleValues)
    {
        Guide& guide = guides[index++];
        ResizeGuide(guide);
        guide.text->SetValue(value);
        MoveGuide(value, guide);
    }
    //we can not use QWidget::show inside this SyncGuidesWithValues method, because it can be called by FrameUpdater and will cause processFrames inside another frame
    delayedExecutor.DelayedExecute([this]() {
        for (Guide& guide : guides)
        {
            //be afraid, because this guide.show may produce next frame
            //and during this frame scene position may change and in this case all guides will be removed
            guide.Show();
            guide.Raise();
        }
    });
}

PackageNode::AxisGuides::iterator GuidesController::GetNearestValuePtr(DAVA::float32 position)
{
    using namespace DAVA;

    float32 range = 1;

    float32 scale = canvasDataAdapter.GetScale();
    if (scale < 1.0f)
    {
        range = 3 / scale;
    }

    cachedValues = GetValues();

    if (cachedValues.empty())
    {
        return cachedValues.end();
    }

    float32 value = PositionToValue(position);

    PackageNode::AxisGuides::iterator iter = std::min_element(cachedValues.begin(), cachedValues.end(), [value](float32 left, float32 right)
                                                              {
                                                                  return std::abs(left - value) < std::abs(right - value);
                                                              });
    if (std::fabs(*iter - value) >= range)
    {
        return cachedValues.end();
    }
    return iter;
}

bool GuidesController::IsEnabled() const
{
    if (documentDataWrapper.HasData())
    {
        SortedControlNodeSet displayedRootControls = documentDataWrapper.GetFieldValue(DocumentData::displayedRootControlsPropertyName).Cast<SortedControlNodeSet>(SortedControlNodeSet());
        if (displayedRootControls.size() == 1)
        {
            return true;
        }
    }
    return false;
}

PackageNode::AxisGuides GuidesController::GetValues() const
{
    PackageNode::Guides guides = documentDataWrapper.GetFieldValue(DocumentData::guidesPropertyName).Cast<PackageNode::Guides>(PackageNode::Guides());
    return guides[orientation];
}

void GuidesController::SetValues(const PackageNode::AxisGuides& values)
{
    using namespace DAVA;

    DataContext* activeContext = accessor->GetActiveContext();
    DocumentData* data = activeContext->GetData<DocumentData>();
    const SortedControlNodeSet& rootControls = data->GetDisplayedRootControls();
    Q_ASSERT(rootControls.size() == 1);
    DAVA::String name = (*rootControls.begin())->GetName();

    data->ExecCommand<SetGuidesCommand>(name, orientation, values);
}

void GuidesController::CreatePreviewGuide()
{
    DVASSERT(previewGuide.line == nullptr && previewGuide.text == nullptr);
    UserAssetsSettings* settings = accessor->GetGlobalContext()->GetData<UserAssetsSettings>();
    previewGuide = CreateGuide(settings->previewGuideColor);
    previewGuide.Hide();
}

void GuidesController::SetupPreviewGuide(DAVA::float32 position)
{
    DAVA::float32 value = PositionToValue(position);
    ResizeGuide(previewGuide);
    previewGuide.text->SetValue(value);
    MoveGuide(value, previewGuide);
}

Guide GuidesController::CreateGuide(const DAVA::Color& color) const
{
    Guide guide;
    guide.line = new QWidget(container);
    guide.text = new GuideLabel(orientation, container);

    guide.line->setAttribute(Qt::WA_TransparentForMouseEvents);
    guide.text->setAttribute(Qt::WA_TransparentForMouseEvents);
    SetGuideColor(guide.line, color);
    return guide;
}

void GuidesController::DragGuide(DAVA::float32 position)
{
    using namespace DAVA;

    DVASSERT(IsEnabled());
    DVASSERT(dragState == DRAG);

    float32 value = PositionToValue(position);

    float32 scale = canvasDataAdapter.GetScale();
    float32 minValue = canvasDataAdapter.GetStartValue()[orientation] / scale;
    float32 maxValue = canvasDataAdapter.GetLastValue()[orientation] / scale;
    SetDisplayState((value < minValue || value > maxValue) ? DISPLAY_REMOVE : DISPLAY_DRAG);

    *valuePtr = PositionToValue(position);
    SetValues(cachedValues);

    SyncGuidesWithValues();
}

void GuidesController::RemoveGuide(DAVA::float32 value)
{
    cachedValues = GetValues();
    cachedValues.remove(value);
    SetValues(cachedValues);
}

void GuidesController::RemoveAllGuides()
{
    SetValues(PackageNode::AxisGuides());
}

bool GuidesController::IsGuidesEnabled() const
{
    return preferencesDataWrapper.GetFieldValue(PreferencesData::guidesEnabledPropertyName).Cast<bool>(true);
}

void GuidesController::SetGuidesEnabled(bool enabled)
{
    preferencesDataWrapper.SetFieldValue(PreferencesData::guidesEnabledPropertyName, enabled);
    if (enabled == false)
    {
        DisableDrag();
        SetDisplayState(NO_DISPLAY);
    }
}

void GuidesController::OnGuidesColorChanged(const DAVA::Any& c)
{
    DAVA::Color color = c.Cast<DAVA::Color>(DAVA::Color(DAVA::Color::Transparent));
    for (Guide& guide : guides)
    {
        SetGuideColor(guide.line, color);
    }
}

void GuidesController::OnPreviewGuideColorChanged(const DAVA::Any& c)
{
    DAVA::Color color = c.Cast<DAVA::Color>(DAVA::Color(DAVA::Color::Transparent));
    SetGuideColor(previewGuide.line, color);
}

void GuidesController::SetGuideColor(QWidget* guide, const DAVA::Color& color) const
{
    QString colorString = QString("rgba(%1, %2, %3, %4)")
                          .arg(color.r * 255.0f)
                          .arg(color.g * 255.0f)
                          .arg(color.b * 255.0f)
                          .arg(color.a * 255.0f);

    guide->setStyleSheet(QString("QWidget { background-color: %1; }").arg(colorString));
}

bool GuidesController::RemoveLastGuideWidget()
{
    Guide& guide = guides.last();
    if (guide.inWork)
    {
        return false;
    }

    delete guide.line;
    delete guide.text;
    guides.removeLast();
    return true;
}

CentralWidgetData* GuidesController::GetCentralWidgetData() const
{
    using namespace DAVA;

    DataContext* globalContext = accessor->GetGlobalContext();
    return globalContext->GetData<CentralWidgetData>();
}

DocumentData* GuidesController::GetDocumentData() const
{
    using namespace DAVA;

    DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    return activeContext->GetData<DocumentData>();
}

DAVA::float32 GuidesController::PositionToValue(DAVA::float32 position) const
{
    return canvasDataAdapter.MapFromScreenToRoot(position, orientation);
}

DAVA::float32 GuidesController::ValueToPosition(DAVA::float32 value) const
{
    using namespace DAVA;
    float32 relativePos = GetCentralWidgetData()->GetGuidesRelativePos()[orientation];
    float32 position = canvasDataAdapter.MapFromRootToScreen(value, orientation);
    return relativePos + position;
}

void GuidesController::ResizeGuide(Guide& guide) const
{
    using namespace DAVA;
    float32 size = GetCentralWidgetData()->GetGuidesSize()[orientation];
    if (orientation == Vector2::AXIS_X)
    {
        guide.line->resize(1, size);
        guide.text->resize(30, 15);
    }
    else
    {
        guide.line->resize(size, 1);
        guide.text->resize(15, 30);
    }
}

void GuidesController::MoveGuide(DAVA::float32 value, Guide& guide) const
{
    using namespace DAVA;
    float32 startPos = GetCentralWidgetData()->GetGuidesPos()[orientation];
    int32 position = static_cast<int32>(std::round(ValueToPosition(value)));
    if (orientation == Vector2::AXIS_X)
    {
        guide.line->move(position, startPos);
        guide.text->move(position + 5, startPos);
    }
    else
    {
        guide.line->move(startPos, position);
        guide.text->move(startPos, position - guide.text->height() - 5);
    }
}
