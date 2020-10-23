#include "SharedDataRequester.h"
#include "Debug/DVAssert.h"
#include "Logger/Logger.h"

#include <Version/Version.h>

#include <QProcess>
#include <QNetworkAccessManager>
#include <QNetworkReply>

SharedDataRequester::SharedDataRequester(QWidget* parent)
    : networkManager(new QNetworkAccessManager(this))
{
}

QNetworkReply* SharedDataRequester::DeleteLater(QNetworkReply*& replyForDeletion)
{
    QNetworkReply* reply = replyForDeletion;
    replyForDeletion->deleteLater();
    replyForDeletion = nullptr;

    return reply;
}

void SharedDataRequester::RequestSharedData(ServerID ownID)
{
    //DAVA::Logger::Debug("Requesting shared data");
    getRequestOwnID = ownID;
    if (getPoolsRequest)
    {
        getPoolsRequest->abort();
        DeleteLater(getPoolsRequest);
    }

    pools.clear();

    if (getServersRequest)
    {
        getServersRequest->abort();
        DeleteLater(getServersRequest);
    }

    static const QNetworkRequest GET_POOLS_REQUEST = QNetworkRequest(QUrl(QString("http://ba-manager.wargaming.net/modules/jsonAPI/acs/api.php?cmd=getPools")));

    //DAVA::Logger::Debug("Sending request: %s", GET_POOLS_REQUEST.url().toString().toStdString().c_str());
    getPoolsRequest = networkManager->get(GET_POOLS_REQUEST);
    connect(getPoolsRequest, &QNetworkReply::finished, this, &SharedDataRequester::OnGetPoolsFinished);
}

void SharedDataRequester::OnGetPoolsFinished()
{
    DVASSERT(getPoolsRequest != nullptr);
    DVASSERT(getServersRequest == nullptr);

    QNetworkReply* reply = DeleteLater(getPoolsRequest);

    QNetworkReply::NetworkError error = reply->error();
    if (error != QNetworkReply::NoError)
    {
        DAVA::Logger::Error("Can't get pools list: %s", reply->errorString().toStdString().c_str());
        return;
    }

    //DAVA::Logger::Debug("Get pools done");
    pools = SharedDataParser::ParsePoolsReply(reply->readAll());

    QString s = QString("http://ba-manager.wargaming.net/modules/jsonAPI/acs/api.php?cmd=getShared&key=%1&version=%2")
                .arg(getRequestOwnID ? QString::number(getRequestOwnID) : "NULL")
                .arg(DAVA::Version::GetVersion().c_str());

    //DAVA::Logger::Debug("Sending request: %s", s.toStdString().c_str());
    getServersRequest = networkManager->get(QNetworkRequest(QUrl(s)));
    connect(getServersRequest, &QNetworkReply::finished, this, &SharedDataRequester::OnGetServersFinished);
}

void SharedDataRequester::OnGetServersFinished()
{
    DVASSERT(getServersRequest != nullptr);

    QNetworkReply* reply = DeleteLater(getServersRequest);

    QNetworkReply::NetworkError error = reply->error();
    if (error != QNetworkReply::NoError)
    {
        DAVA::Logger::Error("Can't get servers list: %s", reply->errorString().toStdString().c_str());
        return;
    }

    //DAVA::Logger::Debug("Get servers done");
    DAVA::List<SharedServerParams> servers = SharedDataParser::ParseServersReply(reply->readAll());
    emit SharedDataReceived(pools, servers);
}

void SharedDataRequester::AddSharedServer(SharedServerParams serverParams, const DAVA::String& appPath)
{
    shareRequestParams = serverParams;

    if (shareRequest)
    {
        shareRequest->abort();
        DeleteLater(shareRequest);
    }

#if defined(__DAVAENGINE_WINDOWS__)
    static DAVA::uint32 osKey = 1; // 1 for win, 2 for mac (see DF-11326)
#else
    static DAVA::uint32 osKey = 2;
#endif

    QString url = QString("http://ba-manager.wargaming.net/modules/jsonAPI/acs/api.php?cmd=share&port=%1&name=%2&poolKey=%3&instPath=%4&os=%5")
                  .arg(serverParams.port)
                  .arg(serverParams.name.c_str())
                  .arg(serverParams.poolID)
                  .arg(appPath.c_str())
                  .arg(osKey);
    //DAVA::Logger::Debug("Sending request: %s", url.toStdString().c_str());
    shareRequest = networkManager->get(QNetworkRequest(QUrl(url)));
    connect(shareRequest, &QNetworkReply::finished, this, &SharedDataRequester::OnAddServerFinished);
}

void SharedDataRequester::OnAddServerFinished()
{
    DVASSERT(shareRequest != nullptr);

    QNetworkReply* reply = DeleteLater(shareRequest);

    QNetworkReply::NetworkError error = reply->error();
    if (error != QNetworkReply::NoError)
    {
        DAVA::Logger::Error("Can't add server: %s", reply->errorString().toStdString().c_str());
        return;
    }

    ServerID serverID = SharedDataParser::ParseAddReply(reply->readAll());
    if (serverID != NullServerID)
    {
        DAVA::Logger::Info("Server is shared: poolID %u serverID %u name %s", shareRequestParams.poolID, serverID, shareRequestParams.name.c_str());
        emit ServerShared(shareRequestParams.poolID, serverID, shareRequestParams.name);
    }
}

void SharedDataRequester::RemoveSharedServer(ServerID serverID)
{
    if (unshareRequest)
    {
        unshareRequest->abort();
        DeleteLater(unshareRequest);
    }

    QString s = QString("http://ba-manager.wargaming.net/modules/jsonAPI/acs/api.php?cmd=unshare&key=%1").arg(serverID);
    //DAVA::Logger::Debug("Sending request: %s", s.toStdString().c_str());
    unshareRequest = networkManager->get(QNetworkRequest(QUrl(s)));
    connect(unshareRequest, &QNetworkReply::finished, this, &SharedDataRequester::OnRemoveServerFinished);
}

void SharedDataRequester::OnRemoveServerFinished()
{
    DVASSERT(unshareRequest != nullptr);

    QNetworkReply* reply = DeleteLater(unshareRequest);

    QNetworkReply::NetworkError error = reply->error();
    if (error != QNetworkReply::NoError)
    {
        DAVA::Logger::Error("Can't remove server");
        return;
    }

    DAVA::Logger::Info("Server is unshared");
    emit ServerUnshared();
}
