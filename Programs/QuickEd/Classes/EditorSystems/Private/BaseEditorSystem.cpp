#include "Classes/EditorSystems/BaseEditorSystem.h"
#include "Classes/Modules/DocumentsModule/EditorSystemsData.h"
#include "Classes/Painter/Painter.h"

#include <TArc/Core/ContextAccessor.h>
#include <UI/UIEvent.h>

BaseEditorSystem::BaseEditorSystem(DAVA::ContextAccessor* accessor_)
    : accessor(accessor_)
{
}

const EditorSystemsManager* BaseEditorSystem::GetSystemsManager() const
{
    DAVA::DataContext* globalContext = accessor->GetGlobalContext();
    EditorSystemsData* editorData = globalContext->GetData<EditorSystemsData>();
    DVASSERT(editorData != nullptr);
    const EditorSystemsManager* systemsManager = editorData->GetSystemsManager();
    DVASSERT(systemsManager != nullptr);
    return systemsManager;
}

EditorSystemsManager* BaseEditorSystem::GetSystemsManager()
{
    using namespace DAVA;
    DataContext* globalContext = accessor->GetGlobalContext();
    EditorSystemsData* editorData = globalContext->GetData<EditorSystemsData>();
    DVASSERT(editorData != nullptr);
    EditorSystemsManager* systemsManager = editorData->systemsManager.get();
    DVASSERT(systemsManager != nullptr);
    return systemsManager;
}

Painting::Painter* BaseEditorSystem::GetPainter() const
{
    using namespace DAVA;
    DataContext* globalContext = accessor->GetGlobalContext();
    EditorSystemsData* editorData = globalContext->GetData<EditorSystemsData>();
    DVASSERT(editorData != nullptr);
    Painting::Painter* painter = editorData->painter.get();
    DVASSERT(painter != nullptr);
    return painter;
}

void BaseEditorSystem::ProcessInput(DAVA::UIEvent* /*currentInput*/, eInputSource /*inputSource*/)
{
}

bool BaseEditorSystem::CanProcessInput(DAVA::UIEvent* currentInput, eInputSource /*inputSource*/) const
{
    return false;
}

eDragState BaseEditorSystem::RequireNewState(DAVA::UIEvent* currentInput, eInputSource /*inputSource*/)
{
    return eDragState::NoDrag;
}

void BaseEditorSystem::OnDragStateChanged(eDragState /*currentState*/, eDragState /*previousState*/)
{
}

void BaseEditorSystem::OnDisplayStateChanged(eDisplayState /*currentState*/, eDisplayState /*previousState*/)
{
}

CanvasControls BaseEditorSystem::CreateCanvasControls()
{
    return CanvasControls();
}

void BaseEditorSystem::DeleteCanvasControls(const CanvasControls& canvasControls)
{
}

void BaseEditorSystem::OnUpdate()
{
}

void BaseEditorSystem::Invalidate()
{
}
