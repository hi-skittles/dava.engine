#include "ModificationWidget.h"
#include "Commands2/TransformCommand.h"
#include "Commands2/Base/RECommandNotificationObject.h"
#include "Math/MathHelpers.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/Project/ProjectManagerData.h"
#include "Classes/Selection/SelectionData.h"

#include "TArc/Core/FieldBinder.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QStylePainter>
#include <QHBoxLayout>
#include <QStyleOptionSpinBox>

ModificationWidget::ModificationWidget(QWidget* parent)
    : QWidget(parent)
{
    auto horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setSpacing(2);
    horizontalLayout->setContentsMargins(2, 1, 2, 1);

    xLabel = new QLabel("X:", this);
    xLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
    horizontalLayout->addWidget(xLabel);

    xAxisModify = new DAVAFloat32SpinBox(this);
    xAxisModify->setMinimumSize(QSize(80, 0));
    xAxisModify->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    horizontalLayout->addWidget(xAxisModify);

    yLabel = new QLabel("Y:", this);
    yLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
    horizontalLayout->addWidget(yLabel);

    yAxisModify = new DAVAFloat32SpinBox(this);
    yAxisModify->setMinimumSize(QSize(80, 0));
    yAxisModify->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    horizontalLayout->addWidget(yAxisModify);

    zLabel = new QLabel("Z:", this);
    zLabel->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
    horizontalLayout->addWidget(zLabel);

    zAxisModify = new DAVAFloat32SpinBox(this);
    zAxisModify->setMinimumSize(QSize(80, 0));
    zAxisModify->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    horizontalLayout->addWidget(zAxisModify);

    connect(xAxisModify, &DAVAFloat32SpinBox::valueEdited, this, &ModificationWidget::OnXChanged);
    connect(yAxisModify, &DAVAFloat32SpinBox::valueEdited, this, &ModificationWidget::OnYChanged);
    connect(zAxisModify, &DAVAFloat32SpinBox::valueEdited, this, &ModificationWidget::OnZChanged);

    connect(SceneSignals::Instance(), &SceneSignals::Activated, this, &ModificationWidget::OnSceneActivated);
    connect(SceneSignals::Instance(), &SceneSignals::Deactivated, this, &ModificationWidget::OnSceneDeactivated);
    connect(SceneSignals::Instance(), &SceneSignals::CommandExecuted, this, &ModificationWidget::OnSceneCommand);

    selectionFieldBinder.reset(new DAVA::TArc::FieldBinder(REGlobal::GetAccessor()));
    {
        DAVA::TArc::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<SelectionData>();
        fieldDescr.fieldName = DAVA::FastName(SelectionData::selectionPropertyName);
        selectionFieldBinder->BindField(fieldDescr, [this](const DAVA::Any&) { ReloadValues(); });
    }
}

ModificationWidget::~ModificationWidget() = default;

void ModificationWidget::SetPivotMode(PivotMode mode)
{
    pivotMode = mode;
    ReloadValues();
}

void ModificationWidget::SetTransformType(Selectable::TransformType mode)
{
    modifMode = mode;
    ReloadValues();
}

void ModificationWidget::ReloadValues()
{
    xAxisModify->clear();
    yAxisModify->clear();
    zAxisModify->clear();

    if (modifMode == Selectable::TransformType::Scale)
    {
        xLabel->setText("Scale:");

        yLabel->setVisible(false);
        zLabel->setVisible(false);
        yAxisModify->setVisible(false);
        zAxisModify->setVisible(false);
    }
    else
    {
        xLabel->setText("X:");

        yLabel->setVisible(true);
        zLabel->setVisible(true);
        yAxisModify->setVisible(true);
        zAxisModify->setVisible(true);
    }

    if (curScene == nullptr)
    {
        setEnabled(false);
        return;
    }

    const SelectableGroup& modificationGroup = curScene->modifSystem->GetTransformableSelection();
    bool canModify = (modifMode != Selectable::TransformType::Disabled) && !modificationGroup.IsEmpty();
    if (canModify)
    {
        xAxisModify->showButtons(true);
        yAxisModify->showButtons(true);
        zAxisModify->showButtons(true);

        if (modificationGroup.GetSize() > 1)
        {
            groupMode = true;

            if (pivotMode == PivotRelative)
            {
                xAxisModify->setValue(0);
                yAxisModify->setValue(0);
                zAxisModify->setValue(0);
            }
            else
            {
                xAxisModify->showButtons(false);
                yAxisModify->showButtons(false);
                zAxisModify->showButtons(false);
                xAxisModify->clear();
                yAxisModify->clear();
                zAxisModify->clear();
            }
        }
        else
        {
            groupMode = false;

            if (pivotMode == PivotRelative)
            {
                xAxisModify->setValue(0);
                yAxisModify->setValue(0);
                zAxisModify->setValue(0);
            }
            else
            {
                const auto& firstObject = modificationGroup.GetFirst();
                DAVA::Matrix4 localMatrix = firstObject.GetLocalTransform();
                DAVA::float32 x = 0;
                DAVA::float32 y = 0;
                DAVA::float32 z = 0;
                switch (modifMode)
                {
                case Selectable::TransformType::Translation:
                {
                    DAVA::Vector3 translation = localMatrix.GetTranslationVector();
                    x = translation.x;
                    y = translation.y;
                    z = translation.z;
                }
                break;
                case Selectable::TransformType::Rotation:
                {
                    DAVA::Vector3 pos, scale, rotate;
                    if (localMatrix.Decomposition(pos, scale, rotate))
                    {
                        x = DAVA::RadToDeg(rotate.x);
                        y = DAVA::RadToDeg(rotate.y);
                        z = DAVA::RadToDeg(rotate.z);
                    }
                }
                break;
                case Selectable::TransformType::Scale:
                {
                    DAVA::Vector3 pos, scale, rotate;
                    if (localMatrix.Decomposition(pos, scale, rotate))
                    {
                        x = scale.x;
                        y = scale.y;
                        z = scale.z;
                    }
                }
                break;
                default:
                    break;
                }

                xAxisModify->setValue(x);
                yAxisModify->setValue(y);
                zAxisModify->setValue(z);
            }
        }
    }
    else
    {
        xAxisModify->showButtons(true);
        yAxisModify->showButtons(true);
        zAxisModify->showButtons(true);
        xAxisModify->clear();
        yAxisModify->clear();
        zAxisModify->clear();
    }

    xAxisModify->setEnabled(canModify);
    yAxisModify->setEnabled(canModify);
    zAxisModify->setEnabled(canModify);
    setEnabled(canModify);
}

void ModificationWidget::ApplyValues(ST_Axis axis)
{
    if (curScene == nullptr)
        return;

    DAVA::Vector3 values(xAxisModify->value(), yAxisModify->value(), zAxisModify->value());
    SelectableGroup selection = curScene->modifSystem->GetTransformableSelection();
    selection.RemoveObjectsWithDependantTransform();

    switch (modifMode)
    {
    case Selectable::TransformType::Translation:
    {
        curScene->modifSystem->ApplyMoveValues(axis, selection, values, pivotMode == PivotMode::PivotAbsolute);
        break;
    }

    case Selectable::TransformType::Rotation:
    {
        curScene->modifSystem->ApplyRotateValues(axis, selection, values, pivotMode == PivotMode::PivotAbsolute);
        break;
    }

    case Selectable::TransformType::Scale:
    {
        curScene->modifSystem->ApplyScaleValues(axis, selection, values, pivotMode == PivotMode::PivotAbsolute);
        break;
    }
    default:
        break;
    }

    ReloadValues();
}

void ModificationWidget::OnSceneCommand(SceneEditor2* scene, const RECommandNotificationObject& commandNotification)
{
    if (curScene == scene)
    {
        ReloadValues();
    }
}

void ModificationWidget::OnXChanged()
{
    ApplyValues(ST_AXIS_X);
}

void ModificationWidget::OnYChanged()
{
    ApplyValues(ST_AXIS_Y);
}

void ModificationWidget::OnZChanged()
{
    ApplyValues(ST_AXIS_Z);
}

void ModificationWidget::OnSceneActivated(SceneEditor2* scene)
{
    curScene = scene;
    ReloadValues();
}

void ModificationWidget::OnSceneDeactivated(SceneEditor2* scene)
{
    curScene = NULL;
    ReloadValues();
}

// =======================================================================================================
// ModificationSpin
// =======================================================================================================

DAVAFloat32SpinBox::DAVAFloat32SpinBox(QWidget* parent /* = 0 */)
    : QAbstractSpinBox(parent)
{
    QRegExp rx("^-?\\d+([\\.,]\\d+){0,1}$");
    QValidator* vd = new QRegExpValidator(rx, this);

    QLineEdit* le = lineEdit();
    le->setValidator(vd);

    connect(this, SIGNAL(editingFinished()), this, SLOT(textEditingFinished()));

    installEventFilter(this);
}

DAVA::float32 DAVAFloat32SpinBox::value() const
{
    return originalValue;
}

void DAVAFloat32SpinBox::showButtons(bool show)
{
    hasButtons = show;
}

void DAVAFloat32SpinBox::clear()
{
    setValue(0);
    QAbstractSpinBox::clear();
}

void DAVAFloat32SpinBox::setValue(DAVA::float32 val)
{
    if (originalValue != val || lineEdit()->text().isEmpty())
    {
        originalValue = val;
        lineEdit()->setText(QString::number((double)val, 'f', precision));
    }
}

bool DAVAFloat32SpinBox::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::Shortcut)
    {
        event->ignore();
        return true;
    }

    return QAbstractSpinBox::eventFilter(object, event);
}

void DAVAFloat32SpinBox::stepBy(int steps)
{
    textEditingFinished();
    setValue(originalValue + (DAVA::float32)steps);
    selectAll();

    emit valueEdited();
}

const DAVA::float32 DAVAFloat32SpinBox::eps = 0.001f;
void DAVAFloat32SpinBox::textEditingFinished()
{
    // was modified
    if (lineEdit()->isUndoAvailable())
    {
        QString newString = lineEdit()->text();
        newString.replace(QChar(','), QChar('.'));

        bool convertedNew = false;
        DAVA::float32 newValue = static_cast<DAVA::float32>(newString.toDouble(&convertedNew));

        // current double value is different from the original
        if (convertedNew && fabs(newValue - originalValue) > eps)
        {
            setValue(newValue);
            emit valueEdited();
        }
    }
}

void DAVAFloat32SpinBox::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        selectAll();
        event->ignore();
        emit editingFinished();
        return;

    default:
        QAbstractSpinBox::keyPressEvent(event);
    }
}

void DAVAFloat32SpinBox::paintEvent(QPaintEvent* event)
{
    QStyleOptionSpinBox opt;
    initStyleOption(&opt);

    // draw buttons disabled if they are
    opt.stepEnabled = stepEnabled();

    QStylePainter p(this);
    p.drawComplexControl(QStyle::CC_SpinBox, opt);
}

QAbstractSpinBox::StepEnabled DAVAFloat32SpinBox::stepEnabled() const
{
    if (hasButtons)
    {
        return (QAbstractSpinBox::StepDownEnabled | QAbstractSpinBox::StepUpEnabled);
    }

    return QAbstractSpinBox::StepNone;
}
