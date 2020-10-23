#pragma once

#include "UI/Find/Filters/FindFilter.h"
#include "UI/Find/Finder/FindItem.h"
#include "Model/PackageHierarchy/PackageBaseNode.h"

#include <Base/BaseTypes.h>
#include <QObject>
#include <QMutex>

class ControlNode;
class PackageNode;

class Finder : public QObject
{
    Q_OBJECT

public:
    Finder(std::shared_ptr<FindFilter> filter, const DAVA::Map<DAVA::String, DAVA::Set<DAVA::FastName>>* prototypes);
    ~Finder() override;

    void Process(const QStringList& files);
    void Process(const PackageNode* package);
    void Process(const PackageNode* package, const SortedControlNodeSet& controlNodesToProcess);
    void Stop();

signals:
    void Finished();
    void ItemFound(FindItem item);
    void ProgressChanged(int filesProcessed, int totalFiles);

private:
    static void CollectControls(FindItem& currentItem, const FindFilter& filter, const ControlInformation* control);

    void ProcessPackage(FindItem& currentItem, const PackageInformation* package);

    std::shared_ptr<FindFilter> filter;
    const DAVA::Map<DAVA::String, DAVA::Set<DAVA::FastName>>* prototypes;
    bool cancelling = false;
    QMutex mutex;
};
