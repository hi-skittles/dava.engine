#pragma once

#include "Utils/PackageListenerProxy.h"

#include <TArc/Core/FieldBinder.h>
#include <QtTools/Updaters/ContinuousUpdater.h>
#include <Base/BaseTypes.h>
#include <QObject>

class FindInDocumentWidget;
class FindFilter;

namespace DAVA
{
class ContextAccessor;
class UI;
} // namespace DAVA

class FindInDocumentController : public QObject, PackageListener
{
    Q_OBJECT
public:
    FindInDocumentController(DAVA::UI* ui, DAVA::ContextAccessor* accessor);

    Q_SIGNAL void FindInDocumentRequest(std::shared_ptr<FindFilter> filter);
    Q_SIGNAL void SelectControlRequest(const QString& path, const QString& name);

private slots:
    void ShowFindInDocumentWidget();
    void HideFindInDocumentWidget();

    void SelectNextFindResult();
    void SelectPreviousFindResult();

    void FindAll();

    void SetFilter(std::shared_ptr<FindFilter> filter);

private:
    struct FindContext
    {
        std::shared_ptr<FindFilter> filter;
        DAVA::Vector<DAVA::String> results;
        DAVA::int32 currentSelection = -1;
    };

    void Find();
    void FindIfActive();
    void MoveSelection(DAVA::int32 step);

    void OnDisplayedRootControlsChanged(const DAVA::Any& value);

    // PackageListener
    void ControlPropertyWasChanged(ControlNode* node, AbstractProperty* property) override;
    void StylePropertyWasChanged(StyleSheetNode* node, AbstractProperty* property) override;
    void ControlWasAdded(ControlNode* node, ControlsContainerNode* destination, int index) override;
    void ControlWasRemoved(ControlNode* node, ControlsContainerNode* from) override;
    void StyleWasAdded(StyleSheetNode* node, StyleSheetsNode* destination, int index) override;
    void StyleWasRemoved(StyleSheetNode* node, StyleSheetsNode* from) override;
    void ImportedPackageWasAdded(PackageNode* node, ImportedPackagesNode* to, int index) override;
    void ImportedPackageWasRemoved(PackageNode* node, ImportedPackagesNode* from) override;

    DAVA::ContextAccessor* accessor = nullptr;
    FindContext context;
    FindInDocumentWidget* findInDocumentWidget = nullptr;
    PackageListenerProxy packageListenerProxy;
    ContinuousUpdater findResultsUpdater;

    std::unique_ptr<DAVA::FieldBinder> fieldBinder;
};
