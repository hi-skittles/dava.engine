#include "UI/Find/Widgets/StringFindFilterEditor.h"
#include "UI/Find/Filters/FindFilter.h"

using namespace DAVA;

StringFindFilterEditor::StringFindFilterEditor(QWidget* parent, const StringFindFilterBuilder& findFilterBuilder_)
    : FindFilterEditor(parent)
    , findFilterBuilder(findFilterBuilder_)
{
    layout = new QHBoxLayout(this);

    value = new QLineEdit(this);

    layout->addWidget(value);
    layout->addStretch();
    layout->addSpacing(10);

    layout->setMargin(0);
    layout->setSpacing(0);

    QObject::connect(value, SIGNAL(textChanged(const QString&)), this, SIGNAL(FilterChanged()));

    setFocusProxy(value);
}

String StringFindFilterEditor::GetString() const
{
    return value->text().toStdString();
}

std::unique_ptr<FindFilter> StringFindFilterEditor::BuildFindFilter()
{
    return findFilterBuilder(this);
}
