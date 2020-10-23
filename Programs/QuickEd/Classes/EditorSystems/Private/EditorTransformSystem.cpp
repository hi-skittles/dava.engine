#include "Classes/EditorSystems/EditorTransformSystem.h"
#include "Classes/EditorSystems/EditorSystemsManager.h"
#include "Classes/EditorSystems/ControlTransformationSettings.h"
#include "Classes/EditorSystems/MovableInEditorComponent.h"
#include "Classes/EditorSystems/CounterpoiseComponent.h"

#include "Classes/Model/PackageHierarchy/PackageNode.h"
#include "Classes/Model/PackageHierarchy/ControlNode.h"
#include "Classes/Model/ControlProperties/RootProperty.h"

#include "Classes/Modules/CanvasModule/CanvasDataAdapter.h"
#include "Classes/Modules/CanvasModule/CanvasData.h"
#include "Classes/Modules/DocumentsModule/DocumentData.h"
#include "Classes/Modules/PreferencesModule/PreferencesData.h"

#include "Classes/QECommands/ChangePropertyValueCommand.h"
#include "Classes/QECommands/ResizeCommand.h"
#include "Classes/QECommands/ChangePivotCommand.h"

#include <TArc/Utils/Utils.h>
#include <TArc/Core/ContextAccessor.h>

#include <Input/InputSystem.h>
#include <UI/UIEvent.h>
#include <UI/UIControl.h>
#include <Reflection/ReflectionRegistrator.h>
#include <Base/BaseTypes.h>
#include <Math/MathHelpers.h>

DAVA::Vector2 EditorTransformSystem::GetMinimumSize()
{
    return DAVA::Vector2(16.0f, 16.0f);
}

const EditorTransformSystem::CornersDirections EditorTransformSystem::cornersDirections =
{ {
{ { NEGATIVE_DIRECTION, NEGATIVE_DIRECTION } }, // TOP_LEFT_AREA
{ { NO_DIRECTION, NEGATIVE_DIRECTION } }, // TOP_CENTER_AREA
{ { POSITIVE_DIRECTION, NEGATIVE_DIRECTION } }, //TOP_RIGHT_AREA
{ { NEGATIVE_DIRECTION, NO_DIRECTION } }, //CENTER_LEFT_AREA
{ { POSITIVE_DIRECTION, NO_DIRECTION } }, //CENTER_RIGHT_AREA
{ { NEGATIVE_DIRECTION, POSITIVE_DIRECTION } }, //BOTTOM_LEFT_AREA
{ { NO_DIRECTION, POSITIVE_DIRECTION } }, //BOTTOM_CENTER_AREA
{ { POSITIVE_DIRECTION, POSITIVE_DIRECTION } } //BOTTOM_RIGHT_AREA
} };

struct EditorTransformSystem::MoveInfo
{
    MoveInfo(ControlNode* node_, AbstractProperty* positionProperty_, const DAVA::UIGeometricData* parentGD_)
        : node(node_)
        , positionProperty(positionProperty_)
        , parentGD(parentGD_)
    {
    }
    ControlNode* node = nullptr;
    AbstractProperty* positionProperty = nullptr;
    const DAVA::UIGeometricData* parentGD = nullptr;
};

struct EditorTransformSystem::MagnetLine
{
    //controlBox and targetBox in parent coordinates. controlSharePos ans targetSharePos is a share of corresponding size
    MagnetLine(DAVA::float32 controlSharePos_, const DAVA::Rect& controlBox_, DAVA::float32 targetSharePos, const DAVA::Rect& targetBox_, DAVA::Vector2::eAxis axis_)
        : controlSharePos(controlSharePos_)
        , controlBox(controlBox_)
        , targetBox(targetBox_)
        , axis(axis_)
    {
        controlPosition = controlBox.GetPosition()[axis] + controlBox.GetSize()[axis] * controlSharePos;
        targetPosition = targetBox.GetPosition()[axis] + targetBox.GetSize()[axis] * targetSharePos;
        interval = controlPosition - targetPosition;
    }

    MagnetLine(DAVA::float32 controlSharePos_, const DAVA::Rect& controlBox_, DAVA::float32 targetPosition_, DAVA::Vector2::eAxis axis_)
        : controlSharePos(controlSharePos_)
        , controlBox(controlBox_)
        , targetPosition(targetPosition_)
        , axis(axis_)
    {
        controlPosition = controlBox.GetPosition()[axis] + controlBox.GetSize()[axis] * controlSharePos;
        interval = controlPosition - targetPosition;
    }

    DAVA::float32 controlSharePos;
    DAVA::float32 controlPosition;
    DAVA::Rect controlBox;

    DAVA::float32 targetPosition;
    DAVA::Rect targetBox;

    DAVA::float32 interval = std::numeric_limits<DAVA::float32>::max();
    DAVA::Vector2::eAxis axis;
};

namespace EditorTransformSystemDetail
{
using namespace DAVA;
const float32 TRANSFORM_EPSILON = 0.0005f;

struct ChangePropertyAction
{
    ChangePropertyAction(ControlNode* node_, AbstractProperty* property_, const Any& value_)
        : node(node_)
        , property(property_)
        , value(value_)
    {
    }
    ControlNode* node = nullptr;
    AbstractProperty* property = nullptr;
    Any value;
};

//when we get request to add a value (x; y) to position it transforms to:
//for angle -45 : 45 returns (x; y)
//for angle 45 : (90 + 45) returns (y; -x)
//for angle (90 + 45) : (180 + 45) returns (-x; -y)
//for angle (180 + 45) : (360 - 45) returns (-y; x);

DAVA::Vector2 RotateVectorForMove(const DAVA::Vector2& delta, DAVA::float32 angle)
{
    static const DAVA::float32 positiveCos = std::cos(PI_025);
    static const DAVA::float32 negativeCos = std::cos(PI + PI_025);

    float32 cos = std::cos(angle);
    Vector2 deltaPosition;
    if (cos > positiveCos)
    {
        return delta;
    }
    else if (cos < negativeCos)
    {
        return Vector2(-delta.dx, -delta.dy);
    }
    else
    {
        float32 sin = std::sin(angle);
        if (sin > 0)
        {
            return Vector2(delta.dy, -delta.dx);
        }
        else
        {
            return Vector2(-delta.dy, delta.dx);
        }
    }
}

void ClampProperty(Vector2& propertyValue, Vector2& extraDelta)
{
    Vector2 clampedValue(std::floor(propertyValue.x + TRANSFORM_EPSILON), std::floor(propertyValue.y + TRANSFORM_EPSILON));
    extraDelta += (propertyValue - clampedValue);
    propertyValue = clampedValue;
}

void ClampProperty(float32& propertyValue, float32& extraDelta)
{
    float32 clampedValue(std::floor(propertyValue));
    extraDelta += (propertyValue - clampedValue);
    propertyValue = clampedValue;
}

using DeltaPositionBehavior = Function<void(Vector2& deltaPosition)>;

DAVA::Vector2 CreateFinalPosition(const DAVA::UIGeometricData* parentGd, const DAVA::Vector2& originalPosition, DAVA::Vector2& mouseDelta,
                                  DAVA::Vector2& extraDelta, DeltaPositionBehavior behavior = DeltaPositionBehavior())
{
    using namespace DAVA;
    //transform mouse delta to control coordinates
    Vector2 scaledDelta = mouseDelta / parentGd->scale;
    Vector2 deltaPosition(Rotate(scaledDelta, -parentGd->angle));

    //add delta from previous move event
    deltaPosition += extraDelta;
    extraDelta.SetZero();

    //call behavior if control must magnet somewhere
    if (behavior)
    {
        behavior(deltaPosition);
    }

    Vector2 finalPosition(originalPosition + deltaPosition);
    EditorTransformSystemDetail::ClampProperty(finalPosition, extraDelta);
    return finalPosition;
};

void CreateMagnetLinesForPivot(DAVA::Vector<MagnetLineInfo>& magnetLines, DAVA::Vector2 target, const DAVA::UIGeometricData& controlGeometricData)
{
    const Vector2 targetSize(controlGeometricData.size);
    Vector2 offset = targetSize * target;
    Vector2 horizontalLinePos(0.0f, offset.y);

    Vector2 verticalLinePos(offset.x, 0.0f);

    Rect horizontalRect(horizontalLinePos, Vector2(targetSize.x, 1.0f));
    Rect verticalRect(verticalLinePos, Vector2(1.0f, targetSize.y));
    Rect targetBox(Vector2(0.0f, 0.0f), targetSize);
    magnetLines.emplace_back(targetBox, horizontalRect, &controlGeometricData, Vector2::AXIS_X);
    magnetLines.emplace_back(targetBox, verticalRect, &controlGeometricData, Vector2::AXIS_Y);
}

template <typename Component>
UIControl* FindControlWithComponentInHierarchy(UIControl* control)
{
    while (control != nullptr && control->GetComponent<Component>() == nullptr)
    {
        control = control->GetParent();
    }
    DVASSERT(control != nullptr);
    return control;
}
}

EditorTransformSystem::EditorTransformSystem(DAVA::ContextAccessor* accessor)
    : BaseEditorSystem(accessor)
    , canvasDataAdapter(accessor)
{
    GetSystemsManager()->activeAreaChanged.Connect(this, &EditorTransformSystem::OnActiveAreaChanged);
}

EditorTransformSystem::~EditorTransformSystem() = default;

void EditorTransformSystem::OnActiveAreaChanged(const HUDAreaInfo& areaInfo)
{
    using namespace DAVA;
    activeArea = areaInfo.area;
    activeControlNode = areaInfo.owner;
    if (nullptr != activeControlNode)
    {
        UIControl* control = activeControlNode->GetControl();
        UIControl* parent = control->GetParent();
        parentGeometricData = parent->GetGeometricData();
        controlGeometricData = control->GetGeometricData();

        DVASSERT(parentGeometricData.scale.x > 0.0f && parentGeometricData.scale.y > 0.0f);
        DVASSERT(controlGeometricData.scale.x > 0.0f && controlGeometricData.scale.y > 0.0f);
        DVASSERT(controlGeometricData.size.x >= 0.0f && controlGeometricData.size.y >= 0.0f);

        RootProperty* rootProperty = activeControlNode->GetRootProperty();
        sizeProperty = rootProperty->FindPropertyByName("size");
        positionProperty = rootProperty->FindPropertyByName("position");
        angleProperty = rootProperty->FindPropertyByName("angle");
        pivotProperty = rootProperty->FindPropertyByName("pivot");
    }
    else
    {
        sizeProperty = nullptr;
        positionProperty = nullptr;
        angleProperty = nullptr;
        pivotProperty = nullptr;
    }
}

eDragState EditorTransformSystem::RequireNewState(DAVA::UIEvent* currentInput, eInputSource /*inputSource*/)
{
    using namespace DAVA;
    eDragState dragState = GetSystemsManager()->GetDragState();
    if (dragState == eDragState::Transform)
    {
        if (currentInput->device == eInputDevices::MOUSE
            && currentInput->phase == UIEvent::Phase::ENDED
            && currentInput->mouseButton == eMouseButtons::LEFT)
        {
            return eDragState::NoDrag;
        }
        else
        {
            return eDragState::Transform;
        }
    }

    HUDAreaInfo areaInfo = GetSystemsManager()->GetCurrentHUDArea();
    if (areaInfo.area != eArea::NO_AREA
        && currentInput->phase == UIEvent::Phase::DRAG
        && currentInput->mouseButton == eMouseButtons::LEFT)
    {
        //initialize start mouse position for correct rotation
        previousMousePos = currentInput->point;
        return eDragState::Transform;
    }
    return eDragState::NoDrag;
}

bool EditorTransformSystem::CanProcessInput(DAVA::UIEvent* currentInput, eInputSource /*inputSource*/) const
{
    using namespace DAVA;

    eDragState dragState = GetSystemsManager()->GetDragState();
    if (dragState == eDragState::Transform || currentInput->device == eInputDevices::KEYBOARD)
    {
        return true;
    }
    return false;
}

void EditorTransformSystem::ProcessInput(DAVA::UIEvent* currentInput, eInputSource /*inputSource*/)
{
    using namespace DAVA;
    switch (currentInput->phase)
    {
    case UIEvent::Phase::KEY_DOWN:
    case UIEvent::Phase::KEY_DOWN_REPEAT:
        ProcessKey(currentInput->key);
        break;

    case UIEvent::Phase::DRAG:
        if (currentInput->mouseButton == eMouseButtons::LEFT)
        {
            ProcessDrag(currentInput->point);
        }
        break;

    case UIEvent::Phase::ENDED:
        if (activeArea == eArea::ROTATE_AREA)
        {
            ClampAngle();
        }
        break;
    default:
        break;
    }
}

void EditorTransformSystem::OnDragStateChanged(eDragState dragState, eDragState previousState)
{
    bool isRootControl = activeControlNode != nullptr && activeControlNode->GetParent()->GetControl() == nullptr;

    DAVA::DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    DocumentData* documentData = activeContext->GetData<DocumentData>();
    DVASSERT(nullptr != documentData);

    if (dragState == eDragState::Transform)
    {
        extraDelta.SetZero();
        extraDeltaToMoveControls.clear();
        PrepareDrag();

        documentData->BeginBatch("transformations");
    }

    else if (previousState == eDragState::Transform)
    {
        documentData->EndBatch();
        canvasDataAdapter.TryCentralizeScene();
    }
}

eSystems EditorTransformSystem::GetOrder() const
{
    return eSystems::TRANSFORM;
}

void EditorTransformSystem::ProcessKey(DAVA::eInputElements key)
{
    using namespace DAVA;
    PrepareDrag();
    if (!selectedControlNodes.empty())
    {
        ControlTransformationSettings* settings = accessor->GetGlobalContext()->GetData<ControlTransformationSettings>();
        Vector2 step(settings->expandedmoveStepByKeyboard2);
        if (IsShiftPressed())
        {
            step = settings->moveStepByKeyboard2;
        }
        Vector2 deltaPos;
        switch (key)
        {
        case eInputElements::KB_LEFT:
            deltaPos.dx -= step.dx;
            break;
        case eInputElements::KB_UP:
            deltaPos.dy -= step.dy;
            break;
        case eInputElements::KB_RIGHT:
            deltaPos.dx += step.dx;
            break;
        case eInputElements::KB_DOWN:
            deltaPos.dy += step.dy;
            break;
        default:
            break;
        }
        if (!deltaPos.IsZero())
        {
            MoveAllSelectedControlsByKeyboard(deltaPos);
        }
    }
}

void EditorTransformSystem::PrepareDrag()
{
    using namespace DAVA;

    DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    DocumentData* data = activeContext->GetData<DocumentData>();
    selectedControlNodes = data->GetSelectedControls();

    nodesToMoveInfos.clear();
    for (ControlNode* selectedControl : selectedControlNodes)
    {
        nodesToMoveInfos.emplace_back(new MoveInfo(selectedControl, nullptr, nullptr));
    }
    CorrectNodesToMove();
    UpdateNeighbours();
}

void EditorTransformSystem::ProcessDrag(const DAVA::Vector2& pos)
{
    using namespace DAVA;

    Vector2 delta = GetSystemsManager()->GetMouseDelta();
    switch (activeArea)
    {
    case eArea::FRAME_AREA:
        MoveAllSelectedControlsByMouse(delta, CanMagnet());
        break;
    case eArea::TOP_LEFT_AREA:
    case eArea::TOP_CENTER_AREA:
    case eArea::TOP_RIGHT_AREA:
    case eArea::CENTER_LEFT_AREA:
    case eArea::CENTER_RIGHT_AREA:
    case eArea::BOTTOM_LEFT_AREA:
    case eArea::BOTTOM_CENTER_AREA:
    case eArea::BOTTOM_RIGHT_AREA:
    {
        bool withPivot = IsKeyPressed(eModifierKeys::ALT);
        bool rateably = IsKeyPressed(eModifierKeys::CONTROL);
        ResizeControl(delta, withPivot, rateably);
        break;
    }
    case eArea::PIVOT_POINT_AREA:
    {
        MovePivot(delta);
        break;
    }
    case eArea::ROTATE_AREA:
    {
        RotateControl(pos);
        break;
    }
    default:
        break;
    }
}

void EditorTransformSystem::MoveAllSelectedControlsByKeyboard(DAVA::Vector2 delta)
{
    using namespace DAVA;

    if (nodesToMoveInfos.empty())
    {
        return;
    }
    DVASSERT(delta.dx == 0.0f || delta.dy == 0.0f);
    Vector<EditorTransformSystemDetail::ChangePropertyAction> propertiesToChange;
    for (auto& nodeToMove : nodesToMoveInfos)
    {
        ControlNode* node = nodeToMove->node;
        const UIGeometricData* gd = nodeToMove->parentGD;
        float32 angle = gd->angle;
        AbstractProperty* property = nodeToMove->positionProperty;
        Vector2 originalPosition = property->GetValue().Cast<Vector2>();

        Vector2 deltaPosition = EditorTransformSystemDetail::RotateVectorForMove(delta, angle);
        Vector2 finalPosition(originalPosition + deltaPosition);
        propertiesToChange.emplace_back(node, property, Any(finalPosition));
    }
    DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    DocumentData* data = activeContext->GetData<DocumentData>();
    std::unique_ptr<ChangePropertyValueCommand> command = data->CreateCommand<ChangePropertyValueCommand>();
    DVASSERT(propertiesToChange.empty() == false);
    for (const EditorTransformSystemDetail::ChangePropertyAction& changePropertyAction : propertiesToChange)
    {
        command->AddNodePropertyValue(changePropertyAction.node, changePropertyAction.property, changePropertyAction.value);
    }
    data->ExecCommand(std::move(command));
}

void EditorTransformSystem::MoveAllSelectedControlsByMouse(DAVA::Vector2 mouseDelta, bool canAdjust)
{
    using namespace DAVA;

    Vector<EditorTransformSystemDetail::ChangePropertyAction> propertiesToChange;

    if (canAdjust)
    {
        //find hovered node alias in nodesToMoveInfos
        Set<PackageBaseNode*> activeControlNodeHierarchy;
        PackageBaseNode* parent = activeControlNode;
        while (parent != nullptr && parent->GetControl() != nullptr)
        {
            activeControlNodeHierarchy.insert(parent);
            parent = parent->GetParent();
        }
        auto iter = std::find_if(nodesToMoveInfos.begin(), nodesToMoveInfos.end(), [&activeControlNodeHierarchy](const std::unique_ptr<MoveInfo>& nodeInfoPtr)
                                 {
                                     PackageBaseNode* target = nodeInfoPtr->node;
                                     return activeControlNodeHierarchy.find(target) != activeControlNodeHierarchy.end();
                                 });

        //TODO: replace this if with an assert when currentArea will be synchronized with selection
        //right now they have difference with one frame
        if (iter == nodesToMoveInfos.end())
        {
            return;
        }

        const MoveInfo* nodeInfo = iter->get();

        auto deltaPositionBehavior = [this, nodeInfo](Vector2& deltaPosition) {
            ControlNode* node = nodeInfo->node;
            const UIGeometricData* gd = nodeInfo->parentGD;
            UIControl* control = node->GetControl();
            Vector<MagnetLineInfo> magnets;
            deltaPosition = AdjustMoveToNearestBorder(deltaPosition, magnets, gd, control);
            GetSystemsManager()->magnetLinesChanged.Emit(magnets);
        };
        const UIGeometricData* gd = nodeInfo->parentGD;

        AbstractProperty* positionProperty = nodeInfo->positionProperty;
        Vector2 originalPosition = positionProperty->GetValue().Cast<Vector2>();

        Vector2 finalPosition = EditorTransformSystemDetail::CreateFinalPosition(gd, originalPosition, mouseDelta, extraDelta, deltaPositionBehavior);

        ControlNode* node = nodeInfo->node;
        propertiesToChange.emplace_back(node, positionProperty, Any(finalPosition));

        //if control will clap it position or will magnet somewhere - it will jump from mouse cursor
        //and all other selected controls must jump too
        //in this case we get actual delta in control coordinates and transform it to global
        mouseDelta = Rotate((finalPosition - originalPosition), gd->angle);
        mouseDelta *= gd->scale;
    }
    for (auto& nodeToMove : nodesToMoveInfos)
    {
        ControlNode* node = nodeToMove->node;
        if (canAdjust && node == activeControlNode)
        {
            continue; //we already move it in this function
        }
        Vector2& activeExtraDelta = extraDeltaToMoveControls[node];
        AbstractProperty* positionProperty = nodeToMove->positionProperty;
        Vector2 originalPosition = positionProperty->GetValue().Cast<Vector2>();
        const UIGeometricData* gd = nodeToMove->parentGD;
        Vector2 finalPosition = EditorTransformSystemDetail::CreateFinalPosition(gd, originalPosition, mouseDelta, activeExtraDelta);

        propertiesToChange.emplace_back(node, positionProperty, Any(finalPosition));
    }

    DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    DocumentData* data = activeContext->GetData<DocumentData>();
    std::unique_ptr<ChangePropertyValueCommand> command = data->CreateCommand<ChangePropertyValueCommand>();
    DVASSERT(propertiesToChange.empty() == false);
    for (const EditorTransformSystemDetail::ChangePropertyAction& changePropertyAction : propertiesToChange)
    {
        command->AddNodePropertyValue(changePropertyAction.node, changePropertyAction.property, changePropertyAction.value);
    }
    data->ExecCommand(std::move(command));
}

DAVA::Vector<EditorTransformSystem::MagnetLine> EditorTransformSystem::CreateMagnetLines(const DAVA::Rect& box, const DAVA::UIGeometricData* parentGD,
                                                                                         DAVA::Vector2::eAxis axis, eTransformType type) const
{
    using namespace DAVA;

    DVASSERT(nullptr != parentGD);
    Vector<MagnetLine> magnets;

    CreateMagnetLinesToParent(box, parentGD, axis, magnets);
    CreateMagnetLinesToNeghbours(box, axis, magnets);
    CreateMagnetLinesToGuides(box, parentGD, axis, magnets);
    if (type == RESIZE)
    {
        CreateMagnetLinesToChildren(box, parentGD, axis, magnets);
    }
    return magnets;
}

void EditorTransformSystem::CreateMagnetLinesToParent(const DAVA::Rect& box, const DAVA::UIGeometricData* parentGD, DAVA::Vector2::eAxis axis, DAVA::Vector<MagnetLine>& lines) const
{
    using namespace DAVA;
    Rect parentBox(Vector2(), parentGD->size);
    if (activeControlNode->GetParent()->GetControl() != nullptr && parentBox.GetSize()[axis] > 0.0f)
    {
        //0.0f is equal to control left and 1.0f is equal to control right
        //first value is share of selected control and second value is share of parent control
        Vector<std::pair<float32, float32>> bordersToMagnet = {
            { 0.0f, 0.0f }, { 0.0f, 0.5f }, { 0.5f, 0.5f }, { 1.0f, 0.5f }, { 1.0f, 1.0f }
        };

        lines.reserve(lines.size() + bordersToMagnet.size());

        for (const auto& bordersPair : bordersToMagnet)
        {
            lines.emplace_back(bordersPair.first, box, bordersPair.second, parentBox, axis);
        }
    }
}

void EditorTransformSystem::CreateMagnetLinesToNeghbours(const DAVA::Rect& box, DAVA::Vector2::eAxis axis, DAVA::Vector<MagnetLine>& lines) const
{
    using namespace DAVA;
    //0.0f is equal to control left and 1.0f is equal to control right
    //first value is share of selected control and second value is share of neighbour
    Vector<std::pair<float32, float32>> bordersToMagnet = {
        { 0.0f, 0.0f }, { 0.0f, 0.5f }, { 0.5f, 0.5f }, { 1.0f, 0.5f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 0.0f }
    };

    lines.reserve(lines.size() + neighbours.size() * bordersToMagnet.size());

    for (const auto& neighbour : neighbours)
    {
        DVASSERT(nullptr != neighbour);
        Rect neighbourBox = neighbour->GetLocalGeometricData().GetAABBox();

        for (const auto& bordersPair : bordersToMagnet)
        {
            lines.emplace_back(bordersPair.first, box, bordersPair.second, neighbourBox, axis);
        }
    }
}

void EditorTransformSystem::CreateMagnetLinesToGuides(const DAVA::Rect& box, const DAVA::UIGeometricData* parentGD, DAVA::Vector2::eAxis axis, DAVA::Vector<MagnetLine>& lines) const
{
    using namespace DAVA;

    DataContext* globalContext = accessor->GetGlobalContext();
    PreferencesData* preferencesData = globalContext->GetData<PreferencesData>();

    if (preferencesData->IsGuidesEnabled() && parentGD->angle == 0.0f)
    {
        DataContext* activeContext = accessor->GetActiveContext();
        DVASSERT(activeContext != nullptr);
        const DocumentData* data = activeContext->GetData<DocumentData>();
        const SortedControlNodeSet& rootControls = data->GetDisplayedRootControls();
        DVASSERT(rootControls.size() == 1);
        PackageNode* package = data->GetPackageNode();
        PackageBaseNode* root = *rootControls.begin();
        PackageNode::AxisGuides values = package->GetAxisGuides(root->GetName(), axis);

        Vector<float32> bordersToMagnet;
        bool isRootControl = activeControlNode->GetParent()->GetControl() == nullptr;
        if (isRootControl)
        {
            bordersToMagnet = { 0.5f, 1.0f };
        }
        else
        {
            bordersToMagnet = { 0.0f, 0.5f, 1.0f };
        }

        lines.reserve(lines.size() + values.size() * bordersToMagnet.size());

        const UIGeometricData rootGD = root->GetControl()->GetGeometricData();
        for (float32 value : values)
        {
            //position in global coordinates, while pivotPoint and value in root control coordinates
            float32 valueInGlobalCoordinates = value * rootGD.scale[axis] + (rootGD.position[axis] - rootGD.pivotPoint[axis] * rootGD.scale[axis]);
            float32 valueInControlCoordinates = (valueInGlobalCoordinates - (parentGD->position[axis] - parentGD->pivotPoint[axis] * parentGD->scale[axis])) / parentGD->scale[axis];

            for (float32 borderToManget : bordersToMagnet)
            {
                lines.emplace_back(borderToManget, box, valueInControlCoordinates, axis);
            }
        }
    }
}

void EditorTransformSystem::CreateMagnetLinesToChildren(const DAVA::Rect& box, const DAVA::UIGeometricData* parentGD, DAVA::Vector2::eAxis axis, DAVA::Vector<MagnetLine>& lines) const
{
    using namespace DAVA;
    //0.0f is equal to control left and 1.0f is equal to control right
    //first value is share of selected control and second value is share of neighbour
    Vector<std::pair<float32, float32>> bordersToMagnet = {
        { 1.0f, 1.0f }
    };

    const auto& children = activeControlNode->GetControl()->GetChildren();
    lines.reserve(lines.size() + children.size() * bordersToMagnet.size());

    for (const auto& child : children)
    {
        Rect neighbourBox = child->GetLocalGeometricData().GetAABBox();
        neighbourBox.SetPosition(neighbourBox.GetPosition() + box.GetPosition());

        for (const auto& bordersPair : bordersToMagnet)
        {
            lines.emplace_back(bordersPair.first, box, bordersPair.second, neighbourBox, axis);
        }
    }
}

void EditorTransformSystem::ExtractMatchedLines(DAVA::Vector<MagnetLineInfo>& magnets, const DAVA::Vector<MagnetLine>& magnetLines, const DAVA::UIControl* control, DAVA::Vector2::eAxis axis)
{
    using namespace DAVA;

    UIControl* parent = control->GetParent();
    const UIGeometricData* parentGD = &parent->GetGeometricData();
    for (const MagnetLine& line : magnetLines)
    {
        if (fabs(line.interval) < EditorTransformSystemDetail::TRANSFORM_EPSILON)
        {
            const Vector2::eAxis oppositeAxis = axis == Vector2::AXIS_X ? Vector2::AXIS_Y : Vector2::AXIS_X;

            float32 controlTop = line.controlBox.GetPosition()[oppositeAxis];
            float32 controlBottom = controlTop + line.controlBox.GetSize()[oppositeAxis];

            float32 targetTop = line.targetBox.GetPosition()[oppositeAxis];
            float32 targetBottom = targetTop + line.targetBox.GetSize()[oppositeAxis];

            Vector2 linePos;
            linePos[axis] = line.targetPosition;
            linePos[oppositeAxis] = Min(controlTop, targetTop);
            Vector2 lineSize;
            lineSize[axis] = 1.0f;
            lineSize[oppositeAxis] = Max(controlBottom, targetBottom) - linePos[oppositeAxis];

            Rect lineRect(linePos, lineSize);
            magnets.emplace_back(line.targetBox, lineRect, parentGD, oppositeAxis);
        }
    }
}

DAVA::Vector2 EditorTransformSystem::AdjustMoveToNearestBorder(DAVA::Vector2 delta, DAVA::Vector<MagnetLineInfo>& magnets,
                                                               const DAVA::UIGeometricData* parentGD, const DAVA::UIControl* control)
{
    using namespace DAVA;
    const UIGeometricData controlGD = control->GetLocalGeometricData();
    Rect box = controlGD.GetAABBox();
    box.SetPosition(box.GetPosition() + delta);

    std::array<Vector<MagnetLine>, Vector2::AXIS_COUNT> magnetLines;

    ControlTransformationSettings* settings = accessor->GetGlobalContext()->GetData<ControlTransformationSettings>();

    for (int32 axisInt = Vector2::AXIS_X; axisInt < Vector2::AXIS_COUNT; ++axisInt)
    {
        Vector2::eAxis axis = static_cast<Vector2::eAxis>(axisInt);
        magnetLines[axis] = CreateMagnetLines(box, parentGD, axis, MOVE);

        //get nearest magnet line
        std::function<bool(const MagnetLine&, const MagnetLine&)> predicate = [](const MagnetLine& left, const MagnetLine& right) -> bool {
            return fabs(left.interval) < fabs(right.interval);
        };
        if (magnetLines[axis].empty())
        {
            continue;
        }

        MagnetLine nearestLine = *std::min_element(magnetLines[axis].begin(), magnetLines[axis].end(), predicate);
        float32 areaNearLineRight = nearestLine.targetPosition + settings->moveMagnetRange[axis];
        float32 areaNearLineLeft = nearestLine.targetPosition - settings->moveMagnetRange[axis];
        if (nearestLine.controlPosition >= areaNearLineLeft && nearestLine.controlPosition <= areaNearLineRight)
        {
            Vector2 oldDelta(delta);
            delta[axis] -= nearestLine.interval;
            extraDelta[axis] = oldDelta[axis] - delta[axis];
        }
    }

    for (int32 axisInt = Vector2::AXIS_X; axisInt < Vector2::AXIS_COUNT; ++axisInt)
    {
        Vector2::eAxis axis = static_cast<Vector2::eAxis>(axisInt);
        //adjust all lines to transformed state to get matched lines
        for (MagnetLine& line : magnetLines[axis])
        {
            line.interval -= extraDelta[axis];
            Vector2 boxPosition = line.controlBox.GetPosition();
            boxPosition -= extraDelta;
            line.controlBox.SetPosition(boxPosition);
        }
        ExtractMatchedLines(magnets, magnetLines[axis], control, axis);
    }
    return delta;
}

void EditorTransformSystem::ResizeControl(DAVA::Vector2 delta, bool withPivot, bool rateably)
{
    using namespace DAVA;
    UIControl* control = activeControlNode->GetControl();

    DVASSERT(activeArea != eArea::NO_AREA);

    const Directions& directions = cornersDirections.at(activeArea - eArea::TOP_LEFT_AREA);

    Vector2 pivot(control->GetPivot());

    Vector2 deltaMappedToControl(delta / controlGeometricData.scale);
    deltaMappedToControl = Rotate(deltaMappedToControl, -controlGeometricData.angle);

    Vector2 deltaSize(deltaMappedToControl);
    Vector2 deltaPosition(deltaMappedToControl);
    for (int32 axisInt = Vector2::AXIS_X; axisInt < Vector2::AXIS_COUNT; ++axisInt)
    {
        Vector2::eAxis axis = static_cast<Vector2::eAxis>(axisInt);
        const int direction = directions[axis];

        deltaSize[axis] *= direction;
        deltaPosition[axis] *= (direction == NEGATIVE_DIRECTION) ? 1.0f - pivot[axis] : pivot[axis];

        if (direction == NO_DIRECTION)
        {
            deltaPosition[axis] = 0.0f;
        }

        //modify if pivot modificator selected
        if (withPivot)
        {
            deltaPosition[axis] = 0.0f;

            auto pivotDelta = direction == NEGATIVE_DIRECTION ? pivot[axis] : 1.0f - pivot[axis];
            if (pivotDelta != 0.0f)
            {
                deltaSize[axis] /= pivotDelta;
            }
        }
    }
    //modify rateably
    const Vector2& size = control->GetSize();
    if (rateably && size.x > 0.0f && size.y > 0.0f)
    {
        //calculate proportion of control
        float proportion = size.y != 0.0f ? size.x / size.y : 0.0f;
        if (proportion != 0.0f)
        {
            Vector2 propDeltaSize(deltaSize.y * proportion, deltaSize.x / proportion);
            //get current drag direction
            Vector2::eAxis axis = fabs(deltaSize.y) > fabs(deltaSize.x) ? Vector2::AXIS_X : Vector2::AXIS_Y;

            deltaSize[axis] = propDeltaSize[axis];
            if (!withPivot)
            {
                deltaPosition[axis] = propDeltaSize[axis];
                if (directions[axis] == NO_DIRECTION)
                {
                    deltaPosition[axis] *= (0.5f - pivot[axis]) * -1.0f; //rainbow unicorn was here and add -1 to the right.
                }
                else
                {
                    deltaPosition[axis] *= (directions[axis] == NEGATIVE_DIRECTION ? 1.0f - pivot[axis] : pivot[axis]) * directions[axis];
                }
            }
        }
    }

    Vector2 transformPoint = withPivot ? pivot : Vector2();
    for (int32 axisInt = Vector2::AXIS_X; axisInt < Vector2::AXIS_COUNT; ++axisInt)
    {
        Vector2::eAxis axis = static_cast<Vector2::eAxis>(axisInt);
        if (directions[axis] == NEGATIVE_DIRECTION)
        {
            transformPoint[axis] = 1.0f - transformPoint[axis];
        }
    }
    Vector2 origDeltaSize(deltaSize);
    deltaSize += extraDelta; //transform to virtual coordinates
    extraDelta.SetZero();

    Vector2 adjustedSize = AdjustResizeToBorderAndToMinimum(deltaSize, transformPoint, directions);

    EditorTransformSystemDetail::ClampProperty(adjustedSize, extraDelta);

    //adjust delta position to new delta size
    for (int32 axisInt = Vector2::AXIS_X; axisInt < Vector2::AXIS_COUNT; ++axisInt)
    {
        Vector2::eAxis axis = static_cast<Vector2::eAxis>(axisInt);
        if (origDeltaSize[axis] != 0.0f)
        {
            deltaPosition[axis] *= origDeltaSize[axis] != 0.0f ? adjustedSize[axis] / origDeltaSize[axis] : 0.0f;
        }
    }

    deltaPosition *= control->GetScale();
    deltaPosition = Rotate(deltaPosition, control->GetAngle());

    Vector2 originalSize = sizeProperty->GetValue().Cast<Vector2>();
    Vector2 finalSize(originalSize + adjustedSize);
    Any sizeValue(finalSize);

    Vector2 originalPosition = positionProperty->GetValue().Cast<Vector2>();
    Vector2 finalPosition = originalPosition;

    bool isRootControl = activeControlNode->GetParent()->GetControl() == nullptr;
    if (isRootControl == false)
    {
        finalPosition += deltaPosition;
    }
    else
    {
        CanvasDataAdapter canvasDataAdapter(accessor);
        deltaPosition = Rotate(deltaPosition, -control->GetAngle());
        deltaPosition /= control->GetScale();
        deltaPosition -= adjustedSize * control->GetPivot();
        deltaPosition *= parentGeometricData.scale * control->GetScale();
        canvasDataAdapter.MoveScene(deltaPosition, true);

        UIControl* movableInEditorParent = EditorTransformSystemDetail::FindControlWithComponentInHierarchy<MovableInEditorComponent>(control);
        DVASSERT(movableInEditorParent != nullptr);
        movableInEditorParent->SetSize(finalSize);

        UIControl* counterPoiseParent = EditorTransformSystemDetail::FindControlWithComponentInHierarchy<CounterpoiseComponent>(control);
        DVASSERT(counterPoiseParent != nullptr);
        counterPoiseParent->SetPosition(counterPoiseParent->GetPosition() + adjustedSize * control->GetPivot());
    }

    Any positionValue(finalPosition);

    DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    DocumentData* data = activeContext->GetData<DocumentData>();
    std::unique_ptr<ResizeCommand> command = data->CreateCommand<ResizeCommand>();

    command->AddNodePropertyValue(activeControlNode,
                                  sizeProperty,
                                  sizeValue,
                                  positionProperty,
                                  positionValue);

    data->ExecCommand(std::move(command));
}

DAVA::Vector2 EditorTransformSystem::AdjustResizeToMinimumSize(DAVA::Vector2 deltaSize)
{
    using namespace DAVA;
    Vector2 scaledMinimum(GetMinimumSize() / controlGeometricData.scale);
    for (int32 axisInt = Vector2::AXIS_X; axisInt < Vector2::AXIS_COUNT; ++axisInt)
    {
        Vector2::eAxis axis = static_cast<Vector2::eAxis>(axisInt);
        if (scaledMinimum[axis] < 1.0f)
        {
            scaledMinimum[axis] = 1.0f;
        }
    }

    Vector2 origSize = sizeProperty->GetValue().Cast<Vector2>();

    Vector2 finalSize(origSize + deltaSize);
    Vector<MagnetLineInfo> magnets;

    for (int32 axisInt = Vector2::AXIS_X; axisInt < Vector2::AXIS_COUNT; ++axisInt)
    {
        Vector2::eAxis axis = static_cast<Vector2::eAxis>(axisInt);
        if (deltaSize[axis] > 0.0f)
        {
            continue;
        }
        if (origSize[axis] > scaledMinimum[axis])
        {
            if (finalSize[axis] > scaledMinimum[axis])
            {
                continue;
            }
            extraDelta[axis] += finalSize[axis] - scaledMinimum[axis];
            deltaSize[axis] = scaledMinimum[axis] - origSize[axis];
        }
        else
        {
            extraDelta[axis] += deltaSize[axis];
            deltaSize[axis] = 0.0f;
        }
    }

    return deltaSize;
}

DAVA::Vector2 EditorTransformSystem::AdjustResizeToBorderAndToMinimum(DAVA::Vector2 deltaSize, DAVA::Vector2 transformPoint, Directions directions)
{
    using namespace DAVA;
    Vector<MagnetLineInfo> magnets;

    Vector2 adjustedDeltaToBorder(deltaSize);
    bool canAdjustResize = CanMagnet() && activeControlNode->GetControl()->GetAngle() == 0.0f;
    if (canAdjustResize)
    {
        adjustedDeltaToBorder = AdjustResizeToBorder(deltaSize, transformPoint, directions, magnets);
    }
    Vector2 adjustedSize = AdjustResizeToMinimumSize(adjustedDeltaToBorder);
    if (adjustedDeltaToBorder != adjustedSize)
    {
        magnets.clear();
    }
    GetSystemsManager()->magnetLinesChanged.Emit(magnets);

    return adjustedSize;
}

DAVA::Vector2 EditorTransformSystem::AdjustResizeToBorder(DAVA::Vector2 deltaSize, DAVA::Vector2 transformPoint, Directions directions, DAVA::Vector<MagnetLineInfo>& magnets)
{
    using namespace DAVA;
    UIControl* control = activeControlNode->GetControl();

    UIGeometricData controlGD = control->GetLocalGeometricData();
    //calculate control box in parent
    controlGD.size += deltaSize;
    Rect box = controlGD.GetAABBox();
    Vector2 sizeAffect = Rotate(deltaSize * transformPoint * controlGD.scale, controlGD.angle);
    box.SetPosition(box.GetPosition() - sizeAffect);

    Vector2 transformPosition = box.GetPosition() + box.GetSize() * transformPoint;

    ControlTransformationSettings* settings = accessor->GetGlobalContext()->GetData<ControlTransformationSettings>();

    for (int32 axisInt = Vector2::AXIS_X; axisInt < Vector2::AXIS_COUNT; ++axisInt)
    {
        Vector2::eAxis axis = static_cast<Vector2::eAxis>(axisInt);
        if (directions[axis] != NO_DIRECTION)
        {
            bool isSceneMoved = axis == Vector2::AXIS_Y ?
            (activeArea == TOP_LEFT_AREA || activeArea == TOP_CENTER_AREA || activeArea == TOP_RIGHT_AREA) :
            (activeArea == BOTTOM_LEFT_AREA || activeArea == CENTER_LEFT_AREA || activeArea == TOP_LEFT_AREA);

            if (IsRootControl(activeControlNode) && isSceneMoved)
            {
                continue;
            }
            Vector<MagnetLine> magnetLines = CreateMagnetLines(box, &parentGeometricData, axis, RESIZE);
            if (magnetLines.empty())
            {
                continue;
            }

            std::function<bool(const MagnetLine&, const MagnetLine&)> predicate = [transformPoint, directions](const MagnetLine& left, const MagnetLine& right) -> bool {
                float32 shareLeft = left.controlSharePos - transformPoint[left.axis];
                float32 shareRight = right.controlSharePos - transformPoint[right.axis];
                float32 distanceLeft = shareLeft == 0.0f ? std::numeric_limits<float32>::max() : left.interval / shareLeft;
                float32 distanceRight = shareRight == 0.0f ? std::numeric_limits<float32>::max() : right.interval / shareRight;
                return fabs(distanceLeft) < fabs(distanceRight);
            };

            MagnetLine nearestLine = *std::min_element(magnetLines.begin(), magnetLines.end(), predicate);
            float32 share = fabs(nearestLine.controlSharePos - transformPoint[nearestLine.axis]);
            float32 rangeForPosition = settings->resizeMagnetRange[axis] * share;
            float32 areaNearLineRight = nearestLine.targetPosition + rangeForPosition;
            float32 areaNearLineLeft = nearestLine.targetPosition - rangeForPosition;

            Vector2 oldDeltaSize(deltaSize);

            if (nearestLine.controlPosition >= areaNearLineLeft && nearestLine.controlPosition <= areaNearLineRight)
            {
                float32 interval = nearestLine.interval * directions[axis] * -1;
                DVASSERT(share > 0.0f);
                interval /= share;
                float32 scaledDistance = interval / controlGD.scale[axis];
                deltaSize[axis] += scaledDistance;
                extraDelta[axis] += oldDeltaSize[axis] - deltaSize[axis];
            }

            for (MagnetLine& line : magnetLines)
            {
                float32 lineShare = fabs(line.controlSharePos - transformPoint[line.axis]);
                line.interval -= extraDelta[line.axis] * controlGD.scale[line.axis] * lineShare * directions[line.axis];
            }
            ExtractMatchedLines(magnets, magnetLines, control, axis);
        }
    }
    return deltaSize;
}

void EditorTransformSystem::MovePivot(DAVA::Vector2 delta)
{
    using namespace DAVA;
    Vector<EditorTransformSystemDetail::ChangePropertyAction> propertiesToChange;
    Vector2 pivot = AdjustPivotToNearestArea(delta);

    Vector2 scaledDelta(delta / parentGeometricData.scale);
    Vector2 rotatedDeltaPosition(Rotate(scaledDelta, -parentGeometricData.angle));
    Vector2 originalPos(positionProperty->GetValue().Cast<Vector2>());
    Vector2 finalPosition(originalPos + rotatedDeltaPosition);

    DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    DocumentData* data = activeContext->GetData<DocumentData>();
    std::unique_ptr<ChangePivotCommand> command = data->CreateCommand<ChangePivotCommand>();
    command->AddNodePropertyValue(activeControlNode,
                                  pivotProperty,
                                  Any(pivot),
                                  positionProperty,
                                  Any(finalPosition));
    data->ExecCommand(std::move(command));
}

DAVA::Vector2 EditorTransformSystem::AdjustPivotToNearestArea(DAVA::Vector2& delta)
{
    using namespace DAVA;
    Vector<MagnetLineInfo> magnetLines;

    const Rect ur(controlGeometricData.GetUnrotatedRect());
    const Vector2 controlSize(ur.GetSize());
    DVASSERT(controlSize.x > 0.0f && controlSize.y > 0.0f);
    const Vector2 rotatedDeltaPivot(Rotate(delta, -controlGeometricData.angle));
    Vector2 deltaPivot(rotatedDeltaPivot / controlSize);

    ControlTransformationSettings* settings = accessor->GetGlobalContext()->GetData<ControlTransformationSettings>();
    const Vector2 range(settings->pivotMagnetRange / controlSize); //range in pivot coordinates

    Vector2 origPivot = pivotProperty->GetValue().Cast<Vector2>();
    Vector2 finalPivot(origPivot + deltaPivot + extraDelta);

    bool found = false;
    if (IsShiftPressed() && settings->shareOfSizeToMagnetPivot.x > 0.0f && settings->shareOfSizeToMagnetPivot.y > 0.0f)
    {
        const float32 maxPivot = 1.0f;

        Vector2 target;
        Vector2 distanceToTarget;
        Vector2 shareOfSizeToMagnetPivot_;
        for (float32 targetX = 0.0f; targetX <= maxPivot; targetX += settings->shareOfSizeToMagnetPivot.x)
        {
            for (float32 targetY = 0.0f; targetY <= maxPivot; targetY += settings->shareOfSizeToMagnetPivot.y)
            {
                float32 left = targetX - range.dx;
                float32 right = targetX + range.dx;
                float32 top = targetY - range.dy;
                float32 bottom = targetY + range.dy;
                if (finalPivot.dx >= left && finalPivot.dx <= right && finalPivot.dy >= top && finalPivot.dy <= bottom)
                {
                    Vector2 currentDistance(fabs(finalPivot.dx - targetX), fabs(finalPivot.dy - targetY));
                    if (currentDistance.IsZero() || !found || currentDistance.x < distanceToTarget.x || currentDistance.y < distanceToTarget.y)
                    {
                        distanceToTarget = currentDistance;
                        target = Vector2(targetX, targetY);
                    }
                    found = true;
                }
            }
        }
        if (found)
        {
            EditorTransformSystemDetail::CreateMagnetLinesForPivot(magnetLines, target, controlGeometricData);
            extraDelta = finalPivot - target;
            delta = Rotate((target - origPivot) * controlSize, controlGeometricData.angle);

            finalPivot = target;
        }
    }

    if (!found)
    {
        if (!extraDelta.IsZero())
        {
            deltaPivot += extraDelta;
            extraDelta.SetZero();
            delta = Rotate(deltaPivot * controlSize, controlGeometricData.angle);
        }
    }
    GetSystemsManager()->magnetLinesChanged.Emit(magnetLines);
    return finalPivot;
}

bool EditorTransformSystem::RotateControl(const DAVA::Vector2& pos)
{
    using namespace DAVA;
    Vector2 rotatePoint(controlGeometricData.GetUnrotatedRect().GetPosition());
    rotatePoint += controlGeometricData.pivotPoint * controlGeometricData.scale;
    Vector2 l1(previousMousePos - rotatePoint);
    Vector2 l2(pos - rotatePoint);

    if (l2.Length() < 15.0f)
    {
        return false;
    }

    float32 angleRad = atan2(l1.x * l2.y - l2.x * l1.y, l1.x * l2.x + l1.y * l2.y);
    float32 deltaAngle = RadToDeg(angleRad);
    //after modification deltaAngle is less than mouse delta positions

    deltaAngle += extraDelta.dx;
    extraDelta.SetZero();
    float32 originalAngle = angleProperty->GetValue().Cast<float32>();

    float32 finalAngle = AdjustRotateToFixedAngle(deltaAngle, originalAngle);

    DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    DocumentData* data = activeContext->GetData<DocumentData>();
    std::unique_ptr<ChangePropertyValueCommand> command = data->CreateCommand<ChangePropertyValueCommand>();
    command->AddNodePropertyValue(activeControlNode, angleProperty, Any(finalAngle));
    data->ExecCommand(std::move(command));

    previousMousePos = pos;
    return true;
}

DAVA::float32 EditorTransformSystem::AdjustRotateToFixedAngle(DAVA::float32 deltaAngle, DAVA::float32 originalAngle)
{
    using namespace DAVA;
    float32 finalAngle = originalAngle + deltaAngle;
    if (IsShiftPressed())
    {
        ControlTransformationSettings* settings = accessor->GetGlobalContext()->GetData<ControlTransformationSettings>();
        const int step = settings->angleSegment; //fixed angle step
        int32 nearestTargetAngle = static_cast<int32>(finalAngle - static_cast<int32>(finalAngle) % step);
        if ((finalAngle >= 0) ^ (deltaAngle > 0))
        {
            nearestTargetAngle += step * (finalAngle >= 0.0f ? 1 : -1);
        }
        //disable rotate backwards if we move cursor forward
        if ((deltaAngle >= 0.0f && nearestTargetAngle <= originalAngle + EditorTransformSystemDetail::TRANSFORM_EPSILON)
            || (deltaAngle < 0.0f && nearestTargetAngle >= originalAngle - EditorTransformSystemDetail::TRANSFORM_EPSILON))
        {
            extraDelta.dx = deltaAngle;
            return originalAngle;
        }
        extraDelta.dx = finalAngle - nearestTargetAngle;
        return nearestTargetAngle;
    }
    else
    {
        EditorTransformSystemDetail::ClampProperty(finalAngle, extraDelta.dx);
    }
    return finalAngle;
}

void EditorTransformSystem::CorrectNodesToMove()
{
    using namespace DAVA;
    nodesToMoveInfos.remove_if([](std::unique_ptr<MoveInfo>& item) {
        const PackageBaseNode* parent = item->node->GetParent();
        return nullptr == parent || nullptr == parent->GetControl();
    });

    auto iter = nodesToMoveInfos.begin();
    while (iter != nodesToMoveInfos.end())
    {
        bool toRemove = false;
        auto iter2 = nodesToMoveInfos.begin();
        while (iter2 != nodesToMoveInfos.end() && !toRemove)
        {
            PackageBaseNode* node = (*iter)->node;
            if (iter != iter2)
            {
                while (nullptr != node->GetParent() && nullptr != node->GetControl() && !toRemove)
                {
                    if (node == (*iter2)->node)
                    {
                        toRemove = true;
                    }
                    node = node->GetParent();
                }
            }
            ++iter2;
        }
        if (toRemove)
        {
            nodesToMoveInfos.erase(iter++);
        }
        else
        {
            ++iter;
        }
    }
    for (auto& moveInfo : nodesToMoveInfos)
    {
        ControlNode* node = moveInfo->node;
        UIControl* control = node->GetControl();
        moveInfo->positionProperty = node->GetRootProperty()->FindPropertyByName("position");
        UIControl* parent = control->GetParent();
        DVASSERT(nullptr != parent);
        moveInfo->parentGD = &parent->GetGeometricData();
    }
}

void EditorTransformSystem::UpdateNeighbours()
{
    using namespace DAVA;

    neighbours.clear();
    if (nullptr != activeControlNode)
    {
        DAVA::UIControl* parent = activeControlNode->GetControl()->GetParent();
        if (nullptr != parent)
        {
            Set<UIControl*> ignoredNeighbours;
            for (ControlNode* node : selectedControlNodes)
            {
                if (node->GetControl()->GetParent() == parent)
                {
                    ignoredNeighbours.insert(node->GetControl());
                }
            }

            const auto& children = parent->GetChildren();
            Set<UIControl*> sortedChildren;
            for (const auto& child : children)
            {
                sortedChildren.insert(child.Get());
            }
            std::set_difference(sortedChildren.begin(), sortedChildren.end(), ignoredNeighbours.begin(), ignoredNeighbours.end(), std::back_inserter(neighbours));
        }
    }
}

void EditorTransformSystem::ClampAngle()
{
    using namespace DAVA;
    float32 angle = angleProperty->GetValue().Cast<float32>();
    if (fabs(angle) > 360)
    {
        angle += angle > 0.0f ? EditorTransformSystemDetail::TRANSFORM_EPSILON : -EditorTransformSystemDetail::TRANSFORM_EPSILON;
        angle = static_cast<int32>(angle) % 360;
    }

    DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    DocumentData* data = activeContext->GetData<DocumentData>();
    std::unique_ptr<ChangePropertyValueCommand> command = data->CreateCommand<ChangePropertyValueCommand>();
    command->AddNodePropertyValue(activeControlNode, angleProperty, Any(angle));
    data->ExecCommand(std::move(command));
}

bool EditorTransformSystem::IsShiftPressed() const
{
    return DAVA::IsKeyPressed(DAVA::eModifierKeys::SHIFT) ^ (accessor->GetGlobalContext()->GetData<ControlTransformationSettings>()->shiftInverted);
}

bool EditorTransformSystem::CanMagnet() const
{
    using namespace DAVA;
    float32 scaleToMagnet = 8.0f;
    return accessor->GetGlobalContext()->GetData<ControlTransformationSettings>()->canMagnet && canvasDataAdapter.GetScale() <= scaleToMagnet;
}
