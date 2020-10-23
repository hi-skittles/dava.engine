#include "ComboPropertyDelegate.h"
#include <QComboBox>
#include <QLayout>
#include <QKeyEvent>
#include "Model/ControlProperties/AbstractProperty.h"
#include "PropertiesTreeItemDelegate.h"
#include "Utils/QtDavaConvertion.h"
#include "PropertiesModel.h"

ComboPropertyDelegate::ComboPropertyDelegate(PropertiesTreeItemDelegate* delegate, std::unique_ptr<CompletionsProvider> completionsProvider_, bool isEditable)
    : BasePropertyDelegate(delegate)
    , completionsProvider(std::move(completionsProvider_))
    , isEditable(isEditable)
{
}

ComboPropertyDelegate::~ComboPropertyDelegate()
{
}

QWidget* ComboPropertyDelegate::createEditor(QWidget* parent, const PropertiesContext& context, const QStyleOptionViewItem& option, const QModelIndex& index)
{
    QComboBox* comboBox = new QComboBox(parent);
    comboBox->setObjectName("comboBox");
    comboBox->setEditable(isEditable);
    comboBox->installEventFilter(this);

    AbstractProperty* property = static_cast<AbstractProperty*>(index.internalPointer());

    comboBox->clear();
    comboBox->addItem("");
    comboBox->addItems(completionsProvider->GetCompletions(property));

    connect(comboBox, SIGNAL(activated(const QString&)), this, SLOT(OnActivated(const QString&)));

    return comboBox;
}

void ComboPropertyDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QComboBox* comboBox = editor->findChild<QComboBox*>("comboBox");

    {
        const QSignalBlocker blocker(comboBox);
        int comboIndex = comboBox->findText(index.data(Qt::DisplayRole).toString());
        comboBox->setCurrentIndex(comboIndex);
        comboBox->setEditText(index.data(Qt::DisplayRole).toString());
    }

    BasePropertyDelegate::SetValueModified(editor, false);
}

bool ComboPropertyDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (BasePropertyDelegate::setModelData(editor, model, index))
        return true;

    QComboBox* comboBox = editor->findChild<QComboBox*>("comboBox");
    return model->setData(index, comboBox->currentText(), Qt::EditRole);
}

void ComboPropertyDelegate::OnCurrentIndexChanged()
{
    QWidget* comboBox = qobject_cast<QWidget*>(sender());
    if (!comboBox)
        return;

    if (comboBox != nullptr)
    {
        CommitDataImpl(comboBox);
    }
}

void ComboPropertyDelegate::OnActivated(const QString& text)
{
    QWidget* comboBox = qobject_cast<QWidget*>(sender());
    if (comboBox != nullptr)
    {
        CommitDataImpl(comboBox);
    }
}

bool ComboPropertyDelegate::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter)
        {
            CommitDataImpl(static_cast<QWidget*>(obj));
        }
    }
    return QObject::eventFilter(obj, event);
}

void ComboPropertyDelegate::CommitDataImpl(QWidget* comboBox)
{
    QWidget* editor = comboBox->parentWidget();
    if (!editor)
        return;

    BasePropertyDelegate::SetValueModified(editor, true);
    itemDelegate->emitCommitData(editor);
}
