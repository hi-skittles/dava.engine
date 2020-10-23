#include "BasePropertyDelegate.h"

#include "Model/ControlProperties/RootProperty.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Modules/DataBindingInspectorModule/DataBindingInspectorModel.h"
#include <Engine/Engine.h>
#include <UI/UIControl.h>
#include <UI/Formula/FormulaContext.h>
#include <UI/UIControlSystem.h>
#include <UI/DataBinding/UIDataBindingSystem.h>
#include <UI/DataBinding/Private/UIDataModel.h>
#include <UI/DataBinding/UIDataBindingComponent.h>

#include "PropertiesModel.h"
#include "PropertiesTreeItemDelegate.h"
#include "Utils/QtDavaConvertion.h"

#include "TArc/Utils/Utils.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QAction>
#include <QTreeView>
#include <QDialog>
#include <QDialogButtonBox>

using namespace DAVA;

BasePropertyDelegate::BasePropertyDelegate(PropertiesTreeItemDelegate* delegate)
    : AbstractPropertyDelegate(delegate)
    , accessor(nullptr)
{
}

BasePropertyDelegate::~BasePropertyDelegate()
{
    itemDelegate = NULL;
}

bool BasePropertyDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (!BasePropertyDelegate::IsValueModified(editor))
        return true;

    BasePropertyDelegate::SetValueModified(editor, false);

    if (BasePropertyDelegate::IsValueReseted(editor))
    {
        BasePropertyDelegate::SetValueReseted(editor, false);
        if (model->setData(index, QVariant(), PropertiesModel::ResetRole))
        {
            return true;
        }
    }
    else if (BasePropertyDelegate::HasBindingExpression(editor))
    {
        QString expr = BasePropertyDelegate::GetBindingExpression(editor);
        BasePropertyDelegate::ResetBindingExpression(editor);

        QVariant prevBinding = model->data(index, PropertiesModel::BindingRole);
        QMap<QString, QVariant> map;
        map.insert("mode", UIDataBindingComponent::MODE_READ);
        if (prevBinding.type() == QVariant::Map)
        {
            QMap<QString, QVariant> prevMap = prevBinding.toMap();
            auto it = prevMap.find("mode");
            if (it != prevMap.end())
            {
                map.insert("mode", *it);
            }
        }
        map.insert("value", DAVA::EscapeString(expr));
        if (model->setData(index, map, PropertiesModel::BindingRole))
        {
            return true;
        }
    }

    return false;
}

void BasePropertyDelegate::enumEditorActions(QWidget* parent, const QModelIndex& index, QList<QAction*>& actions)
{
    AbstractProperty* property = static_cast<AbstractProperty*>(index.internalPointer());

    if (property && property->IsBindable())
    {
        QAction* bindAction = new QAction(QIcon(":/Icons/link.png"), tr("bind"), parent);
        QString expr;
        DataBindingInspectorModel* model = new DataBindingInspectorModel(true, parent);
        RootProperty* root = dynamic_cast<RootProperty*>(property->GetRootProperty());
        UIControl* control = nullptr;
        if (root)
        {
            ControlNode* controlNode = root->GetControlNode();
            control = controlNode->GetControl();
        }

        if (control)
        {
            std::shared_ptr<FormulaContext> context = DAVA::GetEngineContext()->uiControlSystem->GetSystem<UIDataBindingSystem>()->GetFormulaContext(control);
            model->UpdateModel(context.get());
        }
        expr = QString::fromStdString(property->GetBindingExpression());
        bindAction->setProperty("model", QVariant::fromValue(model));
        bindAction->setProperty("bindingExpr", QVariant(expr));
        bindAction->setToolTip(tr("Bind property to data."));
        actions.push_back(bindAction);
        connect(bindAction, SIGNAL(triggered(bool)), this, SLOT(bindClicked()));
    }

    if (property && property->GetFlags() & AbstractProperty::EF_CAN_RESET)
    {
        QAction* resetAction = new QAction(QIcon(":/Icons/edit_undo.png"), tr("reset"), parent);
        resetAction->setToolTip(tr("Reset property value to default."));
        actions.push_back(resetAction);
        connect(resetAction, SIGNAL(triggered(bool)), this, SLOT(resetClicked()));
    }
}

void BasePropertyDelegate::resetClicked()
{
    QAction* resetAction = qobject_cast<QAction*>(sender());
    if (!resetAction)
        return;

    QWidget* editor = resetAction->parentWidget();
    if (!editor)
        return;

    BasePropertyDelegate::SetValueReseted(editor, true);
    BasePropertyDelegate::SetValueModified(editor, true);
    BasePropertyDelegate::ResetBindingExpression(editor);
    itemDelegate->emitCommitData(editor);
    itemDelegate->emitCloseEditor(editor, QAbstractItemDelegate::EndEditHint::NoHint);
}

void BasePropertyDelegate::bindClicked()
{
    QAction* bindAction = qobject_cast<QAction*>(sender());
    if (!bindAction)
        return;

    QWidget* editor = bindAction->parentWidget();
    if (!editor)
        return;

    QString expr = bindAction->property("bindingExpr").toString();
    int32 bindingMode = bindAction->property("bindingMode").toInt();
    QVariant modelVariant = bindAction->property("model");

    DataBindingInspectorModel* model = modelVariant.value<DataBindingInspectorModel*>();
    QDialog* dialog = new QDialog(editor);
    dialog->setWindowTitle("Bind Data");
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    dialog->setLayout(layout);
    dialog->setModal(true);

    QTreeView* treeView = new QTreeView();
    treeView->setModel(model);
    treeView->expandAll();
    treeView->resizeColumnToContents(0);

    dialog->layout()->addWidget(treeView);

    QDialogButtonBox* buttons = new QDialogButtonBox(Qt::Horizontal);
    buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    dialog->layout()->addWidget(buttons);

    connect(buttons, SIGNAL(accepted()), dialog, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), dialog, SLOT(reject()));

    if (dialog->exec() == QDialog::Accepted)
    {
        QModelIndexList indices = treeView->selectionModel()->selectedIndexes();
        QString exprStr = "";
        if (!indices.empty())
        {
            exprStr = model->data(indices.first(), DataBindingInspectorModel::PATH_DATA).toString();
        }
        BasePropertyDelegate::SetBindingExpression(editor, exprStr);
        BasePropertyDelegate::SetValueModified(editor, true);
        itemDelegate->emitCommitData(editor);
    }
    else
    {
        BasePropertyDelegate::ResetBindingExpression(editor);
    }
    itemDelegate->emitCloseEditor(editor, QAbstractItemDelegate::EndEditHint::NoHint);
}
