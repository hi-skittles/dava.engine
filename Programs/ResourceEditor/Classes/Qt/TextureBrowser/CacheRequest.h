#ifndef CACHEREQUEST_H
#define CACHEREQUEST_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QPair>
#include <QList>
#include <QImage>

#include "FileSystem/FilePath.h"

class CacheRequest
: public QObject
{
    Q_OBJECT

private:
    typedef QPair<QString, QVariant> SlotWithArg;
    typedef QList<SlotWithArg> SlotList; // Possible set?
    typedef QMap<QObject*, SlotList> ObserverMap;

public:
    CacheRequest(const DAVA::FilePath _key);
    ~CacheRequest();

    void registerObserver(QObject* object, const QString& slot, const QVariant& userData = QVariant());

public slots:
    void invoke(const QList<QImage>& images);

private:
    const DAVA::FilePath key;
    ObserverMap observers;

private slots:
    void onObserverDestroyed();
};


#endif // CACHEREQUEST_H
