#pragma once

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "UI/Components/UIComponent.h"
#include "Reflection/Reflection.h"
#include "UI/UIControl.h"

namespace DAVA
{
class UIDataListComponent : public UIComponent
{
    DAVA_VIRTUAL_REFLECTION(UIDataListComponent, UIComponent);
    DECLARE_UI_COMPONENT(UIDataListComponent);

public:
    UIDataListComponent() = default;
    UIDataListComponent(const UIDataListComponent& c);

    UIDataListComponent& operator=(const UIDataListComponent& c) = delete;

    UIDataListComponent* Clone() const override;

    const FilePath& GetCellPackage() const;
    void SetCellPackage(const FilePath& package);

    const String& GetCellControlName() const;
    void SetCellControlName(const String& control);

    const String& GetDataContainer() const;
    void SetDataContainer(const String& dataContainer);

    bool IsDirty() const;
    void SetDirty(bool dirty);

    bool IsSelectionSupported() const;
    void SetSelectionSupported(bool supported);

    int32 GetSelectedIndex() const;
    void SetSelectedIndex(int32 index);

protected:
    ~UIDataListComponent() override = default;

private:
    FilePath cellPackage;
    String cellControlName;
    String dataContainer;

    bool isDirty = false;

    bool selectionSupported = false;
    int32 selectedIndex = -1;
};
}
