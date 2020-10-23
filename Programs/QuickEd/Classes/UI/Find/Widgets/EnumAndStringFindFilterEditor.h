#pragma once

#include "UI/Find/Widgets/FindFilterEditor.h"

#include <Base/BaseTypes.h>
#include <Base/EnumMap.h>
#include <Functional/Functional.h>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>

class EnumAndStringFindFilterEditor
: public FindFilterEditor
{
public:
    using EnumAndStringFindFilterBuilder = DAVA::Function<std::unique_ptr<FindFilter>(const EnumAndStringFindFilterEditor*)>;

    EnumAndStringFindFilterEditor(QWidget* parent, const EnumMap* editedEnum, const EnumAndStringFindFilterBuilder& findFilterBuilder);

    DAVA::int32 GetEnumValue() const;
    DAVA::String GetString() const;

    std::unique_ptr<FindFilter> BuildFindFilter() override;

private:
    QHBoxLayout* layout = nullptr;
    QComboBox* enumCombobox = nullptr;
    QLineEdit* value = nullptr;

    EnumAndStringFindFilterBuilder findFilterBuilder;
};
