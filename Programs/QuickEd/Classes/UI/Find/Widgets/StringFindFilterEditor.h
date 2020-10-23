#pragma once

#include "UI/Find/Widgets/FindFilterEditor.h"

#include <Base/BaseTypes.h>
#include <Functional/Functional.h>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

class StringFindFilterEditor
: public FindFilterEditor
{
public:
    using StringFindFilterBuilder = DAVA::Function<std::unique_ptr<FindFilter>(const StringFindFilterEditor*)>;

    StringFindFilterEditor(QWidget* parent, const StringFindFilterBuilder& findFilterBuilder);

    DAVA::String GetString() const;

    std::unique_ptr<FindFilter> BuildFindFilter() override;

private:
    QHBoxLayout* layout = nullptr;
    QLineEdit* value = nullptr;

    StringFindFilterBuilder findFilterBuilder;
};
