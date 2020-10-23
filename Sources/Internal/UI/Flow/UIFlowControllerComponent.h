#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
/**
    Component attaches controller (native or lua) to the state.

    Native controller should be inherited from `UIFlowController`.
    Lua script can contains next functions:
        `init(context)`, `activate(context, view)`,
        `loadResources(context, view)`, `process(frameDelta)`,
        `processEvent(eventName)` (must return true for avoid sending
            current event next), `deactivate(context, view)`,
        `unloadResources(context, view)`, `release(context)`.
*/
class UIFlowControllerComponent final : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIFlowControllerComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIFlowControllerComponent);

public:
    /** Default constructor. */
    UIFlowControllerComponent();
    /** Copy constructor. */
    UIFlowControllerComponent(const UIFlowControllerComponent& src);
    /** Removed assign operator. */
    UIFlowControllerComponent& operator=(const UIFlowControllerComponent&) = delete;

    UIFlowControllerComponent* Clone() const override;

    /** Return reflected name of native controller. */
    const String& GetReflectionTypeName() const;
    /** Setup reflected name of native controller. */
    void SetReflectionTypeName(const String& typeName);

    /** Return path to Lua controller script file. */
    const FilePath& GetLuaScriptPath() const;
    /** Setup path to Lua controller script file. */
    void SetLuaScriptPath(const FilePath& filePath);

private:
    /** Private destructor. */
    ~UIFlowControllerComponent() override;

    String reflectionTypeName;
    FilePath luaScriptPath;
};

inline const String& UIFlowControllerComponent::GetReflectionTypeName() const
{
    return reflectionTypeName;
}

inline const FilePath& UIFlowControllerComponent::GetLuaScriptPath() const
{
    return luaScriptPath;
}
}
