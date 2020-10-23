#pragma once

#include "Base/BaseTypes.h"
#include "Network/Private/ITransport.h"
#include "ServerTransportHolder.h"

namespace DAVA
{
namespace Net
{
class TCPServerTransport;
class IOLoop;
}
}

struct HttpRequest
{
    enum Method
    {
        GET,
        UNEXPECTED
    }; // other methods can be added here: HEAD, POST etc

    Method method = UNEXPECTED;
    DAVA::String uri;
    DAVA::String version;
};

struct HttpResponse
{
    DAVA::String version;
    DAVA::String code;
    DAVA::String body;
};

typedef DAVA::Net::IClientTransport* ClientID;

struct HttpServerListener
{
    virtual void OnHttpServerStopped(){};
    virtual void OnHttpRequestReceived(ClientID clientId, HttpRequest& rq){};
};

class HttpServer
: public ServerTransportListener
{
public:
    HttpServer(DAVA::Net::IOLoop* loop);
    ~HttpServer() override;

    void AddListener(HttpServerListener* listener);
    void RemoveListener(HttpServerListener* listener);

    void Start(const DAVA::Net::Endpoint& endpoint);
    void Stop();
    bool IsStarted() const;
    void SendResponse(ClientID clientId, HttpResponse& resp);

private:
    struct ClientSession
    {
        explicit ClientSession(ClientID client);
        void Clear();

        enum
        {
            WaitingForHeaderPart,
            WaitingForContentPart,
            RequestReady,
            WaitingForOurResponse,
            WaitingForSendComplete,
            Error
        } state = WaitingForHeaderPart;

        ClientID clientID;
        DAVA::String data;
        DAVA::uint32 contentLength = 0;
        HttpRequest request;
    };

private:
    void SendRawData(ClientID clientId, const void* data, size_t size);

    // IServerListener
    void OnTransportSpawned(DAVA::Net::IServerTransport* parent, DAVA::Net::IClientTransport* child) override;
    void OnTransportTerminated(DAVA::Net::IServerTransport* tr) override;

    // IClientListener
    void OnTransportTerminated(DAVA::Net::IClientTransport* tr) override;
    void OnTransportConnected(DAVA::Net::IClientTransport* tr, const DAVA::Net::Endpoint& endp) override;
    void OnTransportDisconnected(DAVA::Net::IClientTransport* tr, DAVA::int32 error) override;
    void OnTransportDataReceived(DAVA::Net::IClientTransport* tr, const void* buffer, size_t length) override;
    void OnTransportSendComplete(DAVA::Net::IClientTransport* tr) override;
    void OnTransportReadTimeout(DAVA::Net::IClientTransport* tr) override;

    void RemoveClient(DAVA::Net::IClientTransport* tr);
    void RemoveAllClients();

    void OnDataChunkAdded(ClientSession& session);
    void OnHeadersReceived(ClientSession& session);
    void OnRequestAssembled(ClientSession& session);
    void ParseHttpHeaders(ClientSession& session);

    void NotifyServerStopped();
    void NotifyRequestReceived(ClientID clientId, HttpRequest& rq);

private:
    DAVA::UnorderedSet<HttpServerListener*> listeners;
    ServerTransportHolder* serverTransportHolder = nullptr;
    DAVA::UnorderedMap<DAVA::Net::IClientTransport*, ClientSession> clientSessions;
    DAVA::Net::IOLoop* loop = nullptr;
    bool started = false;
};

inline bool HttpServer::IsStarted() const
{
    return started;
}
