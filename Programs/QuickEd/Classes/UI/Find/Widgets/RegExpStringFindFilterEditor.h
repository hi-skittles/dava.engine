#pragma once

#include "UI/Find/Widgets/FindFilterEditor.h"

#include <Base/BaseTypes.h>
#include <Functional/Functional.h>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

class RegExpStringFindFilterEditor
: public FindFilterEditor
{
public:
    using RegExpStringFindFilterBuilder = DAVA::Function<std::unique_ptr<FindFilter>(const RegExpStringFindFilterEditor*)>;

    RegExpStringFindFilterEditor(QWidget* parent, const RegExpStringFindFilterBuilder& findFilterBuilder);

    DAVA::String GetString() const;
    bool IsCaseSensitive() const;

    std::unique_ptr<FindFilter> BuildFindFilter() override;

private:
    QHBoxLayout* layout = nullptr;
    QLabel* matchesLabel = nullptr;
    QLineEdit* value = nullptr;
    QCheckBox* caseSensitive = nullptr;

    RegExpStringFindFilterBuilder findFilterBuilder;
};
