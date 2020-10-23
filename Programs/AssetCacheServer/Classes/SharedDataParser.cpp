#include "SharedDataParser.h"

#include "Logger/Logger.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace SharedDataParser
{
DAVA::List<SharedPoolParams> ParsePoolsReply(const QByteArray& data)
{
    DAVA::List<SharedPoolParams> pools;

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        DAVA::Logger::Error("Not a valid JSON document");
        return DAVA::List<SharedPoolParams>();
    }
    QJsonObject rootObj = document.object();
    QJsonValue rootValue = rootObj["pools"];

    if (rootValue.isUndefined())
    {
        DAVA::Logger::Error("'pools' key is not found");
        return DAVA::List<SharedPoolParams>();
    }

    if (!rootValue.isArray())
    {
        DAVA::Logger::Error("Array type is expected for key 'pools'");
        return DAVA::List<SharedPoolParams>();
    }

    QJsonArray rootArray = rootValue.toArray();
    for (QJsonValue val : rootArray)
    {
        if (!val.isObject())
        {
            DAVA::Logger::Error("Object type is expected");
            return DAVA::List<SharedPoolParams>();
        }
        QJsonObject poolObject = val.toObject();

        bool convertOk = false;
        qulonglong poolID = poolObject["key"].toString().toULongLong(&convertOk);
        if (!convertOk)
        {
            DAVA::Logger::Error("Can't convert %s to qulonglong", poolObject["key"].toString().toStdString().c_str());
            return DAVA::List<SharedPoolParams>();
        }

        SharedPoolParams pool;
        pool.poolID = static_cast<PoolID>(poolID);
        pool.name = poolObject["name"].toString().toStdString();
        pool.description = poolObject["description"].toString().toStdString();
        pools.push_back(std::move(pool));
    }

    return pools;
}

DAVA::List<SharedServerParams> ParseServersReply(const QByteArray& data)
{
    DAVA::List<SharedServerParams> servers;

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        DAVA::Logger::Error("Not a valid JSON document '%s'", data.data());
        return DAVA::List<SharedServerParams>();
    }
    QJsonObject rootObj = document.object();
    QJsonValue rootValue = rootObj["shared servers"];

    if (rootValue.isUndefined())
    {
        DAVA::Logger::Error("'shared servers' key is not found");
        return DAVA::List<SharedServerParams>();
    }

    if (!rootValue.isArray())
    {
        DAVA::Logger::Error("Array type is expected for key 'shared servers'");
        return DAVA::List<SharedServerParams>();
    }

    QJsonArray rootArray = rootValue.toArray();
    for (QJsonValue val : rootArray)
    {
        if (!val.isObject())
        {
            DAVA::Logger::Error("Object type is expected");
            return DAVA::List<SharedServerParams>();
        }
        QJsonObject poolObject = val.toObject();

        bool convertOk = false;
        qulonglong serverID = poolObject["key"].toString().toULongLong(&convertOk);
        if (!convertOk)
        {
            DAVA::Logger::Error("Can't convert %s to qulonglong", poolObject["key"].toString().toStdString().c_str());
            return DAVA::List<SharedServerParams>();
        }
        qulonglong poolID = poolObject["poolKey"].toString().toULongLong(&convertOk);
        if (!convertOk)
        {
            DAVA::Logger::Error("Can't convert %s to qulonglong", poolObject["poolKey"].toString().toStdString().c_str());
            return DAVA::List<SharedServerParams>();
        }

        SharedServerParams server;
        server.serverID = static_cast<ServerID>(serverID);
        server.poolID = static_cast<PoolID>(poolID);
        server.name = poolObject["name"].toString().toStdString();
        server.ip = poolObject["ip"].toString().toStdString();
        server.port = poolObject["port"].toInt();
        servers.push_back(std::move(server));
    }

    return servers;
}

ServerID ParseAddReply(const QByteArray& data)
{
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        DAVA::Logger::Error("Not a valid JSON document '%s'", data.data());
        return NullServerID;
    }

    QJsonObject rootObj = document.object();

    bool convertOk = false;
    qulonglong serverID = rootObj["key"].toString().toULongLong(&convertOk);
    if (!convertOk)
    {
        DAVA::Logger::Error("Can't convert %s to qulonglong", rootObj["key"].toString().toStdString().c_str());
        return NullServerID;
    }

    return static_cast<ServerID>(serverID);
}
}
