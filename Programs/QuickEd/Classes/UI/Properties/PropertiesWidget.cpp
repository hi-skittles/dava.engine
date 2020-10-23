#include "ui_PropertiesWidget.h"
#include "UI/Properties/PropertiesWidget.h"
#include "UI/Properties/PropertiesModel.h"

#include "Modules/DocumentsModule/DocumentData.h"
#include "Modules/LegacySupportModule/Private/Project.h"

#include "UI/Properties/PropertiesTreeItemDelegate.h"
#include "UI/CommandExecutor.h"
#include "Model/ControlProperties/ComponentPropertiesSection.h"
#include "Model/ControlProperties/StyleSheetProperty.h"
#include "Model/ControlProperties/StyleSheetSelectorProperty.h"
#include "Model/ControlProperties/RootProperty.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "Utils/QtDavaConvertion.h"

#include <TArc/WindowSubSystem/UI.h>
#include <TArc/Core/ContextAccessor.h>

#include <UI/Components/UIComponent.h>
#include <UI/Components/UIComponentUtils.h>
#include <UI/UIControl.h>
#include <UI/Styles/UIStyleSheetPropertyDataBase.h>
#include <Engine/Engine.h>
#include <Entity/ComponentManager.h>

#include <QAbstractItemModel>
#include <QItemEditorFactory>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QItemSelection>

using namespace DAVA;

namespace
{
String GetPathFromIndex(QModelIndex index)
{
    QString path = index.data().toString();
    while (index.parent().isValid())
    {
        index = index.parent();
        path += "/" + index.data().toString();
    }
    return path.toStdString();
}
}

PropertiesWidget::PropertiesWidget(QWidget* parent)
    : QDockWidget(parent)
    , nodeUpdater(300)
{
    setupUi(this);

    nodeUpdater.SetUpdater(MakeFunction(this, &PropertiesWidget::UpdateModelInternal));
    nodeUpdater.SetStopper([this]() { return selectedNode == nullptr; });

    propertiesModel = new PropertiesModel(treeView);
    propertiesItemsDelegate = new PropertiesTreeItemDelegate(this);
    treeView->setModel(propertiesModel);
    connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &PropertiesWidget::OnSelectionChanged);
    connect(propertiesModel, &PropertiesModel::ComponentAdded, this, &PropertiesWidget::OnComponentAdded);

    treeView->setItemDelegate(propertiesItemsDelegate);

    addComponentAction = CreateAddComponentAction();
    treeView->addAction(addComponentAction);

    addStylePropertyAction = CreateAddStylePropertyAction();
    treeView->addAction(addStylePropertyAction);

    addStyleSelectorAction = CreateAddStyleSelectorAction();
    treeView->addAction(addStyleSelectorAction);

    treeView->addAction(CreateSeparator());

    removeAction = CreateRemoveAction();
    treeView->addAction(removeAction);

    connect(treeView, &QTreeView::expanded, this, &PropertiesWidget::OnExpanded);
    connect(treeView, &QTreeView::collapsed, this, &PropertiesWidget::OnCollapsed);
    DVASSERT(nullptr == selectedNode);
    UpdateModel(nullptr);
}

PropertiesWidget::~PropertiesWidget()
{
    nodeUpdater.Abort();
}

void PropertiesWidget::SetAccessor(DAVA::ContextAccessor* accessor_)
{
    accessor = accessor_;

    documentDataWrapper = accessor->CreateWrapper(DAVA::ReflectedTypeDB::Get<DocumentData>());
    documentDataWrapper.SetListener(this);

    propertiesModel->SetAccessor(accessor);
    propertiesItemsDelegate->SetAccessor(accessor);
}

void PropertiesWidget::SetUI(DAVA::UI* ui_)
{
    ui = ui_;
}

void PropertiesWidget::SetInvoker(DAVA::OperationInvoker* invoker_)
{
    invoker = invoker_;
    propertiesItemsDelegate->SetInvoker(invoker_);
}

void PropertiesWidget::SetProject(const Project* project)
{
    propertiesItemsDelegate->SetProject(project);
}

void PropertiesWidget::OnAddComponent(QAction* action)
{
    CommandExecutor executor(accessor, ui);
    DVASSERT(accessor->GetActiveContext() != nullptr);
    const RootProperty* rootProperty = DAVA::DynamicTypeCheck<const RootProperty*>(propertiesModel->GetRootProperty());

    const Type* componentType = action->data().value<Any>().Cast<const Type*>();
    ComponentPropertiesSection* componentSection = rootProperty->FindComponentPropertiesSection(componentType, 0);
    if (componentSection != nullptr && !UIComponentUtils::IsMultiple(componentType))
    {
        QModelIndex index = propertiesModel->indexByProperty(componentSection);
        OnComponentAdded(index);
    }
    else
    {
        executor.AddComponent(DynamicTypeCheck<ControlNode*>(selectedNode), componentType);
    }
}

void PropertiesWidget::OnRemove()
{
    CommandExecutor executor(accessor, ui);
    DVASSERT(accessor->GetActiveContext() != nullptr);

    QModelIndexList indices = treeView->selectionModel()->selectedIndexes();
    if (!indices.empty())
    {
        const QModelIndex& index = indices.first();
        AbstractProperty* property = static_cast<AbstractProperty*>(index.internalPointer());

        if ((property->GetFlags() & AbstractProperty::EF_CAN_REMOVE) != 0)
        {
            ComponentPropertiesSection* section = dynamic_cast<ComponentPropertiesSection*>(property);
            if (section)
            {
                executor.RemoveComponent(DynamicTypeCheck<ControlNode*>(selectedNode), section->GetComponentType(), section->GetComponentIndex());
            }
            else
            {
                StyleSheetProperty* styleProperty = dynamic_cast<StyleSheetProperty*>(property);
                if (styleProperty)
                {
                    executor.RemoveStyleProperty(DynamicTypeCheck<StyleSheetNode*>(selectedNode), styleProperty->GetPropertyIndex());
                }
                else
                {
                    StyleSheetSelectorProperty* selectorProperty = dynamic_cast<StyleSheetSelectorProperty*>(property);
                    if (selectorProperty)
                    {
                        int32 index = property->GetParent()->GetIndex(selectorProperty);
                        if (index != -1)
                        {
                            executor.RemoveStyleSelector(DynamicTypeCheck<StyleSheetNode*>(selectedNode), index);
                        }
                    }
                }
            }
        }
    }
    UpdateActions();
}

void PropertiesWidget::OnAddStyleProperty(QAction* action)
{
    CommandExecutor executor(accessor, ui);
    DVASSERT(accessor->GetActiveContext() != nullptr);

    uint32 propertyIndex = action->data().toUInt();
    if (propertyIndex < UIStyleSheetPropertyDataBase::STYLE_SHEET_PROPERTY_COUNT)
    {
        executor.AddStyleProperty(DynamicTypeCheck<StyleSheetNode*>(selectedNode), propertyIndex);
    }
    else
    {
        DVASSERT(propertyIndex < UIStyleSheetPropertyDataBase::STYLE_SHEET_PROPERTY_COUNT);
    }
}

void PropertiesWidget::OnAddStyleSelector()
{
    CommandExecutor executor(accessor, ui);
    DVASSERT(accessor->GetActiveContext() != nullptr);
    executor.AddStyleSelector(DynamicTypeCheck<StyleSheetNode*>(selectedNode));
}

void PropertiesWidget::OnSelectionChanged(const QItemSelection& /*selected*/, const QItemSelection& /*deselected*/)
{
    UpdateActions();
}

QAction* PropertiesWidget::CreateAddComponentAction()
{
    QMenu* addComponentMenu = new QMenu(this);
    const Vector<const Type*>& components = GetEngineContext()->componentManager->GetRegisteredUIComponents();
    for (const Type* c : components)
    {
        if (!UIComponentUtils::IsHidden(c))
        {
            String name = UIComponentUtils::GetDisplayName(c);
            QAction* componentAction = new QAction(name.c_str(), this); // TODO: Localize name
            componentAction->setData(QVariant::fromValue(Any(c)));
            addComponentMenu->addAction(componentAction);
        }
    }
    connect(addComponentMenu, &QMenu::triggered, this, &PropertiesWidget::OnAddComponent);

    QAction* action = new QAction(tr("Add Component"), this);
    action->setMenu(addComponentMenu);
    addComponentMenu->setEnabled(false);
    return action;
}

QAction* PropertiesWidget::CreateAddStyleSelectorAction()
{
    QAction* action = new QAction(tr("Add Style Selector"), this);
    connect(action, &QAction::triggered, this, &PropertiesWidget::OnAddStyleSelector);
    action->setEnabled(false);
    return action;
}

QAction* PropertiesWidget::CreateAddStylePropertyAction()
{
    QMenu* propertiesMenu = new QMenu(this);
    QMenu* groupMenu = nullptr;
    UIStyleSheetPropertyGroup* prevGroup = nullptr;
    UIStyleSheetPropertyDataBase* db = UIStyleSheetPropertyDataBase::Instance();
    for (int32 i = 0; i < UIStyleSheetPropertyDataBase::STYLE_SHEET_PROPERTY_COUNT; i++)
    {
        const UIStyleSheetPropertyDescriptor& descr = db->GetStyleSheetPropertyByIndex(i);
        if (descr.group != prevGroup)
        {
            prevGroup = descr.group;
            if (descr.group->prefix.empty())
            {
                groupMenu = propertiesMenu;
            }
            else
            {
                groupMenu = new QMenu(QString::fromStdString(descr.group->prefix), this);
                propertiesMenu->addMenu(groupMenu);
            }
        }
        QAction* componentAction = new QAction(descr.name.c_str(), this);
        componentAction->setData(i);

        groupMenu->addAction(componentAction);
    }
    connect(propertiesMenu, &QMenu::triggered, this, &PropertiesWidget::OnAddStyleProperty);

    QAction* action = new QAction(tr("Add Style Property"), this);
    action->setMenu(propertiesMenu);
    propertiesMenu->setEnabled(false);
    return action;
}

QAction* PropertiesWidget::CreateRemoveAction()
{
    QAction* action = new QAction(tr("Remove"), this);
    connect(action, &QAction::triggered, this, &PropertiesWidget::OnRemove);
    action->setEnabled(false);
    return action;
}

QAction* PropertiesWidget::CreateSeparator()
{
    QAction* separator = new QAction(this);
    separator->setSeparator(true);
    return separator;
}

void PropertiesWidget::OnModelUpdated()
{
    bool blocked = treeView->blockSignals(true);
    treeView->expandToDepth(0);
    treeView->blockSignals(blocked);
    treeView->resizeColumnToContents(0);
    ApplyExpanding();
}

void PropertiesWidget::OnExpanded(const QModelIndex& index)
{
    itemsState[GetPathFromIndex(index)] = true;
}

void PropertiesWidget::OnCollapsed(const QModelIndex& index)
{
    itemsState[GetPathFromIndex(index)] = false;
}

void PropertiesWidget::OnComponentAdded(const QModelIndex& index)
{
    treeView->expand(index);
    treeView->setCurrentIndex(index);
    int rowCount = propertiesModel->rowCount(index);
    if (rowCount > 0)
    {
        QModelIndex lastChildIndex = index.child(rowCount - 1, 0);
        treeView->scrollTo(lastChildIndex, QAbstractItemView::EnsureVisible);
    }
    treeView->scrollTo(index, QAbstractItemView::EnsureVisible);
}

void PropertiesWidget::UpdateModel(PackageBaseNode* node)
{
    if (node == selectedNode)
    {
        return;
    }
    selectedNode = node;
    nodeUpdater.Update();
}

void PropertiesWidget::UpdateModelInternal()
{
    if (nullptr != selectedNode)
    {
        auto index = treeView->indexAt(QPoint(0, 0));
        lastTopIndexPath = GetPathFromIndex(index);
    }
    propertiesModel->Reset(selectedNode);
    bool isControl = dynamic_cast<ControlNode*>(selectedNode) != nullptr;
    bool isStyle = dynamic_cast<StyleSheetNode*>(selectedNode) != nullptr;
    addComponentAction->menu()->setEnabled(isControl);
    addStylePropertyAction->menu()->setEnabled(isStyle);
    addStyleSelectorAction->setEnabled(isStyle);
    removeAction->setEnabled(false);

    OnModelUpdated();
}

void PropertiesWidget::UpdateActions()
{
    QModelIndexList indices = treeView->selectionModel()->selectedIndexes();
    if (!indices.empty())
    {
        AbstractProperty* property = static_cast<AbstractProperty*>(indices.first().internalPointer());
        removeAction->setEnabled((property->GetFlags() & AbstractProperty::EF_CAN_REMOVE) != 0);
    }
}

void PropertiesWidget::ApplyExpanding()
{
    QModelIndex index = propertiesModel->index(0, 0);
    while (index.isValid())
    {
        const auto& path = GetPathFromIndex(index);
        if (path == lastTopIndexPath)
        {
            treeView->scrollTo(index, QTreeView::PositionAtTop);
        }
        auto iter = itemsState.find(path);
        if (iter != itemsState.end())
        {
            treeView->setExpanded(index, iter->second);
        }

        index = treeView->indexBelow(index);
    }
}

void PropertiesWidget::OnDataChanged(const DAVA::DataWrapper& wrapper, const DAVA::Vector<DAVA::Any>& fields)
{
    using namespace DAVA;

    bool hasData = wrapper.HasData();
    treeView->setEnabled(hasData);
    if (hasData == false)
    {
        UpdateModel(nullptr);
        return;
    }

    bool currentNodeWasChanged = fields.empty() || std::find(fields.begin(), fields.end(), DocumentData::currentNodePropertyName) != fields.end();
    bool packageWasChanged = fields.empty() || std::find(fields.begin(), fields.end(), DocumentData::packagePropertyName) != fields.end();

    if (packageWasChanged)
    {
        //clear last cached value because next selected value will be delayed
        UpdateModel(nullptr);
    }

    if (currentNodeWasChanged)
    {
        Any currentNodeValue = wrapper.GetFieldValue(DocumentData::currentNodePropertyName);
        if (currentNodeValue.CanGet<PackageBaseNode*>())
        {
            PackageBaseNode* currentNode = currentNodeValue.Get<PackageBaseNode*>();
            if (currentNode != nullptr)
            {
                PackageBaseNode* parent = currentNode->GetParent();
                if (parent != nullptr && parent->GetParent() != nullptr)
                {
                    UpdateModel(currentNode);
                    return;
                }
            }
        }
        if (packageWasChanged == false)
        {
            UpdateModel(nullptr);
        }
    }
}
