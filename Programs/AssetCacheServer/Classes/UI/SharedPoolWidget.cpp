#include "UI/SharedPoolWidget.h"

#include <QValidator>

SharedPoolWidget::SharedPoolWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::SharedPoolWidget)
{
    ui->setupUi(this);
    connect(ui->enabledCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnChecked(int)));
}

SharedPoolWidget::SharedPoolWidget(const SharedPool& pool, QWidget* parent)
    : SharedPoolWidget(parent)
{
    ui->enabledCheckBox->setChecked(pool.enabled);
    ui->nameLineEdit->setText(pool.poolName.c_str());
    ui->descriptionLabel->setText(pool.poolDescription.c_str());
    poolID = pool.poolID;
}

void SharedPoolWidget::Update(const SharedPool& pool)
{
    ui->nameLineEdit->setText(pool.poolName.c_str());
    ui->descriptionLabel->setText(pool.poolDescription.c_str());
}

void SharedPoolWidget::OnChecked(int val)
{
    emit PoolChecked(val == Qt::Checked);
}

bool SharedPoolWidget::IsChecked() const
{
    return ui->enabledCheckBox->isChecked();
}

void SharedPoolWidget::SetChecked(bool checked)
{
    ui->enabledCheckBox->setChecked(checked);
}
