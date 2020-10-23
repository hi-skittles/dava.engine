#include "SizeDialog.h"

#include <QLabel>

SizeDialog::SizeDialog(QWidget* parent)
    :
    QDialog(parent)
{
    verticalLayout = new QVBoxLayout(this);

    messageLbl = new QLabel(this);
    messageLbl->setText("Set new size for image:");
    verticalLayout->addWidget(messageLbl);

    horLayout = new QHBoxLayout(this);

    widthLbl = new QLabel(this);
    widthLbl->setText("Width:");

    widthSpinBox = new QSpinBox(this);
    widthSpinBox->setMinimum(1);
    widthSpinBox->setMaximum(99999);
    widthSpinBox->setSingleStep(1);

    heightLbl = new QLabel(this);
    heightLbl->setText("Height:");

    heightSpinBox = new QSpinBox(this);
    heightSpinBox->setMinimum(1);
    heightSpinBox->setMaximum(99999);
    heightSpinBox->setSingleStep(1);

    horLayout->addWidget(widthLbl);
    horLayout->addWidget(widthSpinBox);
    horLayout->addWidget(heightLbl);
    horLayout->addWidget(heightSpinBox);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    verticalLayout->addLayout(horLayout);
    verticalLayout->addWidget(buttonBox);
}

SizeDialog::~SizeDialog()
{
    delete horLayout;
    delete verticalLayout;
    delete messageLbl;
    delete widthLbl;
    delete widthSpinBox;
    delete heightLbl;
    delete heightSpinBox;
    delete buttonBox;
}

DAVA::Vector2 SizeDialog::GetSize() const
{
    return DAVA::Vector2(widthSpinBox->value(), heightSpinBox->value());
}
