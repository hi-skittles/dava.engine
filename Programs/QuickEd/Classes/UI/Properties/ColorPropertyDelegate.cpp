#include "ColorPropertyDelegate.h"

#include <TArc/Utils/Utils.h>

#include <QToolButton>
#include <QPainter>
#include <QHBoxLayout>
#include <QLabel>
#include <QAction>
#include <QColorDialog>
#include <QApplication>

#include "PropertiesTreeItemDelegate.h"
#include "PropertiesModel.h"
#include "Utils/QtDavaConvertion.h"

ColorPropertyDelegate::ColorPropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

ColorPropertyDelegate::~ColorPropertyDelegate()
{
}

QWidget* ColorPropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    lineEdit = new QLineEdit(parent); //will be deleted outside this class
    lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
    QRegExpValidator* validator = new QRegExpValidator();
    validator->setRegExp(QRegExp("#?([A-F0-9]{8}|[A-F0-9]{6})", Qt::CaseInsensitive));
    lineEdit->setValidator(validator);

    connect(lineEdit, &QLineEdit::editingFinished, this, &ColorPropertyDelegate::OnEditingFinished);
    connect(lineEdit, &QLineEdit::textChanged, this, &ColorPropertyDelegate::OnTextChanged);
    return lineEdit;
}

void ColorPropertyDelegate::enumEditorActions(QWidget* parent, const QModelIndex& index, QList<QAction*>& actions)
{
    BasePropertyDelegate::enumEditorActions(parent, index, actions);

    chooseColorAction = new QAction(parent); //will be deleted outside this class
    connect(chooseColorAction, SIGNAL(triggered(bool)), this, SLOT(OnChooseColorClicked()));
    actions.push_front(chooseColorAction);
}

void ColorPropertyDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    DVASSERT(nullptr != lineEdit);
    QColor color = DAVA::ColorToQColor(index.data(Qt::EditRole).value<DAVA::Any>().Get<DAVA::Color>());
    lineEdit->setText(QColorToHex(color));
    lineEdit->setProperty("color", color);
}

bool ColorPropertyDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(editor, model, index))
        return true;

    DVASSERT(nullptr != lineEdit);

    QColor newColor = HexToQColor(lineEdit->text());
    DAVA::Any color(DAVA::QColorToColor(newColor));
    QVariant colorVariant;
    colorVariant.setValue<DAVA::Any>(color);
    return model->setData(index, colorVariant, Qt::EditRole);
}

void ColorPropertyDelegate::OnChooseColorClicked()
{
    DVASSERT(nullptr != lineEdit);
    QWidget* editor = lineEdit->parentWidget();
    DVASSERT(nullptr != editor);

    QColorDialog dlg(editor);

    dlg.setOptions(QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
    dlg.setCurrentColor(lineEdit->property("color").value<QColor>());

    if (dlg.exec() == QDialog::Accepted)
    {
        lineEdit->setText(QColorToHex(dlg.selectedColor()));
        lineEdit->setProperty("color", dlg.selectedColor());
        BasePropertyDelegate::SetValueModified(editor, true);
        itemDelegate->emitCommitData(editor);
    }
}

void ColorPropertyDelegate::OnEditingFinished()
{
    DVASSERT(nullptr != lineEdit);
    QWidget* editor = lineEdit->parentWidget();
    DVASSERT(nullptr != editor);

    BasePropertyDelegate::SetValueModified(editor, lineEdit->isModified());
    itemDelegate->emitCommitData(editor);
}

void ColorPropertyDelegate::OnTextChanged(const QString& text)
{
    QPalette palette(lineEdit->palette());
    int pos = -1;
    QString textCopy(text);
    bool valid = lineEdit->validator()->validate(textCopy, pos) == QValidator::Acceptable;
    QColor globalTextColor = qApp->palette().color(QPalette::Text);
    QColor nextColor = valid ? globalTextColor : Qt::red;
    palette.setColor(QPalette::Text, nextColor);
    lineEdit->setPalette(palette);

    if (valid)
    {
        QColor color(HexToQColor(text));
        chooseColorAction->setIcon(DAVA::CreateIconFromColor(color));
    }
}
