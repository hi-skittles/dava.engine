#include "MaterialFilterModel.h"
#include "MaterialModel.h"
#include "MaterialItem.h"

#include "Scene/SceneEditor2.h"
#include "Commands2/MaterialSwitchParentCommand.h"

#include "Scene3D/Scene.h"

#include <QTimer>
#include <QDebug>

namespace MFMLocal
{
int CompareNames(MaterialItem* left, MaterialItem* right)
{
    DAVA::NMaterial* leftMaterial = left->GetMaterial();
    DAVA::NMaterial* rightMaterial = right->GetMaterial();
    const QString lhsText = QString(leftMaterial->GetMaterialName().c_str());
    const QString rhsText = QString(rightMaterial->GetMaterialName().c_str());
    return lhsText.compare(rhsText, Qt::CaseInsensitive);
}

int CompareLods(MaterialItem* left, MaterialItem* right)
{
    return left->GetLodIndex() - right->GetLodIndex();
}

int CompareSwitches(MaterialItem* left, MaterialItem* right)
{
    return left->GetSwitchIndex() - right->GetSwitchIndex();
}

bool Less(int value, int refValue)
{
    return value < refValue;
}

bool Greater(int value, int refValue)
{
    return value > refValue;
}
}

MaterialFilteringModel::MaterialFilteringModel(MaterialModel* _materialModel, QObject* parent /* = NULL */)
    : QSortFilterProxyModel(parent)
    , materialModel(_materialModel)
    , filterType(SHOW_ALL)
{
    setSourceModel(materialModel);
}

void MaterialFilteringModel::Sync()
{
    materialModel->Sync();
}

void MaterialFilteringModel::SetScene(SceneEditor2* scene)
{
    materialModel->SetScene(scene);
}

SceneEditor2* MaterialFilteringModel::GetScene()
{
    return materialModel->GetScene();
}

void MaterialFilteringModel::SetSelection(const SelectableGroup* group)
{
    materialModel->SetSelection(group);
}

DAVA::NMaterial* MaterialFilteringModel::GetMaterial(const QModelIndex& index) const
{
    return materialModel->GetMaterial(mapToSource(index));
}

QModelIndex MaterialFilteringModel::GetIndex(DAVA::NMaterial* material, const QModelIndex& parent /*= QModelIndex()*/) const
{
    return mapFromSource(materialModel->GetIndex(material, mapFromSource(parent)));
}

bool MaterialFilteringModel::dropCanBeAccepted(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    QModelIndex target = mapToSource(index(row, column, parent));
    return materialModel->dropCanBeAccepted(data, action, target.row(), target.column(), target.parent());
}

void MaterialFilteringModel::setFilterType(int type)
{
    if (type == filterType)
        return;

    filterType = static_cast<eFilterType>(type);
    invalidate();
    //invalidateFilter();
}

int MaterialFilteringModel::getFilterType() const
{
    return filterType;
}

bool MaterialFilteringModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    MaterialItem* item = (MaterialItem*)materialModel->itemFromIndex(materialModel->index(sourceRow, 0, sourceParent));
    if (item == nullptr)
    {
        return false;
    }

    DAVA::NMaterial* material = item->GetMaterial();
    bool isSelected = item->GetFlag(MaterialItem::IS_PART_OF_SELECTION);
    bool isTopLevelMaterial = (nullptr == material->GetParent()) || (materialModel->GetGlobalMaterial() == material->GetParent());

    switch (filterType)
    {
    case SHOW_ALL:
        return true;

    case SHOW_NOTHING:
        return false;

    case SHOW_INSTANCES_AND_MATERIALS:
        return isTopLevelMaterial || isSelected;

    case SHOW_ONLY_INSTANCES:
    {
        if (isSelected)
            return true;

        if (isTopLevelMaterial)
        {
            const int n = item->rowCount();
            for (int i = 0; i < n; i++)
            {
                MaterialItem* childItem = static_cast<MaterialItem*>(item->child(i));
                DVASSERT(childItem != nullptr);
                if (childItem->GetFlag(MaterialItem::IS_PART_OF_SELECTION))
                {
                    return true;
                }
            }
        }

        return false;
    }

    default:
        DVASSERT(0, "Invalid material editor filter used");
        break;
    }

    return false;
}

bool MaterialFilteringModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    // global material should always be first
    MaterialItem* lhsItem = materialModel->itemFromIndex(left.sibling(left.row(), 0));
    MaterialItem* rhsItem = materialModel->itemFromIndex(right.sibling(right.row(), 0));
    DAVA::NMaterial* mLeft = lhsItem->GetMaterial();
    DAVA::NMaterial* mRight = rhsItem->GetMaterial();

    if ((mLeft == nullptr) || (mRight == nullptr))
        return QSortFilterProxyModel::lessThan(left, right);

    bool isLess = false;
    if (mLeft == materialModel->GetGlobalMaterial())
    {
        isLess = (sortOrder() == Qt::AscendingOrder);
    }
    else if (mRight == materialModel->GetGlobalMaterial())
    {
        isLess = (sortOrder() == Qt::DescendingOrder);
    }
    else
    {
        typedef int (*CompareFnSignature)(MaterialItem*, MaterialItem*);
        DAVA::Array<CompareFnSignature, 3> comparationChain;

        switch (sortColumn())
        {
        case MaterialModel::TITLE_COLUMN:
            comparationChain[0] = &MFMLocal::CompareNames;
            comparationChain[1] = &MFMLocal::CompareLods;
            comparationChain[2] = &MFMLocal::CompareSwitches;
            break;
        case MaterialModel::LOD_COLUMN:
            comparationChain[0] = &MFMLocal::CompareLods;
            comparationChain[1] = &MFMLocal::CompareSwitches;
            comparationChain[2] = &MFMLocal::CompareNames;
            break;
        case MaterialModel::SWITCH_COLUMN:
            comparationChain[0] = &MFMLocal::CompareSwitches;
            comparationChain[1] = &MFMLocal::CompareLods;
            comparationChain[2] = &MFMLocal::CompareNames;
            break;
        default:
            break;
        }

        typedef bool (*LessFunctor)(int, int);
        LessFunctor currentLessFunctor = &MFMLocal::Less;
        LessFunctor alternativeLessFunctor = currentLessFunctor;

        /// We have unusual requirements.
        /// we need to sort by "sortColumn()" in "sortOrder()"
        /// but if values in "sortColumn()" is equal, we need sort by other columns in AscendingOrder
        /// that why we invert comparation functor for DescendingOrder
        if (sortOrder() == Qt::DescendingOrder)
            alternativeLessFunctor = &MFMLocal::Greater;

        for (CompareFnSignature& compareFn : comparationChain)
        {
            int result = compareFn(lhsItem, rhsItem);
            if (result != 0)
            {
                isLess = currentLessFunctor(result, 0);
                break;
            }

            currentLessFunctor = alternativeLessFunctor;
        }
    }

    return isLess;
}

bool MaterialFilteringModel::dropMimeData(QMimeData const* data, Qt::DropAction action, int row, int column, QModelIndex const& parent)
{
    const bool ret = QSortFilterProxyModel::dropMimeData(data, action, row, column, parent);
    if (ret)
        invalidate();

    return ret;
}
