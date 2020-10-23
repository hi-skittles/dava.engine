#ifndef __LANDSCAPETOOLSTOGGLECOMMAND_H__
#define __LANDSCAPETOOLSTOGGLECOMMAND_H__

#include "Functional/Function.h"
#include "Commands2/Base/RECommand.h"
#include "Scene/System/LandscapeEditorDrawSystem.h"

class SceneEditor2;

class LandscapeToolsToggleCommand : public RECommand
{
public:
    LandscapeToolsToggleCommand(DAVA::int32 identifier, SceneEditor2* sceneEditor,
                                const DAVA::String& commandDescr, bool isEnabling,
                                DAVA::uint32 allowedTools, DAVA::String disablingError);
    DAVA::Entity* GetEntity() const;

    void Redo() override;
    void Undo() override;
    bool IsClean() const override;

    void SaveEnabledToolsState();
    void ApplySavedState();

    using IsEnabledFunction = DAVA::Function<bool()>;
    using EnableFunction = DAVA::Function<LandscapeEditorDrawSystem::eErrorType()>;
    using DisableFunction = DAVA::Function<bool()>;

protected:
    virtual void OnEnabled();
    virtual void OnDisabled();

protected:
    SceneEditor2* sceneEditor = nullptr;
    DAVA::String disablingError;
    DAVA::uint32 allowedTools = 0;
    DAVA::int32 enabledTools = 0;
    IsEnabledFunction isEnabledFunction;
    EnableFunction enableFunction;
    DisableFunction disableFunction;
};

inline bool LandscapeToolsToggleCommand::IsClean() const
{
    return true;
}

template <typename ForwardCommand>
class LandscapeToolsReverseCommand : public ForwardCommand
{
public:
    template <typename... Args>
    LandscapeToolsReverseCommand(SceneEditor2* sceneEditor, Args... a)
        : ForwardCommand(sceneEditor, a..., false)
    {
    }

    inline void Redo() override
    {
        ForwardCommand::Undo();
    }

    inline void Undo() override
    {
        ForwardCommand::Redo();
    }
};

/*
 * Concerete commands
 */
class EnableHeightmapEditorCommand : public LandscapeToolsToggleCommand
{
public:
    EnableHeightmapEditorCommand(SceneEditor2* forSceneEditor, bool isEnabling = true);

private:
    void OnDisabled() override;
};
using DisableHeightmapEditorCommand = LandscapeToolsReverseCommand<EnableHeightmapEditorCommand>;

class EnableNotPassableCommand : public LandscapeToolsToggleCommand
{
public:
    EnableNotPassableCommand(SceneEditor2* forSceneEditor, bool isEnabling = true);
};
using DisableNotPassableCommand = LandscapeToolsReverseCommand<EnableNotPassableCommand>;

class EnableRulerToolCommand : public LandscapeToolsToggleCommand
{
public:
    EnableRulerToolCommand(SceneEditor2* forSceneEditor, bool isEnabling = true);
};
using DisableRulerToolCommand = LandscapeToolsReverseCommand<EnableRulerToolCommand>;

class EnableTilemaskEditorCommand : public LandscapeToolsToggleCommand
{
public:
    EnableTilemaskEditorCommand(SceneEditor2* forSceneEditor, bool isEnabling = true);
};
using DisableTilemaskEditorCommand = LandscapeToolsReverseCommand<EnableTilemaskEditorCommand>;

class EnableCustomColorsCommand : public LandscapeToolsToggleCommand
{
public:
    EnableCustomColorsCommand(SceneEditor2* forSceneEditor, bool saveChanges, bool isEnabling = true);

private:
    void OnEnabled() override;
};
using DisableCustomColorsCommand = LandscapeToolsReverseCommand<EnableCustomColorsCommand>;

#endif // __LANDSCAPETOOLSTOGGLECOMMAND_H__
