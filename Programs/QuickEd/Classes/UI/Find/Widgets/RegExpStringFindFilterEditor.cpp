#include "UI/Find/Widgets/RegExpStringFindFilterEditor.h"
#include "UI/Find/Filters/FindFilter.h"

using namespace DAVA;

RegExpStringFindFilterEditor::RegExpStringFindFilterEditor(QWidget* parent, const RegExpStringFindFilterBuilder& findFilterBuilder_)
    : FindFilterEditor(parent)
    , findFilterBuilder(findFilterBuilder_)
{
    layout = new QHBoxLayout(this);

    matchesLabel = new QLabel(this);
    matchesLabel->setText(tr("matches"));

    caseSensitive = new QCheckBox(this);
    caseSensitive->setText(tr("case sensitive"));
    caseSensitive->setChecked(false);

    value = new QLineEdit(this);

    layout->addWidget(matchesLabel);
    layout->addSpacing(10);
    layout->addWidget(value);
    layout->addStretch();
    layout->addSpacing(10);
    layout->addWidget(caseSensitive);

    layout->setMargin(0);
    layout->setSpacing(0);

    QObject::connect(value, SIGNAL(textChanged(const QString&)), this, SIGNAL(FilterChanged()));
    QObject::connect(caseSensitive, SIGNAL(stateChanged(int)), this, SIGNAL(FilterChanged()));

    setFocusProxy(value);
}

String RegExpStringFindFilterEditor::GetString() const
{
    return value->text().toStdString();
}

bool RegExpStringFindFilterEditor::IsCaseSensitive() const
{
    return caseSensitive->isChecked();
}

std::unique_ptr<FindFilter> RegExpStringFindFilterEditor::BuildFindFilter()
{
    return findFilterBuilder(this);
}
