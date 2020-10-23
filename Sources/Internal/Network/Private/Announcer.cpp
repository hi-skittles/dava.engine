#include <Functional/Function.h>
#include <Debug/DVAssert.h>

#include <Network/Private/Announcer.h>

namespace DAVA
{
namespace Net
{
Announcer::Announcer(IOLoop* ioLoop, const Endpoint& endp, uint32 sendPeriod, Function<size_t(size_t, void*)> needDataCallback, const Endpoint& tcpEndp)
    : loop(ioLoop)
    , socket(ioLoop)
    , timer(ioLoop)
    , endpoint(endp)
    , announcePeriod(sendPeriod * 1000)
    , isTerminating(false)
    , runningObjects(0)
    , dataCallback(needDataCallback)
    , tcpEndpoint(tcpEndp)
    , acceptor(ioLoop)
{
    DVASSERT(true == endpoint.Address().IsMulticast());

    const bool toStringResult = endpoint.Address().ToString(endpAsString.data(), endpAsString.size());
    DVASSERT(toStringResult);

    DVASSERT(loop != nullptr && announcePeriod > 0 && dataCallback != nullptr);
}

Announcer::~Announcer()
{
    DVASSERT(0 == runningObjects && false == isTerminating);
}

void Announcer::Start()
{
    DVASSERT(false == isTerminating && 0 == runningObjects);
    loop->Post(MakeFunction(this, &Announcer::DoStart));
}

void Announcer::Stop(Function<void(IController*)> callback)
{
    DVASSERT(false == isTerminating);
    DVASSERT(callback != nullptr);
    isTerminating = true;
    stopCallback = callback;
    loop->Post(MakeFunction(this, &Announcer::DoStop));
}

void Announcer::Restart()
{
    loop->Post(MakeFunction(this, &Announcer::DoStop));
}

void Announcer::DoStart()
{
    int32 error = socket.Bind(Endpoint(endpoint.Port()), true);
    if (0 == error)
    {
        error = socket.JoinMulticastGroup(endpAsString.data(), NULL);
        if (0 == error)
            error = timer.Wait(0, MakeFunction(this, &Announcer::TimerHandleTimer));
    }

    int32 errorA = acceptor.Bind(tcpEndpoint);
    if (0 == errorA)
    {
        errorA = acceptor.StartListen(MakeFunction(this, &Announcer::AcceptorHandleConnect));
    }

    if ((error != 0 || errorA != 0) && false == isTerminating)
    {
        status = START_FAILED;
        DoStop();
    }
    else
    {
        status = STARTED;
    }
}

void Announcer::DoStop()
{
    if (status != START_FAILED)
    {
        status = NOT_STARTED;
    }

    if (socket.IsOpen() && !socket.IsClosing())
    {
        runningObjects += 1;
        socket.Close([this](UDPSocket*) { DoObjectClose(); });
    }
    if (timer.IsOpen() && !timer.IsClosing())
    {
        runningObjects += 1;
        timer.Close([this](DeadlineTimer*) { DoObjectClose(); });
    }
    if (acceptor.IsOpen() && !acceptor.IsClosing())
    {
        runningObjects += 1;
        acceptor.Close([this](TCPAcceptor*) { DoObjectClose(); });
    }
}

void Announcer::DoObjectClose()
{
    runningObjects -= 1;
    if (0 == runningObjects)
    {
        if (true == isTerminating)
        {
            loop->Post(MakeFunction(this, &Announcer::DoBye));
        }
        else
        {
            timer.Wait(RESTART_DELAY_PERIOD, [this](DeadlineTimer*) { DoStart(); });
        }
    }
}

void Announcer::DoBye()
{
    isTerminating = false;
    stopCallback(this);
}

void Announcer::TimerHandleTimer(DeadlineTimer* timer)
{
    if (true == isTerminating)
        return;

    size_t length = dataCallback(sizeof(buffer), buffer);
    if (length > 0)
    {
        Buffer buf = CreateBuffer(buffer, length);
        int32 error = socket.Send(endpoint, &buf, 1, MakeFunction(this, &Announcer::SocketHandleSend));
        if (error != 0)
        {
            DoStop();
        }
    }
    else
        timer->Wait(announcePeriod, MakeFunction(this, &Announcer::TimerHandleTimer));
}

void Announcer::SocketHandleSend(UDPSocket* socket, int32 error, const Buffer* buffers, size_t bufferCount)
{
    if (true == isTerminating)
        return;

    if (0 == error)
    {
        timer.Wait(announcePeriod, MakeFunction(this, &Announcer::TimerHandleTimer));
    }
    else
    {
        DoStop();
    }
}

void Announcer::AcceptorHandleConnect(TCPAcceptor* acceptor, int32 error)
{
    if (!error)
    {
        auto socketCloseHandler = [](TCPSocket* socket) { delete socket; };

        // Socket will be deleted later through callback system
        TCPSocket* socket = new TCPSocket(loop);
        acceptor->Accept(socket);

        size_t length = dataCallback(sizeof(tcpBuffer), tcpBuffer);
        if (length > 0)
        {
            auto socketWriteHandler = [socketCloseHandler](TCPSocket* socket, int32, const Buffer*, size_t) {
                socket->Close(socketCloseHandler);
            };

            Buffer buf = CreateBuffer(tcpBuffer, length);
            socket->Write(&buf, 1, socketWriteHandler);
        }
        else
        {
            socket->Close(socketCloseHandler);
        }
    }
}

} // namespace Net
} // namespace DAVA
