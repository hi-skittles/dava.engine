#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/FilePath.h"
#include "UI/Components/UIComponent.h"

namespace DAVA
{
class UIDataSourceComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIDataSourceComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIDataSourceComponent);

public:
    enum eSourceType
    {
        FROM_REFLECTION,
        FROM_FILE,
        FROM_EXPRESSION
    };
    UIDataSourceComponent() = default;
    UIDataSourceComponent(const String& modelName);
    UIDataSourceComponent(const UIDataSourceComponent& c);

    UIDataSourceComponent& operator=(const UIDataSourceComponent& c) = delete;

    UIDataSourceComponent* Clone() const override;

    eSourceType GetSourceType() const;
    void SetSourceType(eSourceType source);

    const Reflection& GetData() const;
    void SetData(const Reflection& data);

    const String& GetSource() const;
    void SetSource(const String& value);

    bool IsDirty() const;
    void SetDirty(bool dirty_);

protected:
    ~UIDataSourceComponent() override = default;

private:
    eSourceType sourceType;
    Reflection data;
    String source;
    bool isDirty = false;
};
}
