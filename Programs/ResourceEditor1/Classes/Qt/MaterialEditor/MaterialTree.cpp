#include "Classes/Qt/MaterialEditor/MaterialTree.h"
#include "Classes/Qt/MaterialEditor/MaterialFilterModel.h"
#include "Classes/Qt/MaterialEditor/MaterialAssignSystem.h"
#include "Classes/Qt/Main/mainwindow.h"
#include "Classes/Qt/Scene/SceneSignals.h"

#include "Classes/Application/REGlobal.h"
#include "Classes/Selection/Selection.h"
#include "Classes/Selection/SelectionData.h"

#include "Classes/Commands2/RemoveComponentCommand.h"
#include "Classes/Commands2/Base/RECommandBatch.h"
#include "Classes/Commands2/Base/RECommandNotificationObject.h"

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

    selectionFieldBinder.reset(new DAVA::TArc::FieldBinder(REGlobal::GetAccessor()));
    {
        DAVA::TArc::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<SelectionData>();
        fieldDescr.fieldName = DAVA::FastName(SelectionData::selectionPropertyName);
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

void MaterialTree::SetScene(SceneEditor2* sceneEditor)
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
    SceneEditor2* curScene = treeModel->GetScene();

    if (nullptr != curScene && materials.size() > 0)
    {
        SelectableGroup newSelection;
        std::function<void(DAVA::NMaterial*)> fn = [&fn, &curScene, &newSelection](DAVA::NMaterial* material)
        {
            DAVA::Entity* entity = curScene->materialSystem->GetEntity(material);
            if ((nullptr != entity) && (newSelection.ContainsObject(entity) == false && Selection::IsEntitySelectable(entity)))
            {
                newSelection.Add(entity, curScene->collisionSystem->GetUntransformedBoundingBox(entity));
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

        Selection::SetSelection(newSelection);
        LookAtSelection(curScene);
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

    contextMenu.addAction(DAVA::TArc::SharedIcon(":/QtIcons/zoom.png"), "Select entities", this, SLOT(OnSelectEntities()));

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

void MaterialTree::OnCommandExecuted(SceneEditor2* scene, const RECommandNotificationObject& commandNotification)
{
    if (treeModel->GetScene() == scene)
    {
        if (commandNotification.MatchCommandID(CMDID_INSP_MEMBER_MODIFY))
        {
            treeModel->invalidate();
        }
        else if (commandNotification.MatchCommandIDs({ CMDID_DELETE_RENDER_BATCH, CMDID_CLONE_LAST_BATCH, CMDID_CONVERT_TO_SHADOW, CMDID_MATERIAL_SWITCH_PARENT,
                                                       CMDID_MATERIAL_REMOVE_CONFIG, CMDID_MATERIAL_CREATE_CONFIG, CMDID_LOD_DELETE, CMDID_LOD_CREATE_PLANE, CMDID_LOD_COPY_LAST_LOD }))
        {
            Update();
        }
        else
        {
            auto processRemoveCommand = [this](const RECommand* command, bool redo)
            {
                if (command->MatchCommandID(CMDID_COMPONENT_REMOVE))
                {
                    const RemoveComponentCommand* removeCommand = static_cast<const RemoveComponentCommand*>(command);
                    DVASSERT(removeCommand->GetComponent() != nullptr);
                    if (removeCommand->GetComponent()->GetType() == DAVA::Type::Instance<DAVA::RenderComponent>())
                    {
                        Update();
                    }
                }
            };

            commandNotification.ExecuteForAllCommands(processRemoveCommand);
        }
    }
}

void MaterialTree::OnStructureChanged(SceneEditor2* scene, DAVA::Entity* parent)
{
    treeModel->Sync();
}

void MaterialTree::OnSelectionChanged(const DAVA::Any& selectionAny)
{
    if (selectionAny.CanGet<SelectableGroup>())
    {
        const SelectableGroup& selection = selectionAny.Get<SelectableGroup>();
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
