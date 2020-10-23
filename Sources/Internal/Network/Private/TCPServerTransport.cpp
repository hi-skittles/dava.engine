#include <Functional/Function.h>
#include <Debug/DVAssert.h>

#include <Network/Base/IOLoop.h>

#include <Network/Private/TCPClientTransport.h>
#include <Network/Private/TCPServerTransport.h>

namespace DAVA
{
namespace Net
{
TCPServerTransport::TCPServerTransport(IOLoop* aLoop, const Endpoint& aEndpoint, uint32 readTimeout_)
    : loop(aLoop)
    , endpoint(aEndpoint)
    , acceptor(aLoop)
    , listener(NULL)
    , isTerminating(false)
    , readTimeout(readTimeout_)
{
    DVASSERT(loop != NULL);
}

TCPServerTransport::~TCPServerTransport()
{
    DVASSERT(NULL == listener && false == isTerminating && true == spawnedClients.empty());
}

int32 TCPServerTransport::Start(IServerListener* aListener)
{
    DVASSERT(NULL == listener && false == isTerminating && aListener != NULL);
    listener = aListener;
    return DoStart();
}

void TCPServerTransport::Stop()
{
    DVASSERT(listener != NULL && false == isTerminating);
    isTerminating = true;
    DoStop();
}

void TCPServerTransport::Reset()
{
    DVASSERT(listener != NULL && false == isTerminating);
    DoStop();
}

void TCPServerTransport::ReclaimClient(IClientTransport* client)
{
    DVASSERT(spawnedClients.find(client) != spawnedClients.end());
    if (spawnedClients.find(client) != spawnedClients.end())
    {
        spawnedClients.erase(client);
        SafeDelete(client);
    }
}

int32 TCPServerTransport::DoStart()
{
    int32 error = acceptor.Bind(endpoint);
    if (0 == error)
    {
        error = acceptor.StartListen(MakeFunction(this, &TCPServerTransport::AcceptorHandleConnect));
    }
    return error;
}

void TCPServerTransport::DoStop()
{
    if (acceptor.IsOpen() && !acceptor.IsClosing())
    {
        acceptor.Close(MakeFunction(this, &TCPServerTransport::AcceptorHandleClose));
    }
}

void TCPServerTransport::AcceptorHandleClose(TCPAcceptor* acceptor)
{
    if (isTerminating)
    {
        IServerListener* p = listener;
        listener = NULL;
        isTerminating = false;
        p->OnTransportTerminated(this); // This can be the last executed line of object instance
    }
    else
    {
        DoStart();
    }
}

void TCPServerTransport::AcceptorHandleConnect(TCPAcceptor* acceptor, int32 error)
{
    if (true == isTerminating)
        return;

    if (0 == error)
    {
        TCPClientTransport* client = new TCPClientTransport(loop, readTimeout);
        error = acceptor->Accept(&client->Socket());
        if (0 == error)
        {
            spawnedClients.insert(client);
            listener->OnTransportSpawned(this, client);
        }
        else
        {
            SafeDelete(client);
        }
    }
    else
    {
        DoStop();
    }
}

} // namespace Net
} // namespace DAVA
