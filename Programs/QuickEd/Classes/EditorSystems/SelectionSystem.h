#pragma once

#include "EditorSystems/SelectionContainer.h"
#include "EditorSystems/BaseEditorSystem.h"

#include <TArc/DataProcessing/SettingsNode.h>
#include <TArc/DataProcessing/DataWrapper.h>

#include <Reflection/Reflection.h>

#include <Math/Rect.h>
#include <UI/UIEvent.h>

class EditorSystemsManager;
class ControlNode;
class ControlsContainerNode;

namespace DAVA
{
class Vector2;
}

class SelectionSettings : public DAVA::SettingsNode
{
public:
    bool canFindCommonForSelection = false;

    DAVA_VIRTUAL_REFLECTION(SelectionSettings, DAVA::SettingsNode);
};

class SelectionSystem : public BaseEditorSystem
{
public:
    SelectionSystem(DAVA::ContextAccessor* accessor);

    void ClearSelection();
    void SelectAllControls();
    void FocusNextChild();
    void FocusPreviousChild();

    void SelectNode(ControlNode* node);

    ControlNode* GetNearestNodeUnderPoint(const DAVA::Vector2& point) const;
    ControlNode* GetCommonNodeUnderPoint(const DAVA::Vector2& point, bool canGoDeeper) const;

private:
    bool CanProcessInput(DAVA::UIEvent* currentInput, eInputSource eInputSource) const override;
    void ProcessInput(DAVA::UIEvent* currentInput, eInputSource eInputSource) override;
    eSystems GetOrder() const override;

    void GetNodesForSelection(DAVA::Vector<ControlNode*>& nodesUnderPoint, const DAVA::Vector2& point) const;

    void FocusToChild(bool next);
    void SelectNodesImpl(const SelectedNodes& selection);

    ControlNode* FindSmallNodeUnderNode(const DAVA::Vector<ControlNode*>& nodesUnderPoint) const;

    DAVA::DataWrapper documentDataWrapper;
};
