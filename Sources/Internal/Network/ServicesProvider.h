#pragma once

#include <Base/BaseTypes.h>
#include <Network/ServiceRegistrar.h>
#include <Engine/Engine.h>

namespace DAVA
{
namespace Net
{
class NetService;
class ServicesProviderImpl;

/**
    ServicesProvider allows application to be detected by remote peers and provide network services for that peers.

    Usage:
        1. Create instance of ServicesProvider
        2. Create instances of necessary network services. 
           Only LoggerService, MemoryProfilerService should be used as remote peer expects these services only.
           Instance of service must be enclosed in shared_ptr.
        3. Add these services to ServicesProvider instance
        4. Call Start()

        Internally, on Start invocation, ServicesProvider will try to start all specified network services using first allowed TCP port.
        If that port is already occupied, ServicesProvider will try to use next allowed TCP port. As soon as connection is established,
        ServicesProvider is starting to announce into network about its available services.
        If any application on the remote side wants to connnect to our ServicesProvider-application and its services, it should use PeerDescription and Discoverer classes,
        that will check that announcement and extract available network service from that.
        Only one remote peer could be connected to ServicesProvider.
        
    Example:
        ServiceProvider sp(engine, "MyAppName");
        shared_ptr<NetLogger> netLoggerService = make_shared<NetLogger>();
        sp->AddService(LOG_SERVICE_ID, netLoggerService);
        sp->Start();
                                  // from now, remote peer can use Discoverer to detect our ServiceProvider app
                                  // and PeerDescription to extract available network services.
                                  // In our case, remote peer will detect that NetLogger service is being provided
                                  // and in own turn create network service that will accept logs transferred
                                  // over network from our ServiceProvider app.
                                  // Note that only one remote peer can be connected.
        ......
        netLoggerService.reset(); // after passing service to AddService, initial shared_ptr could be resetted
                                  // In that case, service will be deleted in ServiceProvider destructor.
        .....
        sp->Stop();
*/

class ServicesProvider final
{
public:
    /** constructs ServicesProvider object, accepting `engine` as ref to DAVA Engine instance
        and `appName` as a name of application that creates ServicesProvider object in it.
        `appName` may be used on the remote peer as a string tag of connected application.
    */
    explicit ServicesProvider(Engine& engine, const String& appName);

    /** destroys object, calling Stop() and unregistering all passed services */
    ~ServicesProvider();

    /** adds new service to ServicesProvider and registers it in ServiceRegistrar.

        `serviceId` is the unique identifier of added service
        It is not allowed to call AddService twice for each distinct ServiceID value.
        I.e,
            sp.AddService(0, s1); // allowed
            sp.AddService(0, s2); // forbidden, will induce assert

        `service` is a shared pointer to created instance of network service.
        Should be not nullable, otherwise will asserted
        I.e,
            shared_ptr<IChannelListener> s1(new Service1);
            shared_ptr<IChannelListener> s2;
            sp.AddService(1, s1); // allowed
            sp.AddService(2, s2); // forbidden, will induce assert

        It is assumed that passed network service is a 'server'
        whereas corresponding service on the remote peer side is a 'client'
        E.g.,
            NetLogger is a 'server' service and should be on ServiceProvider side,
            LogConsumer is a 'client' service and should be on remote peer side.

        Note that calling AddService after Start is not allowed and will generate assert
    */
    void AddService(ServiceID serviceId, std::shared_ptr<IChannelListener>& service);

    /** starts passed services on first available TCP port, that starts network announcement about it
        Note that two consecutive Start() calls are not allowed and will generate assert.
    */
    void Start();

    /** stops both network services and network announcement */
    void Stop();

    /** return range of TCP ports that provider can use for its purposes */
    static std::pair<uint16, uint16> GetTcpPortsRange();

private:
    class ServicesProviderImpl;
    std::unique_ptr<ServicesProviderImpl> impl;
};
}
}
