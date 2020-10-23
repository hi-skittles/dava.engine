#pragma once

#include "UI/Find/Widgets/FindFilterEditor.h"

#include <Base/BaseTypes.h>
#include <Base/EnumMap.h>
#include <Functional/Functional.h>
#include <QHBoxLayout>
#include <QComboBox>

class ComboBoxFilterEditor
: public FindFilterEditor
{
public:
    using ComboBoxFilterBuilder = DAVA::Function<std::unique_ptr<FindFilter>(const ComboBoxFilterEditor*)>;

    struct ComboBoxData
    {
        DAVA::String description;
        DAVA::uint64 userData; //Any should be used, lack QVariant<->Any converters
    };

    ComboBoxFilterEditor(QWidget* parent, const DAVA::Vector<ComboBoxData>& data, const ComboBoxFilterBuilder& findFilterBuilder);

    DAVA::uint64 GetUserData() const;
    std::unique_ptr<FindFilter> BuildFindFilter() override;

private:
    QHBoxLayout* layout = nullptr;
    QComboBox* enumCombobox = nullptr;

    ComboBoxFilterBuilder findFilterBuilder;
};
