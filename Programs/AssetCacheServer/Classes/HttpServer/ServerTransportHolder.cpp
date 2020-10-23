#include "HttpServer/ServerTransportHolder.h"
#include "Job/JobManager.h"
#include "Functional/Function.h"
#include "Engine/Engine.h"

ServerTransportHolder::ServerTransportHolder(DAVA::Net::IOLoop* aLoop, const DAVA::Net::Endpoint& aEndpoint, DAVA::uint32 readTimeout)
    : serverTransport(aLoop, aEndpoint, readTimeout)
{
}

ServerTransportHolder::~ServerTransportHolder()
{
    DVASSERT(isWorking == false);
}

DAVA::int32 ServerTransportHolder::Start(ServerTransportListener* owner_)
{
    DVASSERT(isWorking == false);
    DVASSERT(owner == nullptr);

    isWorking = true;
    owner = owner_;
    return serverTransport.Start(this);
}

void ServerTransportHolder::Stop()
{
    owner = nullptr;

    if (isWorking)
    {
        serverTransport.Stop();
    }
    else
    {
        DAVA::GetEngineContext()->jobManager->CreateWorkerJob(DAVA::MakeFunction(this, &ServerTransportHolder::DeleteItself));
    }
}

void ServerTransportHolder::DeleteItself()
{
    DVASSERT(isWorking == false && owner == nullptr);
    delete this;
}

void ServerTransportHolder::OnTransportSpawned(DAVA::Net::IServerTransport* parent, DAVA::Net::IClientTransport* client)
{
    if (owner)
    {
        owner->OnTransportSpawned(parent, client);
        client->Start(this);
    }
    else
    {
        client->Stop();
    }
}

void ServerTransportHolder::OnTransportTerminated(DAVA::Net::IServerTransport* serv)
{
    isWorking = false;

    if (owner)
    {
        static_cast<IServerListener*>(owner)->OnTransportTerminated(serv);
    }
    else
    {
        if (DAVA::GetEngineContext()->jobManager != nullptr)
        {
            DAVA::GetEngineContext()->jobManager->CreateWorkerJob(DAVA::MakeFunction(this, &ServerTransportHolder::DeleteItself));
        }
    }
}

void ServerTransportHolder::OnTransportTerminated(DAVA::Net::IClientTransport* clt)
{
    if (owner)
    {
        static_cast<IClientListener*>(owner)->OnTransportTerminated(clt);
    }

    serverTransport.ReclaimClient(clt);
}

void ServerTransportHolder::OnTransportConnected(DAVA::Net::IClientTransport* clt, const DAVA::Net::Endpoint& endp)
{
    if (owner)
        owner->OnTransportConnected(clt, endp);
}

void ServerTransportHolder::OnTransportDisconnected(DAVA::Net::IClientTransport* clt, DAVA::int32 error)
{
    if (owner)
        owner->OnTransportDisconnected(clt, error);
}

void ServerTransportHolder::OnTransportDataReceived(DAVA::Net::IClientTransport* clt, const void* buffer, size_t length)
{
    if (owner)
        owner->OnTransportDataReceived(clt, buffer, length);
}

void ServerTransportHolder::OnTransportSendComplete(DAVA::Net::IClientTransport* clt)
{
    if (owner)
        owner->OnTransportSendComplete(clt);
}

void ServerTransportHolder::OnTransportReadTimeout(DAVA::Net::IClientTransport* clt)
{
    if (owner)
        owner->OnTransportReadTimeout(clt);
}
