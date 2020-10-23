#include <Functional/Function.h>
#include <Debug/DVAssert.h>
#include "Logger/Logger.h"

#include "Network/Private/Discoverer.h"
#include "Network/Base/NetworkUtils.h"

namespace DAVA
{
namespace Net
{
Discoverer::Discoverer(IOLoop* ioLoop, const Endpoint& endp, Function<void(size_t, const void*, const Endpoint&)> dataReadyCallback)
    : loop(ioLoop)
    , socket(ioLoop)
    , timer(ioLoop)
    , endpoint(endp)
    , isTerminating(false)
    , runningObjects(0)
    , dataCallback(dataReadyCallback)
    , tcpSocket(ioLoop)
{
    const bool toStringResult = endpoint.Address().ToString(endpAsString.data(), endpAsString.size());
    DVASSERT(toStringResult);

    DVASSERT(true == endpoint.Address().IsMulticast());
    DVASSERT(loop != nullptr && dataCallback != nullptr);
}

Discoverer::~Discoverer()
{
}

void Discoverer::Start()
{
    DVASSERT(false == isTerminating);
    loop->Post(MakeFunction(this, &Discoverer::DoStart));
}

void Discoverer::Stop(Function<void(IController*)> callback)
{
    DVASSERT(false == isTerminating);
    DVASSERT(callback != nullptr);
    stopCallback = callback;
    loop->Post(MakeFunction(this, &Discoverer::DoStop));
}

void Discoverer::Restart()
{
    loop->Post(MakeFunction(this, &Discoverer::DoStop));
}

bool Discoverer::TryDiscoverDevice(const Endpoint& endpoint)
{
    DVASSERT(!isTerminating);
    if (!(tcpSocket.IsOpen() || tcpSocket.IsClosing()))
    {
        DAVA::Logger::FrameworkDebug("Discovering on %s", endpoint.ToString().c_str());
        tcpEndpoint = endpoint;
        loop->Post(MakeFunction(this, &Discoverer::DiscoverDevice));
        return true;
    }
    else
    {
        tcpSocket.Close();
        return false;
    }
}

void Discoverer::DoStart()
{
    int32 error = socket.Bind(Endpoint(endpoint.Port()), true);
    if (0 == error)
    {
        error = socket.JoinMulticastGroup(endpAsString.data(), NULL);
        if (0 == error)
            error = socket.StartReceive(CreateBuffer(inbuf, sizeof(inbuf)), MakeFunction(this, &Discoverer::SocketHandleReceive));
    }
    if (error != 0 && false == isTerminating)
    {
        status = START_FAILED;
        DoStop();
    }
    else
    {
        status = STARTED;
    }
}

void Discoverer::DoStop()
{
    isTerminating = true;
    status = NOT_STARTED;

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
    if (tcpSocket.IsOpen() && !tcpSocket.IsClosing())
    {
        runningObjects += 1;
        tcpSocket.Close([this](TCPSocket*) { DoObjectClose(); });
    }
}

void Discoverer::DoObjectClose()
{
    runningObjects -= 1;
    if (0 == runningObjects)
    {
        if (true == isTerminating)
        {
            loop->Post(MakeFunction(this, &Discoverer::DoBye));
        }
        else
        {
            timer.Wait(RESTART_DELAY_PERIOD, [this](DeadlineTimer*) { DoStart(); });
        }
    }
}

void Discoverer::DoBye()
{
    isTerminating = false;
    stopCallback(this);
}

void Discoverer::DiscoverDevice()
{
    auto connectHandler = [this](TCPSocket* socket, int32 error) {
        if (0 == error)
        {
            socket->StartRead(CreateBuffer(tcpInbuf, sizeof(tcpInbuf)), MakeFunction(this, &Discoverer::TcpSocketHandleRead));
        }
        else
        {
            Logger::Debug("Can't discover on %s: %s", tcpEndpoint.ToString().c_str(), ErrorToString(error));
            socket->Close();
            if (isTerminating)
            {
                DoObjectClose();
                return;
            }
        }
    };

    tcpSocket.Connect(tcpEndpoint, connectHandler);
}

void Discoverer::SocketHandleReceive(UDPSocket* socket, int32 error, size_t nread, const Endpoint& endpoint, bool partial)
{
    if (true == isTerminating)
        return;

    if (0 == error)
    {
        if (nread > 0 && false == partial)
        {
            dataCallback(nread, inbuf, endpoint);
        }
    }
    else
    {
        DoStop();
    }
}

void Discoverer::TcpSocketHandleRead(TCPSocket* socket, int32 error, size_t nread)
{
    if (0 == error && nread > 0)
    {
        Endpoint remoteEndpoint;
        socket->RemoteEndpoint(remoteEndpoint);
        dataCallback(nread, tcpInbuf, remoteEndpoint);
    }
    socket->Close();
}

} // namespace Net
} // namespace DAVA
