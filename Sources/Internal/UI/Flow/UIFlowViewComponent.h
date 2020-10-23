#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"
#include "Reflection/Reflection.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
/**
    Component configures view representation of state.

    It contains path to yaml with content, control name which will be loaded
    from yaml and path to control-container on parent view where the loaded
    view will be added.

    Also this component configures data-binding data source by the name which
    was stored in `context.data` and data scope expression.
*/
class UIFlowViewComponent final : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIFlowViewComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIFlowViewComponent);

public:
    /** Default constructor. */
    UIFlowViewComponent();
    /** Copy constructor. */
    UIFlowViewComponent(const UIFlowViewComponent& src);
    /** Deleted assign operator. */
    UIFlowViewComponent& operator=(const UIFlowViewComponent&) = delete;

    UIFlowViewComponent* Clone() const override;

    /** Return path to view package yaml. */
    const FilePath& GetViewYaml() const;
    /** Setup path to view package yaml. */
    void SetViewYaml(const FilePath& path);
    /** Return path to control which need load from package. */
    const String& GetControlName() const;
    /** Setup path to control which need load from package. */
    void SetControlName(const String& name);
    /** Return container path where should appended loaded control. */
    const String& GetContainerPath() const;
    /** Setup container path where should appended loaded control. */
    void SetContainerPath(const String& path);
    /** Return model name from UIFlowContext.data which should binded to loaded view. */
    const String& GetModelName() const;
    /** Setup model name from UIFlowContext.data which should binded to loaded view. */
    void SetModelName(const String& name);
    /** Return data expression which should added to loaded view. */
    const String& GetModelScope() const;
    /** Setup data expression which should added to loaded view. */
    void SetModelScope(const String& scope);

private:
    ~UIFlowViewComponent() override;

    FilePath viewYamlPath;
    String controlName;
    String containerPath;
    String modelName;
    String modelScope;
};

inline const FilePath& UIFlowViewComponent::GetViewYaml() const
{
    return viewYamlPath;
}

inline const String& UIFlowViewComponent::GetControlName() const
{
    return controlName;
}

inline const String& UIFlowViewComponent::GetContainerPath() const
{
    return containerPath;
}

inline const String& UIFlowViewComponent::GetModelName() const
{
    return modelName;
}

inline const String& UIFlowViewComponent::GetModelScope() const
{
    return modelScope;
}
}
