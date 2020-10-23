#pragma once

#include "Classes/EditorSystems/EditorSystemsConstants.h"

#include "Classes/Model/PackageHierarchy/PackageBaseNode.h"
#include "Classes/Model/PackageHierarchy/ControlNode.h"

#include "Classes/Interfaces/EditorSystemsManagerInteface.h"

#include <TArc/DataProcessing/DataWrapper.h>
#include <TArc/DataProcessing/DataListener.h>

#include <Base/BaseTypes.h>
#include <Base/RefPtr.h>
#include <Functional/Signal.h>
#include <UI/UIControl.h>

#include <Math/Rect.h>
#include <Math/Vector.h>

namespace DAVA
{
class UIControl;
class UIEvent;
class UIGeometricData;
class Any;
class ContextAccessor;
class FieldBinder;
}

struct HUDAreaInfo
{
    HUDAreaInfo(ControlNode* owner_ = nullptr, eArea area_ = eArea::NO_AREA)
        : owner(owner_)
        , area(area_)
    {
        DVASSERT((owner != nullptr && area != eArea::NO_AREA) || (owner == nullptr && area == eArea::NO_AREA));
    }
    ControlNode* owner = nullptr;
    eArea area = NO_AREA;
};

struct MagnetLineInfo
{
    MagnetLineInfo(const DAVA::Rect& targetRect_, const DAVA::Rect& rect_, const DAVA::UIGeometricData* gd_, DAVA::Vector2::eAxis axis_)
        : targetRect(targetRect_)
        , rect(rect_)
        , gd(gd_)
        , axis(axis_)
    {
    }
    DAVA::Rect targetRect;
    DAVA::Rect rect;
    const DAVA::UIGeometricData* gd;
    const DAVA::Vector2::eAxis axis;
};

class BaseEditorSystem;
class AbstractProperty;
class PackageNode;
class EditorControlsView;
class SelectionSystem;
class HUDSystem;

class EditorSystemsManager : public Interfaces::EditorSystemsManagerInterface
{
    using StopPredicate = std::function<bool(const ControlNode*)>;
    static StopPredicate defaultStopPredicate;

public:
    explicit EditorSystemsManager(DAVA::ContextAccessor* accessor);
    ~EditorSystemsManager();

    eDragState GetDragState() const;
    eDisplayState GetDisplayState() const;
    HUDAreaInfo GetCurrentHUDArea() const;

    //TODO: remove this function by moving systems to the separate modules
    DAVA_DEPRECATED(void InitSystems());

    void OnInput(DAVA::UIEvent* currentInput, eInputSource inputSource);

    template <class OutIt, class Predicate>
    void CollectControlNodes(OutIt destination, Predicate predicate, StopPredicate stopPredicate = defaultStopPredicate) const;

    ControlNode* GetControlNodeAtPoint(const DAVA::Vector2& point, bool canGoDeeper = false) const;
    DAVA::uint32 GetIndexOfNearestRootControl(const DAVA::Vector2& point) const;

    void UpdateDisplayState();

    void SelectAll();
    void FocusNextChild();
    void FocusPreviousChild();
    void ClearSelection();
    void SelectNode(ControlNode* node);
    void SetActiveHUDArea(const HUDAreaInfo& areaInfo);

    void Invalidate(ControlNode* removedNode);
    void Invalidate();

    DAVA::Signal<const HUDAreaInfo& /*areaInfo*/> activeAreaChanged;
    DAVA::Signal<const DAVA::Vector<MagnetLineInfo>& /*magnetLines*/> magnetLinesChanged;
    DAVA::Signal<ControlNode*, AbstractProperty*, const DAVA::Any&> propertyChanged;
    DAVA::Signal<bool> emulationModeChanged;

    //helpers
    DAVA::Vector2 GetMouseDelta() const;
    DAVA::Vector2 GetLastMousePos() const;

private:
    void InitFieldBinder();
    void SetDragState(eDragState dragState);
    void SetDisplayState(eDisplayState displayState);

    void OnEmulationModeChanged(const DAVA::Any& emulationMode);
    void OnRootContolsChanged(const DAVA::Any& rootControls);

    void OnUpdate();

    template <class OutIt, class Predicate>
    void CollectControlNodesImpl(OutIt destination, Predicate predicate, StopPredicate stopPredicate, ControlNode* node) const;

    void InitDAVAScreen();

    void OnDragStateChanged(eDragState currentState, eDragState previousState);

    const SortedControlNodeSet& GetDisplayedRootControls() const;

    //EditorSystemsManagerInteface
    void RegisterEditorSystem(BaseEditorSystem* editorSystem) override;
    void UnregisterEditorSystem(BaseEditorSystem* editorSystem) override;

    DAVA::RefPtr<DAVA::UIControl> rootControl;

    DAVA::Map<eSystems, BaseEditorSystem*> systems;
    DAVA::Map<BaseEditorSystem*, DAVA::Vector<DAVA::RefPtr<DAVA::UIControl>>> systemsControls;

    SelectionSystem* selectionSystemPtr = nullptr; // weak pointer to selection system
    HUDSystem* hudSystemPtr = nullptr;

    eDragState dragState = eDragState::NoDrag;
    eDragState previousDragState = eDragState::NoDrag;
    eDisplayState displayState = eDisplayState::Preview;
    eDisplayState previousDisplayState = eDisplayState::Preview;

    HUDAreaInfo currentHUDArea;
    //helpers
    DAVA::Vector2 lastMousePos;
    DAVA::Vector2 mouseDelta;

    DAVA::ContextAccessor* accessor = nullptr;
    std::unique_ptr<DAVA::FieldBinder> fieldBinder;
};

template <class OutIt, class Predicate>
void EditorSystemsManager::CollectControlNodes(OutIt destination, Predicate predicate, StopPredicate stopPredicate) const
{
    for (PackageBaseNode* rootControl : GetDisplayedRootControls())
    {
        ControlNode* controlNode = dynamic_cast<ControlNode*>(rootControl);
        DVASSERT(nullptr != controlNode);
        CollectControlNodesImpl(destination, predicate, stopPredicate, controlNode);
    }
}

template <class OutIt, class Predicate>
void EditorSystemsManager::CollectControlNodesImpl(OutIt destination, Predicate predicate, StopPredicate stopPredicate, ControlNode* node) const
{
    if (predicate(node))
    {
        *destination++ = node;
    }
    if (!stopPredicate(node))
    {
        int count = node->GetCount();
        for (int i = 0; i < count; ++i)
        {
            CollectControlNodesImpl(destination, predicate, stopPredicate, node->Get(i));
        }
    }
}
