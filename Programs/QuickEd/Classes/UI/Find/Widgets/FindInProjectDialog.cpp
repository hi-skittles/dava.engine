#include "UI/Find/Widgets/FindInProjectDialog.h"
#include "UI/Find/Filters/FindFilter.h"
#include "UI/Find/Widgets/CompositeFindFilterWidget.h"

#include "ui_FindInProjectDialog.h"

using namespace DAVA;

FindInProjectDialog::FindInProjectDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::FindInProjectDialog())
{
    ui->setupUi(this);

    QObject::connect(ui->findButton, SIGNAL(clicked()), this, SLOT(accept()));
    QObject::connect(ui->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

FindInProjectDialog::~FindInProjectDialog() = default;

std::unique_ptr<FindFilter> FindInProjectDialog::BuildFindFilter() const
{
    return ui->filtersList->BuildFindFilter();
}
