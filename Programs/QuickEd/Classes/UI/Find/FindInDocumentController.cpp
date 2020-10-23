#include "UI/Find/FindInDocumentController.h"
#include "UI/Find/Finder/Finder.h"
#include "UI/Find/Widgets/FindInDocumentWidget.h"
#include "UI/mainwindow.h"
#include "Modules/DocumentsModule/DocumentData.h"
#include "Modules/DocumentsModule/DocumentsModule.h"

#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/WindowSubSystem/UI.h>
#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/WindowSubSystem/AlignedGeometryProcessor.h>
#include <QApplication>

FindInDocumentController::FindInDocumentController(DAVA::UI* ui, DAVA::ContextAccessor* accessor_)
    : QObject()
    , accessor(accessor_)
    , packageListenerProxy(this, accessor)
    , findResultsUpdater(300)
{
    using namespace DAVA;

    findInDocumentWidget = new FindInDocumentWidget();
    findInDocumentWidget->show();
    OverCentralPanelInfo info;
    info.geometryProcessor.reset(new AlignedGeometryProcessor(static_cast<DAVA::eAlign>(DAVA::ALIGN_TOP | DAVA::ALIGN_RIGHT), QPoint(30, 60)));
    PanelKey key("FindInDocuments", info);
    ui->AddView(DAVA::mainWindowKey, key, findInDocumentWidget);

    FieldDescriptor packageFieldDescr;
    packageFieldDescr.type = ReflectedTypeDB::Get<DocumentData>();
    packageFieldDescr.fieldName = FastName(DocumentData::packagePropertyName);

    const auto actionUpdater = [](const Any& fieldValue) -> Any {
        return fieldValue.CanCast<PackageNode*>() && fieldValue.Cast<PackageNode*>() != nullptr;
    };

    QtAction* findInDocumentAction = new QtAction(accessor, QObject::tr("Find in Document"), this);
    findInDocumentAction->setShortcut(QKeySequence::Find);
    findInDocumentAction->SetStateUpdationFunction(QtAction::Enabled, packageFieldDescr, actionUpdater);

    QtAction* findNextAction = new QtAction(accessor, QObject::tr("Find Next"), this);
    findNextAction->setShortcut(QKeySequence("F3"));
    findNextAction->SetStateUpdationFunction(QtAction::Enabled, packageFieldDescr, actionUpdater);

    QtAction* findPreviousAction = new QtAction(accessor, QObject::tr("Find Previous"), this);
    findPreviousAction->setShortcut(QKeySequence("Shift+F3"));
    findPreviousAction->SetStateUpdationFunction(QtAction::Enabled, packageFieldDescr, actionUpdater);

    QObject::connect(findInDocumentWidget, &FindInDocumentWidget::OnFindFilterReady, this, &FindInDocumentController::SetFilter);
    QObject::connect(findInDocumentWidget, &FindInDocumentWidget::OnFindNext, this, &FindInDocumentController::SelectNextFindResult);
    QObject::connect(findInDocumentWidget, &FindInDocumentWidget::OnFindPrevious, this, &FindInDocumentController::SelectPreviousFindResult);
    QObject::connect(findInDocumentWidget, &FindInDocumentWidget::OnFindAll, this, &FindInDocumentController::FindAll);
    QObject::connect(findInDocumentWidget, &FindInDocumentWidget::OnStopFind, this, &FindInDocumentController::HideFindInDocumentWidget);

    QObject::connect(findInDocumentAction, &QAction::triggered, this, &FindInDocumentController::ShowFindInDocumentWidget);
    QObject::connect(findNextAction, &QAction::triggered, this, &FindInDocumentController::SelectNextFindResult);
    QObject::connect(findPreviousAction, &QAction::triggered, this, &FindInDocumentController::SelectPreviousFindResult);

    ActionPlacementInfo placementInfo(CreateMenuPoint("Find", InsertionParams(InsertionParams::eInsertionMethod::AfterItem)));
    ui->AddAction(DAVA::mainWindowKey, placementInfo, findInDocumentAction);
    ui->AddAction(DAVA::mainWindowKey, placementInfo, findNextAction);
    ui->AddAction(DAVA::mainWindowKey, placementInfo, findPreviousAction);

    fieldBinder.reset(new FieldBinder(accessor));
    FieldDescriptor displayedRootControlsFieldDescriptor(ReflectedTypeDB::Get<DocumentData>(), FastName(DocumentData::displayedRootControlsPropertyName));
    fieldBinder->BindField(displayedRootControlsFieldDescriptor, MakeFunction(this, &FindInDocumentController::OnDisplayedRootControlsChanged));

    findResultsUpdater.SetUpdater(DAVA::MakeFunction(this, &FindInDocumentController::Find));
    findResultsUpdater.SetStopper([this]() {
        return context.filter == nullptr;
    });
}

void FindInDocumentController::ShowFindInDocumentWidget()
{
    DAVA::DataContext* activeContext = accessor->GetActiveContext();
    if (activeContext != nullptr)
    {
        findInDocumentWidget->Reset();
        DAVA::ShowOverCentralPanel(findInDocumentWidget);
        findInDocumentWidget->setFocus();
    }
}

void FindInDocumentController::HideFindInDocumentWidget()
{
    DAVA::HideOverCentralPanel(findInDocumentWidget);
}

void FindInDocumentController::SelectNextFindResult()
{
    MoveSelection(+1);
}

void FindInDocumentController::SelectPreviousFindResult()
{
    MoveSelection(-1);
}

void FindInDocumentController::FindAll()
{
    emit FindInDocumentRequest(context.filter);
}

void FindInDocumentController::SetFilter(std::shared_ptr<FindFilter> filter)
{
    context.filter = filter;

    Find();
}

void FindInDocumentController::Find()
{
    using namespace DAVA;

    context.results.clear();
    context.currentSelection = -1;

    Finder finder(context.filter, nullptr);

    QObject::connect(&finder, &Finder::ItemFound,
                     [this](const FindItem& item)
                     {
                         for (const String& path : item.GetControlPaths())
                         {
                             context.results.push_back(path);
                         }
                     });

    DataContext* activeContext = accessor->GetActiveContext();
    if (activeContext != nullptr)
    {
        DocumentData* data = activeContext->GetData<DocumentData>();

        const SortedControlNodeSet& displayedRootControls = data->GetDisplayedRootControls();

        finder.Process(data->GetPackageNode(), displayedRootControls);
    }

    findInDocumentWidget->SetResultIndex(context.currentSelection);
    findInDocumentWidget->SetResultCount(static_cast<int32>(context.results.size()));
}

void FindInDocumentController::FindIfActive()
{
    if (findInDocumentWidget->isVisible())
    {
        findResultsUpdater.Update();
    }
}

void FindInDocumentController::MoveSelection(DAVA::int32 step)
{
    using namespace DAVA;

    if (!context.results.empty())
    {
        context.currentSelection += step;

        if (context.currentSelection < 0)
        {
            context.currentSelection = static_cast<int32>(context.results.size() - 1);
        }
        else if (context.currentSelection >= context.results.size())
        {
            context.currentSelection = 0;
        }

        DataContext* activeContext = accessor->GetActiveContext();
        DVASSERT(activeContext != nullptr);
        DocumentData* data = activeContext->GetData<DocumentData>();

        const QString& path = data->GetPackageAbsolutePath();
        const QString& name = QString::fromStdString(context.results[context.currentSelection]);
        emit SelectControlRequest(path, name);
    }
    else
    {
        QApplication::beep();
        context.currentSelection = -1;
    }

    findInDocumentWidget->SetResultIndex(context.currentSelection);
}

void FindInDocumentController::OnDisplayedRootControlsChanged(const DAVA::Any& value)
{
    HideFindInDocumentWidget();

    context.results.clear();
    context.currentSelection = -1;
}

void FindInDocumentController::ControlPropertyWasChanged(ControlNode* node, AbstractProperty* property)
{
    FindIfActive();
}

void FindInDocumentController::StylePropertyWasChanged(StyleSheetNode* node, AbstractProperty* property)
{
    FindIfActive();
}

void FindInDocumentController::ControlWasAdded(ControlNode* node, ControlsContainerNode* destination, int index)
{
    FindIfActive();
}

void FindInDocumentController::ControlWasRemoved(ControlNode* node, ControlsContainerNode* from)
{
    FindIfActive();
}

void FindInDocumentController::StyleWasAdded(StyleSheetNode* node, StyleSheetsNode* destination, int index)
{
    FindIfActive();
}

void FindInDocumentController::StyleWasRemoved(StyleSheetNode* node, StyleSheetsNode* from)
{
    FindIfActive();
}

void FindInDocumentController::ImportedPackageWasAdded(PackageNode* node, ImportedPackagesNode* to, int index)
{
    FindIfActive();
}

void FindInDocumentController::ImportedPackageWasRemoved(PackageNode* node, ImportedPackagesNode* from)
{
    FindIfActive();
}
