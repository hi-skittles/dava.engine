#include "FilePathPropertyDelegate.h"
#include <QLineEdit>
#include <QApplication>
#include "DAVAEngine.h"
#include "Engine/Engine.h"

#include "Utils/QtDavaConvertion.h"
#include "PropertiesTreeItemDelegate.h"

FilePathPropertyDelegate::FilePathPropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : BasePropertyDelegate(delegate)
{
}

FilePathPropertyDelegate::~FilePathPropertyDelegate()
{
}

QWidget* FilePathPropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem&, const QModelIndex&)
{
    lineEdit = new QLineEdit(parent);
    lineEdit->setObjectName(QString::fromUtf8("lineEdit"));
    connect(lineEdit, &QLineEdit::editingFinished, this, &FilePathPropertyDelegate::OnEditingFinished);
    connect(lineEdit, &QLineEdit::textChanged, this, &FilePathPropertyDelegate::OnTextChanged);
    return lineEdit;
}

void FilePathPropertyDelegate::setEditorData(QWidget*, const QModelIndex& index) const
{
    DAVA::Any variant = index.data(Qt::EditRole).value<DAVA::Any>();
    QString stringValue = StringToQString(variant.Get<DAVA::FilePath>().GetStringValue());
    DVASSERT(!lineEdit.isNull());
    lineEdit->setText(stringValue);
}

bool FilePathPropertyDelegate::setModelData(QWidget* rawEditor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(rawEditor, model, index))
        return true;
    DVASSERT(!lineEdit.isNull());
    DAVA::Any value = index.data(Qt::EditRole).value<DAVA::Any>();
    DAVA::String str = QStringToString(lineEdit->text());
    DAVA::FilePath filePath = str;
    value.Set(filePath);

    QVariant variant;
    variant.setValue<DAVA::Any>(value);

    return model->setData(index, variant, Qt::EditRole);
}

void FilePathPropertyDelegate::OnEditingFinished()
{
    DVASSERT(!lineEdit.isNull());
    QWidget* editor = lineEdit->parentWidget();
    DVASSERT(nullptr != editor);
    if (!IsPathValid(lineEdit->text()))
    {
        return;
    }
    BasePropertyDelegate::SetValueModified(editor, lineEdit->isModified());
    itemDelegate->emitCommitData(editor);
}

void FilePathPropertyDelegate::OnTextChanged(const QString& text)
{
    QPalette palette(lineEdit->palette());
    QString textCopy(text);

    QColor globalTextColor = qApp->palette().color(QPalette::Text);
    QColor nextColor = IsPathValid(text) ? globalTextColor : Qt::red;
    palette.setColor(QPalette::Text, nextColor);
    lineEdit->setPalette(palette);
}

bool FilePathPropertyDelegate::IsPathValid(const QString& path) const
{
    DAVA::FilePath filePath(QStringToString(path));
    DAVA::FileSystem* fileSystem = DAVA::Engine::Instance()->GetContext()->fileSystem;
    return fileSystem->Exists(filePath);
}
