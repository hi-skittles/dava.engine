#include "Utils/AppsCommandsSender.h"
#include "QtHelpers/LauncherListener.h"
#include "QtHelpers/Private/LauncherIPCHelpers.h"

#include <QEventLoop>
#include <QTimer>

namespace AppsCommandsSenderDetails
{
//this enum describes channel-level of the OSI model
enum eChannelReply
{
    TARGET_NOT_FOUND = static_cast<int>(LauncherIPCHelpers::USER_REPLY),
    TIMEOUT_ERROR,
    OTHER_ERROR
};
}

AppsCommandsSender::AppsCommandsSender(QObject* parent)
    : QObject(parent)
    , socket(new QLocalSocket(this))
{
}

AppsCommandsSender::~AppsCommandsSender()
{
    if (socket->state() != QLocalSocket::UnconnectedState)
    {
        socket->abort();
    }
}

bool AppsCommandsSender::HostIsAvailable(const QString& appPath)
{
    QString actualPath = LauncherIPCHelpers::PathToKey(appPath);
    Q_ASSERT(socket->state() == QLocalSocket::UnconnectedState);

    socket->connectToServer(actualPath);
    socket->waitForConnected();
    if (socket->state() == QLocalSocket::ConnectedState)
    {
        socket->disconnectFromServer();
        return true;
    }
    return false;
}

bool AppsCommandsSender::Ping(const QString& appPath)
{
    using namespace LauncherIPCHelpers;
    int pingCode = static_cast<int>(PING);
    int replyCode = SendCommand(pingCode, appPath);
    return static_cast<eProtocolReply>(replyCode) == PONG;
}

bool AppsCommandsSender::RequestQuit(const QString& appPath)
{
    using namespace AppsCommandsSenderDetails;
    int quitCode = static_cast<int>(LauncherListener::eMessage::QUIT);
    int replyCode = SendCommand(quitCode, appPath);
    return static_cast<eChannelReply>(replyCode) == TARGET_NOT_FOUND
    || static_cast<LauncherListener::eReply>(replyCode) == LauncherListener::eReply::ACCEPT;
}

int AppsCommandsSender::SendCommand(int message, const QString& appPath)
{
    using namespace AppsCommandsSenderDetails;
    using namespace LauncherIPCHelpers;
    QString actualPath = LauncherIPCHelpers::PathToKey(appPath);
    Q_ASSERT(socket->state() == QLocalSocket::UnconnectedState);
    socket->connectToServer(actualPath);
    socket->waitForConnected();
    if (socket->state() == QLocalSocket::ConnectedState)
    {
        QEventLoop eventLoop;
        QTimer waitTimer(this);
        waitTimer.setSingleShot(true);
        connect(socket, &QLocalSocket::readyRead, &eventLoop, &QEventLoop::quit);
        connect(&waitTimer, &QTimer::timeout, &eventLoop, &QEventLoop::quit);

        waitTimer.start(10 * 1000); //wait 10 seconds for reply
        socket->write(QByteArray::number(message));
        eventLoop.exec();

        if (waitTimer.isActive() == false)
        {
            return static_cast<int>(TIMEOUT_ERROR);
        }
        QByteArray data = socket->readAll();
        socket->disconnectFromServer();
        return ProcessReply(data);
    }
    else
    {
        QLocalSocket::LocalSocketError lastError = socket->error();
        if (lastError == QLocalSocket::ServerNotFoundError)
        {
            return static_cast<int>(TARGET_NOT_FOUND);
        }
    }
    return static_cast<int>(OTHER_ERROR);
}

int AppsCommandsSender::ProcessReply(const QByteArray& data)
{
    bool ok = false;
    int code = data.toInt(&ok);
    if (ok)
    {
        return code;
    }
    return LauncherListener::UNKNOWN_MESSAGE;
}
