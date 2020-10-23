#include "CacheRequest.h"

#include <QMetaObject>
#include <string>
#include <QList>
#include <QDebug>

CacheRequest::CacheRequest(const DAVA::FilePath _key)
    : QObject(NULL)
    , key(_key)
{
}

CacheRequest::~CacheRequest()
{
}

void CacheRequest::registerObserver(QObject* object, const QString& slot, const QVariant& userData)
{
    Q_ASSERT(object);

    connect(object, SIGNAL(destroyed()), SLOT(onObserverDestroyed()));
    observers[object] << SlotWithArg(slot, userData);
}

void CacheRequest::invoke(const QList<QImage>& images)
{
    for (ObserverMap::iterator i = observers.begin(); i != observers.end(); i++)
    {
        QObject* obj = i.key();
        const SlotList& slotList = i.value();
        foreach (const SlotWithArg& slotInfo, slotList)
        {
            const std::string methodName = slotInfo.first.toStdString();
            QMetaObject::invokeMethod(obj, methodName.c_str(), Qt::QueuedConnection,
                                      Q_ARG(const QList<QImage>&, images), Q_ARG(QVariant, slotInfo.second));
        }
    }
}

void CacheRequest::onObserverDestroyed()
{
    observers.remove(sender());
}
