#pragma once

#include <Base/BaseTypes.h>
#include <QDialog>

class FindFilter;

namespace Ui
{
class FindInProjectDialog;
}

class FindInProjectDialog : public QDialog
{
    Q_OBJECT
public:
    FindInProjectDialog(QWidget* parent = nullptr);
    ~FindInProjectDialog() override;

    std::unique_ptr<FindFilter> BuildFindFilter() const;

private:
    std::unique_ptr<Ui::FindInProjectDialog> ui;
};
