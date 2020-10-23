#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
/**
    Script component.
    Add cusom behaviour throuth custom Lua or Native controller.
*/
class UIScriptComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIScriptComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIScriptComponent);

public:
    UIScriptComponent();
    UIScriptComponent(const UIScriptComponent& src);
    UIScriptComponent& operator=(const UIScriptComponent&) = delete;
    UIScriptComponent* Clone() const override;

    /** Reflection type name of native controller */
    const String& GetReflectionTypeName() const;
    void SetReflectionTypeName(const String& typeName);

    /** File path to Lua-script controller */
    const FilePath& GetLuaScriptPath() const;
    void SetLuaScriptPath(const FilePath& filePath);

    /** Raw string with parameters for controller. */
    const String& GetParameters() const;
    void SetParameters(const String& value);

    /** Is parameters string changed? */
    bool GetModifiedParameters() const;
    void SetModifiedParameters(bool value);

    /** Is controller script or type changed? */
    bool GetModifiedScripts() const;
    void SetModifiedScripts(bool value);

private:
    ~UIScriptComponent() override;

    String reflectionTypeName;
    FilePath luaScriptPath;
    String parameters;

    bool modifiedParameters = false;
    bool modifiedScripts = false;
};

inline const String& UIScriptComponent::GetReflectionTypeName() const
{
    return reflectionTypeName;
}

inline const FilePath& UIScriptComponent::GetLuaScriptPath() const
{
    return luaScriptPath;
}

inline const String& UIScriptComponent::GetParameters() const
{
    return parameters;
}
}
