#pragma once

#include "Base/BaseTypes.h"
#include "ApplicationSettings.h"

#include "SharedDataParser.h"

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;

class SharedDataRequester : public QObject
{
    Q_OBJECT

public:
    explicit SharedDataRequester(QWidget* parent = 0);

    void RequestSharedData(ServerID serverID);
    void AddSharedServer(SharedServerParams serverParams, const DAVA::String& appPath);
    void RemoveSharedServer(ServerID serverID);

signals:
    void SharedDataReceived(const DAVA::List<SharedPoolParams>& pools, const DAVA::List<SharedServerParams>& servers);
    void ServerShared(PoolID poolID, ServerID serverID, const DAVA::String& serverName);
    void ServerUnshared();

private slots:
    void OnGetPoolsFinished();
    void OnGetServersFinished();
    void OnAddServerFinished();
    void OnRemoveServerFinished();

private:
    QNetworkReply* DeleteLater(QNetworkReply*& replyForDeletion);

private:
    QNetworkAccessManager* networkManager = nullptr;
    QNetworkReply* getPoolsRequest = nullptr;
    QNetworkReply* getServersRequest = nullptr;
    QNetworkReply* shareRequest = nullptr;
    QNetworkReply* unshareRequest = nullptr;

    ServerID getRequestOwnID = NullServerID;
    SharedServerParams shareRequestParams;

    DAVA::List<SharedPoolParams> pools;
};
