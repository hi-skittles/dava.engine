#include "Classes/Qt/MaterialEditor/MaterialTree.h"
#include "Classes/Qt/MaterialEditor/MaterialFilterModel.h"
#include "Classes/Qt/Main/mainwindow.h"
#include "Classes/Qt/Scene/SceneSignals.h"

#include <REPlatform/Commands/CloneLastBatchCommand.h>
#include <REPlatform/Commands/ConvertToShadowCommand.h>
#include <REPlatform/Commands/CopyLastLODCommand.h>
#include <REPlatform/Commands/CreatePlaneLODCommand.h>
#include <REPlatform/Commands/DeleteLODCommand.h>
#include <REPlatform/Commands/DeleteRenderBatchCommand.h>
#include <REPlatform/Commands/InspMemberModifyCommand.h>
#include <REPlatform/Commands/MaterialConfigCommands.h>
#include <REPlatform/Commands/MaterialSwitchParentCommand.h>
#include <REPlatform/Commands/RECommandNotificationObject.h>
#include <REPlatform/Commands/RemoveComponentCommand.h>
#include <REPlatform/DataNodes/SelectableGroup.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/Scene/SceneHelper.h>
#include <REPlatform/Scene/Systems/CollisionSystem.h>
#include <REPlatform/Scene/Systems/EditorMaterialSystem.h>
#include <REPlatform/Scene/Systems/SelectionSystem.h>

#include <TArc/Core/Deprecated.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Utils/Utils.h>

#include <Entity/Component.h>

#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QHeaderView>

MaterialTree::MaterialTree(QWidget* parent /* = 0 */)
    : QTreeView(parent)
{
    treeModel = new MaterialFilteringModel(new MaterialModel(this));
    setModel(treeModel);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setIconSize(QSize(24, 24));

    QObject::connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(ShowContextMenu(const QPoint&)));

    QObject::connect(SceneSignals::Instance(), &SceneSignals::CommandExecuted, this, &MaterialTree::OnCommandExecuted);
    QObject::connect(SceneSignals::Instance(), &SceneSignals::StructureChanged, this, &MaterialTree::OnStructureChanged);

    selectionFieldBinder.reset(new DAVA::FieldBinder(DAVA::Deprecated::GetAccessor()));
    {
        DAVA::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<DAVA::SelectionData>();
        fieldDescr.fieldName = DAVA::FastName(DAVA::SelectionData::selectionPropertyName);
        selectionFieldBinder->BindField(fieldDescr, DAVA::MakeFunction(this, &MaterialTree::OnSelectionChanged));
    }

    header()->setSortIndicator(0, Qt::AscendingOrder);
    header()->setStretchLastSection(false);
    header()->setSectionResizeMode(0, QHeaderView::Stretch);
    header()->setSectionResizeMode(1, QHeaderView::Fixed);
    header()->setSectionResizeMode(2, QHeaderView::Fixed);
    header()->resizeSection(1, 35);
    header()->resizeSection(2, 35);
}

MaterialTree::~MaterialTree()
{
}

void MaterialTree::SetScene(DAVA::SceneEditor2* sceneEditor)
{
    setSortingEnabled(false);
    treeModel->SetScene(sceneEditor);

    sortByColumn(0);
    setSortingEnabled(true);
}

DAVA::NMaterial* MaterialTree::GetMaterial(const QModelIndex& index) const
{
    return treeModel->GetMaterial(index);
}

void MaterialTree::SelectMaterial(DAVA::NMaterial* material)
{
    selectionModel()->clear();

    QModelIndex index = treeModel->GetIndex(material);
    if (index.isValid())
    {
        selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        scrollTo(index, QAbstractItemView::PositionAtCenter);
    }
}

void MaterialTree::SelectEntities(const QList<DAVA::NMaterial*>& materials)
{
    DAVA::SceneEditor2* curScene = treeModel->GetScene();

    if (nullptr != curScene && materials.size() > 0)
    {
        DAVA::SelectableGroup newSelection;
        DAVA::EditorMaterialSystem* materialSystem = curScene->GetSystem<DAVA::EditorMaterialSystem>();
        DAVA::SceneCollisionSystem* collisionSystem = curScene->GetSystem<DAVA::SceneCollisionSystem>();
        DAVA::SelectionSystem* selectionSystem = curScene->GetSystem<DAVA::SelectionSystem>();
        std::function<void(DAVA::NMaterial*)> fn = [&](DAVA::NMaterial* material)
        {
            DAVA::Entity* entity = materialSystem->GetEntity(material);
            if ((nullptr != entity) && (newSelection.ContainsObject(entity) == false && selectionSystem->IsEntitySelectable(entity)))
            {
                newSelection.Add(entity, collisionSystem->GetUntransformedBoundingBox(entity));
            }

            const DAVA::Vector<DAVA::NMaterial*>& children = material->GetChildren();
            for (DAVA::NMaterial* child : children)
            {
                fn(child);
            }
        };

        for (int i = 0; i < materials.size(); i++)
        {
            DAVA::NMaterial* material = materials.at(i);
            fn(material);
        }

        selectionSystem->SetSelection(newSelection);
        DAVA::LookAtSelection(curScene);
    }
}

void MaterialTree::Update()
{
    treeModel->Sync();
    treeModel->invalidate();
    emit Updated();
}

int MaterialTree::getFilterType() const
{
    return treeModel->getFilterType();
}

void MaterialTree::setFilterType(int filterType)
{
    treeModel->setFilterType(filterType);
}

void MaterialTree::ShowContextMenu(const QPoint& pos)
{
    QMenu contextMenu(this);

    contextMenu.addAction(DAVA::SharedIcon(":/QtIcons/zoom.png"), "Select entities", this, SLOT(OnSelectEntities()));

    emit ContextMenuPrepare(&contextMenu);
    contextMenu.exec(mapToGlobal(pos));
}

void MaterialTree::dragEnterEvent(QDragEnterEvent* event)
{
    QTreeView::dragEnterEvent(event);
    dragTryAccepted(event);
}

void MaterialTree::dragMoveEvent(QDragMoveEvent* event)
{
    QTreeView::dragMoveEvent(event);
    dragTryAccepted(event);
}

void MaterialTree::dropEvent(QDropEvent* event)
{
    QTreeView::dropEvent(event);

    event->setDropAction(Qt::IgnoreAction);
    event->accept();
}

void MaterialTree::dragTryAccepted(QDragMoveEvent* event)
{
    int row, col;
    QModelIndex parent;

    GetDropParams(event->pos(), parent, row, col);
    if (treeModel->dropCanBeAccepted(event->mimeData(), event->dropAction(), row, col, parent))
    {
        event->setDropAction(Qt::MoveAction);
        event->accept();
        treeModel->invalidate();
    }
    else
    {
        event->setDropAction(Qt::IgnoreAction);
        event->accept();
    }
}

void MaterialTree::GetDropParams(const QPoint& pos, QModelIndex& index, int& row, int& col)
{
    row = -1;
    col = -1;
    index = indexAt(pos);

    switch (dropIndicatorPosition())
    {
    case QAbstractItemView::OnItem:
    case QAbstractItemView::AboveItem:
        row = index.row();
        col = index.column();
        index = index.parent();
        break;
    case QAbstractItemView::BelowItem:
        row = index.row() + 1;
        col = index.column();
        index = index.parent();
        break;
    case QAbstractItemView::OnViewport:
        index = QModelIndex();
        break;
    }
}

void MaterialTree::OnCommandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification)
{
    using namespace DAVA;
    if (treeModel->GetScene() == scene)
    {
        if (commandNotification.MatchCommandTypes<InspMemberModifyCommand>())
        {
            treeModel->invalidate();
        }

        if (commandNotification.MatchCommandTypes<DeleteRenderBatchCommand, CloneLastBatchCommand, ConvertToShadowCommand, MaterialSwitchParentCommand,
                                                  MaterialRemoveConfig, MaterialCreateConfig, DeleteLODCommand, CreatePlaneLODCommand, CopyLastLODToLod0Command>())
        {
            Update();
        }
        else
        {
            commandNotification.ForEach<RemoveComponentCommand>([&](const RemoveComponentCommand* cmd) {
                DVASSERT(cmd->GetComponent() != nullptr);
                if (cmd->GetComponent()->GetType() == DAVA::Type::Instance<DAVA::RenderComponent>())
                {
                    Update();
                }
            });
        }
    }
}

void MaterialTree::OnStructureChanged(DAVA::SceneEditor2* scene, DAVA::Entity* parent)
{
    treeModel->Sync();
}

void MaterialTree::OnSelectionChanged(const DAVA::Any& selectionAny)
{
    if (selectionAny.CanGet<DAVA::SelectableGroup>())
    {
        const DAVA::SelectableGroup& selection = selectionAny.Get<DAVA::SelectableGroup>();
        treeModel->SetSelection(&selection);
        treeModel->invalidate();
    }
}

void MaterialTree::OnSelectEntities()
{
    const QModelIndexList selection = selectionModel()->selectedRows();
    QList<DAVA::NMaterial*> materials;

    materials.reserve(selection.size());
    for (int i = 0; i < selection.size(); i++)
    {
        DAVA::NMaterial* material = treeModel->GetMaterial(selection.at(i));
        if (material != nullptr)
            materials << material;
    }

    SelectEntities(materials);
}
