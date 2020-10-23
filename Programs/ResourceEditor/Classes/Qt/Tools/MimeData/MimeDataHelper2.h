#pragma once

#include "DAVAEngine.h"
#include <QMimeData>
#include <QDataStream>

template <class T>
class MimeDataHelper2
{
public:
    static QMimeData* EncodeMimeData(const QVector<T*>& data);
    static QVector<T*> DecodeMimeData(const QMimeData* data);

    static bool IsValid(const QMimeData* mimeData);
    static inline const QString GetMimeType();
};

template <class T>
QMimeData* MimeDataHelper2<T>::EncodeMimeData(const QVector<T*>& data)
{
    if (data.size() > 0)
    {
        QByteArray encodedData;
        QDataStream stream(&encodedData, QIODevice::WriteOnly);

        QMimeData* mimeData = new QMimeData();
        for (int i = 0; i < data.size(); ++i)
        {
            stream.writeRawData((char*)&data[i], sizeof(T*));
        }

        mimeData->setData(MimeDataHelper2<T>::GetMimeType(), encodedData);
        return mimeData;
    }

    return NULL;
}

template <class T>
QVector<T*> MimeDataHelper2<T>::DecodeMimeData(const QMimeData* data)
{
    QVector<T*> ret;

    QString format = MimeDataHelper2<T>::GetMimeType();
    if (data->hasFormat(format))
    {
        QByteArray encodedData = data->data(format);
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        while (!stream.atEnd())
        {
            T* object = NULL;
            stream.readRawData((char*)&object, sizeof(T*));
            ret.push_back(object);
        }
    }

    return ret;
}

template <class T>
bool MimeDataHelper2<T>::IsValid(const QMimeData* mimeData)
{
    return ((mimeData != NULL) && mimeData->hasFormat(MimeDataHelper2<T>::GetMimeType()));
}

template <>
inline const QString MimeDataHelper2<DAVA::Entity>::GetMimeType()
{
    return "application/dava.entity";
}
template <>
inline const QString MimeDataHelper2<DAVA::ParticleEmitterInstance>::GetMimeType()
{
    return "application/dava.particleemitterinstance";
}
template <>
inline const QString MimeDataHelper2<DAVA::ParticleLayer>::GetMimeType()
{
    return "application/dava.particlelayer";
}
template <>
inline const QString MimeDataHelper2<DAVA::ParticleForceSimplified>::GetMimeType()
{
    return "application/dava.particleforce";
}

template <>
inline const QString MimeDataHelper2<DAVA::ParticleForce>::GetMimeType()
{
    return "application/dava.particledragforce";
}

template <>
inline const QString MimeDataHelper2<DAVA::NMaterial>::GetMimeType()
{
    return "application/dava.nmaterial";
}

template <class T>
inline const QString MimeDataHelper2<T>::GetMimeType()
{
    DVASSERT(false && "calling non-instantiated GetMimeType");
    return QString();
}
