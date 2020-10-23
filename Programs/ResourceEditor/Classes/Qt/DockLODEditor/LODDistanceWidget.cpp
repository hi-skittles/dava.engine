#include "Classes/Qt/DockLODEditor/LODDistanceWidget.h"

#include "Classes/Qt/Tools/EventFilterDoubleSpinBox/EventFilterDoubleSpinBox.h"

#include <REPlatform/Scene/Systems/EditorLODSystem.h>

#include <TArc/Utils/Utils.h>

#include <Utils/StringFormat.h>

#include <QApplication>
#include <QBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QSignalBlocker>

namespace LODDistanceWidgetDetail
{
const QString multiplePlaceHolder = "multiple values";
}

LODDistanceWidget::LODDistanceWidget(QWidget* parent)
    : QWidget(parent)
{
    CreateUI();
    SetupSignals();

    UpdateLabelText();
    UpdateDistanceValues();
    UpdateDisplayingStyle();
}

void LODDistanceWidget::CreateUI()
{
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setMargin(0);
    setLayout(layout);

    label = new QLabel(this);
    QSizePolicy sizePolicyLabel(QSizePolicy::Minimum, QSizePolicy::Fixed);
    sizePolicyLabel.setHorizontalStretch(0);
    sizePolicyLabel.setVerticalStretch(0);
    sizePolicyLabel.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
    label->setSizePolicy(sizePolicyLabel);
    label->setMinimumSize(QSize(70, 0));
    label->setLayoutDirection(Qt::LeftToRight);
    label->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);
    label->setToolTip("Trinagles count");
    layout->addWidget(label);

    deleteButton = new QPushButton(this);
    QSizePolicy sizePolicyButton(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicyButton.setHorizontalStretch(0);
    sizePolicyButton.setVerticalStretch(0);
    sizePolicyButton.setHeightForWidth(deleteButton->sizePolicy().hasHeightForWidth());
    deleteButton->setSizePolicy(sizePolicyButton);
    deleteButton->setMinimumSize(QSize(24, 24));
    deleteButton->setMaximumSize(QSize(24, 24));
    deleteButton->setIcon(DAVA::SharedIcon(":/QtIcons/remove.png"));
    deleteButton->setToolTip("Remove geometry");
    deleteButton->setEnabled(active && canDelete);
    layout->addWidget(deleteButton);

    spinBox = new EventFilterDoubleSpinBox(this);
    QSizePolicy sizePolicySpinBox(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizePolicySpinBox.setHorizontalStretch(0);
    sizePolicySpinBox.setVerticalStretch(0);
    sizePolicySpinBox.setHeightForWidth(spinBox->sizePolicy().hasHeightForWidth());
    spinBox->setSizePolicy(sizePolicySpinBox);
    spinBox->setMinimum(minValue);
    spinBox->setMaximum(maxValue);
    spinBox->setValue(distance);
    spinBox->setFocusPolicy(Qt::WheelFocus);
    spinBox->setKeyboardTracking(false);
    spinBox->setToolTip("Switching of LODs distance");
    spinBox->setMinimumSize(QSize(80, 24));

    layout->addWidget(spinBox);

    multipleText = new QLineEdit(this);
    multipleText->setPlaceholderText(LODDistanceWidgetDetail::multiplePlaceHolder);
    multipleText->setMinimumSize(QSize(80, 24));
    layout->addWidget(multipleText);

    resetButton = new QPushButton(this);
    sizePolicyButton.setHeightForWidth(resetButton->sizePolicy().hasHeightForWidth());
    resetButton->setSizePolicy(sizePolicyButton);
    resetButton->setMinimumSize(QSize(24, 24));
    resetButton->setMaximumSize(QSize(24, 24));
    resetButton->setToolTip("Reset distance");

    resetButton->setIcon(DAVA::SharedIcon(":/QtIcons/reset.png"));
    layout->addWidget(resetButton);
}

void LODDistanceWidget::SetupSignals()
{
    connect(spinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), this, &LODDistanceWidget::DistanceChangedBySpinBox);
    connect(multipleText, &QLineEdit::editingFinished, this, &LODDistanceWidget::DistanceChangedByLineEdit);
    connect(deleteButton, &QPushButton::released, this, &LODDistanceWidget::DistanceRemoved);
    connect(resetButton, &QPushButton::released, this, &LODDistanceWidget::DistanceChangedByResetButton);
}

void LODDistanceWidget::SetActive(bool active_)
{
    if (active != active_)
    {
        active = active_;
        deleteButton->setEnabled(active && canDelete);
        UpdateDisplayingStyle();
    }
}

void LODDistanceWidget::SetCanDelete(bool canDelete_)
{
    if (canDelete != canDelete_)
    {
        canDelete = canDelete_;
        deleteButton->setEnabled(active && canDelete);
    }
}

DAVA::float32 LODDistanceWidget::ClampDistanceBySpinBox(DAVA::float32 newDistance)
{
    QSignalBlocker guardSpinBox(spinBox);

    spinBox->setValue(newDistance);
    return static_cast<DAVA::float32>(spinBox->value());
}

void LODDistanceWidget::SetDistance(DAVA::float32 distance_, bool isMultiple_)
{
    if (distance != distance_ || isMultiple != isMultiple_)
    {
        distance = ClampDistanceBySpinBox(distance_);
        isMultiple = isMultiple_;

        UpdateDistanceValues();
    }
}

DAVA::float32 LODDistanceWidget::GetDistance() const
{
    return distance;
}

void LODDistanceWidget::SetMinMax(DAVA::float64 min, DAVA::float64 max)
{
    if (minValue != min || maxValue != max)
    {
        minValue = min;
        maxValue = max;

        { //TODO do we need to notify systems?
            QSignalBlocker guardSpinBox(spinBox);
            spinBox->setMinimum(minValue);
            spinBox->setMaximum(maxValue);
        }
    }
}

void LODDistanceWidget::SetIndex(DAVA::int32 widgetIndex_)
{
    if (widgetIndex != widgetIndex_)
    {
        widgetIndex = widgetIndex_;
        UpdateLabelText();
    }
}

void LODDistanceWidget::SetTrianglesCount(DAVA::uint32 trianglesCount_)
{
    if (trianglesCount != trianglesCount_)
    {
        trianglesCount = trianglesCount_;
        UpdateLabelText();
    }
}

void LODDistanceWidget::DistanceChangedBySpinBox(double value)
{
    distance = static_cast<DAVA::float32>(value);
    isMultiple = false;

    UpdateDistanceValues();

    emit DistanceChanged();
}

void LODDistanceWidget::DistanceChangedByResetButton()
{
    distance = ClampDistanceBySpinBox(DAVA::EditorLODSystem::LOD_DISTANCE_INFINITY);
    isMultiple = false;

    UpdateDistanceValues();
    emit DistanceChanged();
}

void LODDistanceWidget::DistanceChangedByLineEdit()
{
    bool ok = false;
    QString text = multipleText->text();
    DAVA::float32 value = text.toFloat(&ok);
    if (ok)
    {
        distance = ClampDistanceBySpinBox(value);
        isMultiple = false;

        UpdateDistanceValues();
        emit DistanceChanged();
    }
}

void LODDistanceWidget::UpdateLabelText()
{
    label->setText(DAVA::Format("%u. (%u)", widgetIndex, trianglesCount).c_str());
}

void LODDistanceWidget::UpdateDistanceValues()
{
    QSignalBlocker guardSpinBox(spinBox);
    QSignalBlocker guardMultipleText(multipleText);

    spinBox->setVisible(!isMultiple);
    multipleText->setVisible(isMultiple);

    spinBox->setValue(distance);
    if (isMultiple)
    {
        multipleText->setText("");
    }
    else
    {
        multipleText->setText(QString::number(distance));
    }
}

void LODDistanceWidget::UpdateDisplayingStyle()
{
    const QPalette globalPalette = qApp->palette();

    QPalette spinBoxPalette = spinBox->GetTextPalette();
    QPalette textPalette = multipleText->palette();

    if (active)
    {
        spinBoxPalette.setColor(QPalette::Text, globalPalette.color(QPalette::Text));
        textPalette.setColor(QPalette::Text, globalPalette.color(QPalette::Text));
    }
    else
    {
        spinBoxPalette.setColor(QPalette::Text, globalPalette.color(QPalette::Disabled, QPalette::Text));
        textPalette.setColor(QPalette::Text, globalPalette.color(QPalette::Disabled, QPalette::Text));
    }

    spinBox->SetTextPalette(spinBoxPalette);
    multipleText->setPalette(textPalette);
}
