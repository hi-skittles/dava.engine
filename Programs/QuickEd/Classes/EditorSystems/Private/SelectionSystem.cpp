#include "EditorSystems/SelectionSystem.h"
#include "EditorSystems/EditorSystemsManager.h"
#include "Model/PackageHierarchy/PackageControlsNode.h"
#include "Model/ControlProperties/RootProperty.h"
#include "Model/ControlProperties/VisibleValueProperty.h"
#include "Model/PackageHierarchy/ControlNode.h"

#include "Modules/DocumentsModule/DocumentData.h"

#include <TArc/Core/ContextAccessor.h>
#include <TArc/Utils/Utils.h>

#include <Reflection/ReflectedTypeDB.h>
#include <Reflection/ReflectionRegistrator.h>
#include <UI/UIEvent.h>
#include <UI/UIControl.h>
#include <Input/InputSystem.h>

DAVA_VIRTUAL_REFLECTION_IMPL(SelectionSettings)
{
    DAVA::ReflectionRegistrator<SelectionSettings>::Begin()[DAVA::M::DisplayName("Control Selection"), DAVA::M::SettingsSortKey(100)]
    .ConstructorByPointer()
    .Field("canFindCommonForSelection", &SelectionSettings::canFindCommonForSelection)[DAVA::M::DisplayName("Can search most common node")]
    .End();
}

SelectionSystem::SelectionSystem(DAVA::ContextAccessor* accessor)
    : BaseEditorSystem(accessor)
{
    documentDataWrapper = accessor->CreateWrapper(DAVA::ReflectedTypeDB::Get<DocumentData>());
}

eSystems SelectionSystem::GetOrder() const
{
    return eSystems::SELECTION;
}

void SelectionSystem::ProcessInput(DAVA::UIEvent* currentInput, eInputSource /*eInputSource*/)
{
    ControlNode* selectedNode = GetSystemsManager()->GetControlNodeAtPoint(currentInput->point, currentInput->tapCount > 1);
    if (nullptr != selectedNode)
    {
        SelectNode(selectedNode);
    }
}

void SelectionSystem::ClearSelection()
{
    SelectNodesImpl(SelectedNodes());
}

void SelectionSystem::SelectAllControls()
{
    using namespace DAVA;

    DataContext* activeContext = accessor->GetActiveContext();
    if (activeContext == nullptr)
    {
        return;
    }

    DocumentData* documentData = activeContext->GetData<DocumentData>();

    SelectedNodes selected;
    //find only children of root controls
    for (const PackageBaseNode* node : documentData->GetDisplayedRootControls())
    {
        for (int i = 0, count = node->GetCount(); i < count; ++i)
        {
            selected.insert(node->Get(i));
        }
    }
    if (selected.empty() == false)
    {
        SelectNodesImpl(selected);
    }
}

void SelectionSystem::FocusNextChild()
{
    FocusToChild(true);
}

void SelectionSystem::FocusPreviousChild()
{
    FocusToChild(false);
}

void SelectionSystem::FocusToChild(bool next)
{
    if (accessor->GetActiveContext() == nullptr)
    {
        return;
    }

    PackageBaseNode* startNode = nullptr;
    SelectedNodes selection = documentDataWrapper.GetFieldValue(DocumentData::selectionPropertyName).Cast<SelectedNodes>(SelectedNodes());

    if (!selection.empty())
    {
        startNode = *selection.rbegin();
    }
    PackageBaseNode* nextNode = nullptr;
    DAVA::Vector<PackageBaseNode*> allNodes;
    GetSystemsManager()->CollectControlNodes(std::back_inserter(allNodes), [](const ControlNode*) { return true; });
    if (allNodes.empty())
    {
        return;
    }
    auto findIt = std::find(allNodes.begin(), allNodes.end(), startNode);
    if (findIt == allNodes.end())
    {
        nextNode = next ? allNodes.front() : allNodes.back();
    }
    else if (next)
    {
        ++findIt;
        nextNode = findIt == allNodes.end() ? allNodes.front() : *findIt;
    }
    else
    {
        nextNode = findIt == allNodes.begin() ? allNodes.back() : *(--findIt);
    }

    SelectNodesImpl({ nextNode });
}

void SelectionSystem::SelectNodesImpl(const SelectedNodes& selection)
{
    //TODO: remove this "if" when other systems will not emit signal selectionChanged
    if (documentDataWrapper.HasData())
    {
        documentDataWrapper.SetFieldValue(DocumentData::selectionPropertyName, selection);
    }
}

void SelectionSystem::SelectNode(ControlNode* selectedNode)
{
    SelectedNodes newSelection;
    SelectedNodes currentSelection = documentDataWrapper.GetFieldValue(DocumentData::selectionPropertyName).Cast<SelectedNodes>(SelectedNodes());
    if (DAVA::IsKeyPressed(DAVA::eModifierKeys::SHIFT) ||
        (GetSystemsManager()->GetDisplayState() == eDisplayState::Edit && currentSelection.find(selectedNode) != currentSelection.end()))
    {
        newSelection = currentSelection;
    }

    if (selectedNode != nullptr)
    {
        if (DAVA::IsKeyPressed(DAVA::eModifierKeys::SHIFT) && currentSelection.size() > 1 && currentSelection.find(selectedNode) != currentSelection.end())
        {
            newSelection.erase(selectedNode);
        }
        else
        {
            newSelection.insert(selectedNode);
        }
    }

    SelectNodesImpl(newSelection);
}

ControlNode* SelectionSystem::FindSmallNodeUnderNode(const DAVA::Vector<ControlNode*>& nodesUnderPoint) const
{
    if (!accessor->GetGlobalContext()->GetData<SelectionSettings>()->canFindCommonForSelection || nodesUnderPoint.empty())
    {
        return nullptr;
    }
    //if control much smaller than parent - we can want to select it
    DAVA::Vector<std::pair<ControlNode*, DAVA::Vector2>> sizes;
    sizes.reserve(nodesUnderPoint.size());
    PackageBaseNode* node = nodesUnderPoint.back();
    DAVA::Set<PackageBaseNode*> topLevelItemHierarchy;
    ControlNode* parentNode = dynamic_cast<ControlNode*>(node->GetParent());
    if (parentNode == nullptr)
    {
        return nullptr;
    }
    //we can place controls under each other
    for (ControlNode* child : *parentNode)
    {
        topLevelItemHierarchy.insert(child);
    }

    //get hierarchy to ensure that node under cursor is a top visible node
    do
    {
        topLevelItemHierarchy.insert(node);
        node = node->GetParent();
    } while (node != nullptr && node->GetControl() != nullptr);

    for (auto iter = nodesUnderPoint.rbegin(); iter != nodesUnderPoint.rend(); ++iter)
    {
        ControlNode* node = *iter;
        DAVA::Vector2 size = node->GetControl()->GetAbsoluteRect().GetSize();
        if (!sizes.empty())
        {
            const std::pair<ControlNode*, DAVA::Vector2>& lastNodeSize = sizes.back();
            DAVA::Vector2 previousSize = lastNodeSize.second;

            ControlNode* previousNode = lastNodeSize.first;
            //not top level node or it hierarchy. We don't search in background nodes
            if (topLevelItemHierarchy.find(previousNode) == topLevelItemHierarchy.end())
            {
                break;
            }

            //some size issues. They can migrate to the preferences system later
            const DAVA::Vector2 acceptableSizeProportion(3.0f, 3.0f);
            const DAVA::Vector2 acceptableRelativeSizeDifference(30.0f, 30.0f);
            const DAVA::Vector2 acceptableAbsoluteSizeDifference(200.0f, 200.0f);
            for (DAVA::int32 axisInt = DAVA::Vector2::AXIS_X; axisInt < DAVA::Vector2::AXIS_COUNT; ++axisInt)
            {
                DAVA::Vector2::eAxis axis = static_cast<DAVA::Vector2::eAxis>(axisInt);
                if ((size[axis] / previousSize[axis] > acceptableSizeProportion[axis]
                     && size[axis] - previousSize[axis] > acceptableRelativeSizeDifference[axis])
                    || size[axis] - previousSize[axis] > acceptableAbsoluteSizeDifference[axis])
                {
                    return previousNode;
                }
            }
        }
        //someone still can create control with a negative size
        if (size.dx > 0.0f && size.dy > 0.0f)
        {
            sizes.push_back(std::make_pair(node, size));
        }
    }
    return nullptr;
}

void SelectionSystem::GetNodesForSelection(DAVA::Vector<ControlNode*>& nodesUnderPoint, const DAVA::Vector2& point) const
{
    auto findPredicate = [point](const ControlNode* node) -> bool {
        const DAVA::UIControl* control = node->GetControl();
        DVASSERT(nullptr != control);
        return control->GetVisibilityFlag() && !control->IsHiddenForDebug() && control->IsPointInside(point);
    };

    auto stopPredicate = [](const ControlNode* node) -> bool {
        const DAVA::UIControl* control = node->GetControl();
        DVASSERT(nullptr != control);
        return !control->GetVisibilityFlag() || control->IsHiddenForDebug();
    };
    GetSystemsManager()->CollectControlNodes(std::back_inserter(nodesUnderPoint), findPredicate, stopPredicate);
}

bool SelectionSystem::CanProcessInput(DAVA::UIEvent* currentInput, eInputSource /*inputSource*/) const
{
    using namespace DAVA;

    eDisplayState displayState = GetSystemsManager()->GetDisplayState();
    eDragState dragState = GetSystemsManager()->GetDragState();
    return (displayState == eDisplayState::Edit
            || displayState == eDisplayState::Preview)
    && dragState == eDragState::NoDrag
    && currentInput->device == eInputDevices::MOUSE
    && currentInput->mouseButton == eMouseButtons::LEFT
    && currentInput->phase == UIEvent::Phase::BEGAN;
}

ControlNode* SelectionSystem::GetCommonNodeUnderPoint(const DAVA::Vector2& point, bool canGoDeeper) const
{
    HUDAreaInfo currentArea = GetSystemsManager()->GetCurrentHUDArea();
    if (canGoDeeper == false && currentArea.area != eArea::NO_AREA)
    {
        return currentArea.owner;
    }

    DAVA::Vector<ControlNode*> nodesUnderPoint;
    GetNodesForSelection(nodesUnderPoint, point);
    SelectedNodes selection = documentDataWrapper.GetFieldValue(DocumentData::selectionPropertyName).Cast<SelectedNodes>(SelectedNodes());

    //here we use only selected controls
    for (auto iter = selection.begin(); iter != selection.end();)
    {
        if ((*iter)->GetControl() == nullptr)
        {
            iter = selection.erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    //no selection. Search for the child of root under cursor
    if (nodesUnderPoint.empty())
    {
        return nullptr;
    }
    else if (nodesUnderPoint.size() == 1)
    {
        return nodesUnderPoint.front();
    }

    if (!selection.empty())
    {
        //collect all selected hierarchy
        SelectedNodes parentsOfSelectedNodes;
        for (PackageBaseNode* node : selection)
        {
            node = node->GetParent();
            while (node != nullptr && node->GetControl() != nullptr)
            {
                parentsOfSelectedNodes.insert(node);
                node = node->GetParent();
            }
        }

        //move from deep nodes to root node
        for (auto iter = nodesUnderPoint.rbegin(); iter != nodesUnderPoint.rend(); ++iter)
        {
            ControlNode* node = *iter;
            PackageBaseNode* nodeParent = node->GetParent();

            if (selection.find(node) != selection.end())
            {
                return node;
            }

            //search child of selected to move down by hierarchy
            // or search neighbor to move left-right
            if (selection.find(nodeParent) != selection.end())
            {
                if (canGoDeeper)
                {
                    return node;
                }
                else
                {
                    DVASSERT(dynamic_cast<ControlNode*>(nodeParent) != nullptr);
                    return dynamic_cast<ControlNode*>(nodeParent);
                }
            }
            else if (selection.find(node) == selection.end()
                     && parentsOfSelectedNodes.find(nodeParent) != parentsOfSelectedNodes.end())
            {
                return node;
            }
        }
        //may be there some small node inside big one
        ControlNode* node = FindSmallNodeUnderNode(nodesUnderPoint);
        if (node != nullptr)
        {
            return node;
        }
    }
    //try to find child of root control
    else
    {
        ControlNode* node = FindSmallNodeUnderNode(nodesUnderPoint);
        if (node != nullptr)
        {
            return node;
        }
        else
        {
            for (auto iter = nodesUnderPoint.rbegin(); iter != nodesUnderPoint.rend(); ++iter)
            {
                ControlNode* node = *iter;
                PackageBaseNode* parent = node->GetParent();
                if (parent != nullptr)
                {
                    parent = parent->GetParent();
                }
                if (parent != nullptr && parent->GetControl() == nullptr)
                {
                    return node;
                }
            }
        }
    }
    return nullptr;
}

ControlNode* SelectionSystem::GetNearestNodeUnderPoint(const DAVA::Vector2& point) const
{
    HUDAreaInfo currentArea = GetSystemsManager()->GetCurrentHUDArea();
    if (currentArea.area == eArea::NO_AREA || currentArea.area == eArea::FRAME_AREA)
    {
        DAVA::Vector<ControlNode*> nodesUnderPoint;
        GetNodesForSelection(nodesUnderPoint, point);
        return nodesUnderPoint.empty() ? nullptr : nodesUnderPoint.back();
    }
    else
    {
        return currentArea.owner;
    }
}
