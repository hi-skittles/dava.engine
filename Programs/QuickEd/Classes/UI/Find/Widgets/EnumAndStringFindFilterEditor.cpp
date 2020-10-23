#include "UI/Find/Widgets/EnumAndStringFindFilterEditor.h"
#include "UI/Find/Filters/FindFilter.h"

using namespace DAVA;

EnumAndStringFindFilterEditor::EnumAndStringFindFilterEditor(QWidget* parent, const EnumMap* editedEnum, const EnumAndStringFindFilterBuilder& findFilterBuilder_)
    : FindFilterEditor(parent)
    , findFilterBuilder(findFilterBuilder_)
{
    layout = new QHBoxLayout(this);

    enumCombobox = new QComboBox(this);

    for (int32 enumIndex = 0; enumIndex < editedEnum->GetCount(); ++enumIndex)
    {
        enumCombobox->addItem(editedEnum->ToString(enumIndex));
    }

    layout->addWidget(enumCombobox);

    layout->addSpacing(10);

    value = new QLineEdit(this);

    layout->addWidget(value);
    layout->addStretch();
    layout->addSpacing(10);

    layout->addStretch();

    layout->setMargin(0);
    layout->setSpacing(0);

    QObject::connect(enumCombobox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(FilterChanged()));

    setFocusProxy(enumCombobox);
}

int32 EnumAndStringFindFilterEditor::GetEnumValue() const
{
    return enumCombobox->currentIndex();
}

DAVA::String EnumAndStringFindFilterEditor::GetString() const
{
    return value->text().toStdString();
}

std::unique_ptr<FindFilter> EnumAndStringFindFilterEditor::BuildFindFilter()
{
    return findFilterBuilder(this);
}
