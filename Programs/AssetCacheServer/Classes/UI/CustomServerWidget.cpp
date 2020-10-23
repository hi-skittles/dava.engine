#include "CustomServerWidget.h"

#include <QValidator>

CustomServerWidget::CustomServerWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::CustomServerWidget)
{
    ui->setupUi(this);

    ui->ipLineEdit->setText(DAVA::AssetCache::GetLocalHost().c_str());

    connect(ui->removeServerButton, &QPushButton::clicked,
            this, &CustomServerWidget::RemoveLater);
    connect(ui->ipLineEdit, &QLineEdit::textChanged,
            this, &CustomServerWidget::ParametersChanged);
    connect(ui->enabledCheckBox, SIGNAL(stateChanged(int)), this, SLOT(OnChecked(int)));
}

CustomServerWidget::CustomServerWidget(const RemoteServerParams& newServer, QWidget* parent)
    : CustomServerWidget(parent)
{
    ui->enabledCheckBox->setChecked(newServer.enabled);
    ui->ipLineEdit->setText(newServer.ip.c_str());
}

RemoteServerParams CustomServerWidget::GetServerData() const
{
    return RemoteServerParams(ui->ipLineEdit->text().toStdString(), ui->enabledCheckBox->isChecked());
}

void CustomServerWidget::OnChecked(int val)
{
    emit ServerChecked(val == Qt::Checked);
}

bool CustomServerWidget::IsChecked() const
{
    return ui->enabledCheckBox->isChecked();
}

void CustomServerWidget::SetChecked(bool checked)
{
    ui->enabledCheckBox->setChecked(checked);
}
