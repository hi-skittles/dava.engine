#include "UI/Find/Widgets/ComboBoxFilterEditor.h"
#include "UI/Find/Filters/FindFilter.h"

using namespace DAVA;

ComboBoxFilterEditor::ComboBoxFilterEditor(QWidget* parent, const DAVA::Vector<ComboBoxData>& data, const ComboBoxFilterBuilder& findFilterBuilder_)
    : FindFilterEditor(parent)
    , findFilterBuilder(findFilterBuilder_)
{
    layout = new QHBoxLayout(this);

    enumCombobox = new QComboBox(this);

    for (const ComboBoxData& d : data)
    {
        enumCombobox->addItem(d.description.c_str(), QVariant(d.userData));
    }

    layout->addWidget(enumCombobox);
    layout->addStretch();

    layout->setMargin(0);
    layout->setSpacing(0);

    QObject::connect(enumCombobox, SIGNAL(currentIndexChanged(int)), this, SIGNAL(FilterChanged()));

    setFocusProxy(enumCombobox);
}

uint64 ComboBoxFilterEditor::GetUserData() const
{
    return enumCombobox->currentData().toULongLong();
}

std::unique_ptr<FindFilter> ComboBoxFilterEditor::BuildFindFilter()
{
    return findFilterBuilder(this);
}
