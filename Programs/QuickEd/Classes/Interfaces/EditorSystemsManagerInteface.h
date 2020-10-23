#pragma once

class BaseEditorSystem;

namespace Interfaces
{
class EditorSystemsManagerInterface
{
public:
    virtual void RegisterEditorSystem(BaseEditorSystem* editorSystem) = 0;
    virtual void UnregisterEditorSystem(BaseEditorSystem* editorSystem) = 0;
};
}