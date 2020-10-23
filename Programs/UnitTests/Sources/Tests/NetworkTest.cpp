#include "DAVAEngine.h"
#include "UnitTests/UnitTests.h"

#include "Network/Base/IPAddress.h"
#include "Network/Base/Endpoint.h"

#include "Network/NetConfig.h"
#include "Network/NetService.h"
#include "Network/NetCore.h"

#if !defined(DAVA_NETWORK_DISABLE)

using namespace DAVA;
using namespace DAVA::Net;

struct Parcel
{
    void* outbuf;
    size_t length;
    uint32 packetId;

    friend bool operator==(const Parcel& o, const void* p)
    {
        return o.outbuf == p;
    }
};

class TestEchoServer : public DAVA::Net::NetService
{
public:
    TestEchoServer() = default;

    void OnPacketReceived(const std::shared_ptr<IChannel>& channel, const void* buffer, size_t length) override
    {
        bytesRecieved += length;
        SendEcho(buffer, length);
    }
    void OnPacketSent(const std::shared_ptr<IChannel>& channel, const void* buffer, size_t length) override
    {
        // buffer must be among sent buffers and its length must correspond to sent length
        Deque<Parcel>::iterator i = std::find(parcels.begin(), parcels.end(), buffer);
        if (i != parcels.end())
        {
            Parcel& parcel = *i;
            if (parcel.length == length)
            {
                // Check whether we have echoed end marker
                if (3 == length && 0 == strncmp(static_cast<const char8*>(buffer), "END", 3))
                {
                    lastPacketId = parcel.packetId; // Save packet ID for end marker
                }
                free(parcel.outbuf);
                parcel.outbuf = NULL;
            }
            bytesSent += length;
        }
    }
    void OnPacketDelivered(const std::shared_ptr<IChannel>& channel, uint32 packetId) override
    {
        if (false == parcels.empty())
        {
            // Delivery notifications must arrive in order of send operations
            Parcel parcel = parcels.front();
            parcels.pop_front();

            if (parcel.packetId == packetId)
            {
                bytesDelivered += parcel.length;
            }
        }
        if (packetId == lastPacketId)
        {
            testDone = true; // End marker has been recieved, echoed and confirmed, so testing is done
        }
    }

    bool IsTestDone() const
    {
        return testDone;
    }

    size_t BytesRecieved() const
    {
        return bytesRecieved;
    }
    size_t BytesSent() const
    {
        return bytesSent;
    }
    size_t BytesDelivered() const
    {
        return bytesDelivered;
    }

private:
    void SendEcho(const void* buffer, size_t length)
    {
        parcels.push_back(Parcel());
        Parcel& parcel = parcels.back();
        parcel.outbuf = malloc(length);
        parcel.length = length;
        parcel.packetId = 0;
        Memcpy(parcel.outbuf, buffer, length);
        Send(parcel.outbuf, parcel.length, &parcel.packetId);
    }

private:
    bool testDone = false;
    size_t bytesRecieved = 0;
    size_t bytesSent = 0;
    size_t bytesDelivered = 0;
    uint32 lastPacketId = 0;

    Deque<Parcel> parcels;
};

class TestEchoClient : public DAVA::Net::NetService
{
public:
    TestEchoClient()
    {
        // Prepare data of various length
        Vector<Parcel> a = {
            { ::operator new(1), 1, 0 },
            { ::operator new(1000), 1000, 0 },
            { ::operator new(10000), 10000, 0 },
            { ::operator new(100000), 100000, 0 },
            { ::operator new(1000000), 1000000, 0 },
            { ::operator new(10000000), 10000000, 0 }
        };
        uint8 v = 'A';
        for (size_t i = 0; i < a.size(); ++i, ++v)
        {
            Memset(a[i].outbuf, v, a[i].length);
            parcels.push_back(a[i]);
        }

        // Prepare end marker
        Parcel end = { ::operator new(3), 3, 0 };
        Memcpy(end.outbuf, "END", 3);
        parcels.push_back(end);
    }
    virtual ~TestEchoClient()
    {
        for (auto& x : parcels)
            ::operator delete(x.outbuf);
    }

    void ChannelOpen() override
    {
        // Send all parcels at a time
        for (auto& x : parcels)
            SendParcel(&x);
    }
    void OnPacketReceived(const std::shared_ptr<IChannel>& channel, const void* buffer, size_t length) override
    {
        if (pendingRead < parcels.size())
        {
            Parcel& p = parcels[pendingRead];
            if (p.length == length && 0 == Memcmp(p.outbuf, buffer, length))
            {
                bytesRecieved += length;
            }
            pendingRead += 1;
        }
        // Check for end marker echoed from server
        if (3 == length && 0 == strncmp(static_cast<const char8*>(buffer), "END", 3))
        {
            testDone = true;
        }
    }
    void OnPacketSent(const std::shared_ptr<IChannel>& channel, const void* buffer, size_t length) override
    {
        if (pendingSent < parcels.size())
        {
            // Check that send operation is sequential
            Parcel& p = parcels[pendingSent];
            if (p.length == length && 0 == Memcmp(p.outbuf, buffer, length))
            {
                bytesSent += length;
            }
            pendingSent += 1;
        }
    }
    void OnPacketDelivered(const std::shared_ptr<IChannel>& channel, uint32 packetId) override
    {
        if (pendingDelivered < parcels.size())
        {
            // Check that delivery notification is sequential
            Parcel& p = parcels[pendingDelivered];
            if (p.packetId == packetId)
            {
                bytesDelivered += p.length;
            }
            pendingDelivered += 1;
        }
    }

    bool IsTestDone() const
    {
        return testDone;
    }

    size_t BytesRecieved() const
    {
        return bytesRecieved;
    }
    size_t BytesSent() const
    {
        return bytesSent;
    }
    size_t BytesDelivered() const
    {
        return bytesDelivered;
    }

private:
    void SendParcel(Parcel* parcel)
    {
        Send(parcel->outbuf, parcel->length, &parcel->packetId);
    }

private:
    bool testDone = false;
    size_t bytesRecieved = 0;
    size_t bytesSent = 0;
    size_t bytesDelivered = 0;

    Deque<Parcel> parcels;
    size_t pendingRead = 0; // Parcel index expected to be read from server
    size_t pendingSent = 0; // Parcel index expected to be sent
    size_t pendingDelivered = 0; // Parcel index expected to be confirmed as delivered
};

DAVA_TESTCLASS (NetworkTest)
{
    //BEGIN_FILES_COVERED_BY_TESTS( )
    //FIND_FILES_IN_TARGET( DavaFramework )
    //    DECLARE_COVERED_FILES("NetCore.cpp")
    //    DECLARE_COVERED_FILES("NetConfig.cpp")
    //    DECLARE_COVERED_FILES("IPAddress.cpp")
    //    DECLARE_COVERED_FILES("Endpoint.cpp")
    //END_FILES_COVERED_BY_TESTS()

    enum eServiceTypes
    {
        SERVICE_ECHO = 1000
    };

    enum
    {
        ECHO_SERVER_CONTEXT,
        ECHO_CLIENT_CONTEXT
    };

    static const uint16 ECHO_PORT = 55101;

    bool echoTestDone = false;
    TestEchoServer echoServer;
    TestEchoClient echoClient;

    NetCore::TrackId serverId = NetCore::INVALID_TRACK_ID;
    NetCore::TrackId clientId = NetCore::INVALID_TRACK_ID;

    void Update(float32 timeElapsed, const String& testName) override
    {
        if (testName == "TestEcho")
        {
            echoTestDone = echoServer.IsTestDone() && echoClient.IsTestDone();
            if (echoTestDone)
            {
                TEST_VERIFY(echoServer.BytesRecieved() == echoServer.BytesSent());
                TEST_VERIFY(echoServer.BytesRecieved() == echoServer.BytesDelivered());

                TEST_VERIFY(echoClient.BytesRecieved() == echoClient.BytesSent());
                TEST_VERIFY(echoClient.BytesRecieved() == echoClient.BytesDelivered());

                TEST_VERIFY(echoServer.BytesRecieved() == echoClient.BytesRecieved());
            }
        }

        TestClass::Update(timeElapsed, testName);
    }

    void TearDown(const String& testName) override
    {
        if (testName == "TestEcho")
        {
            // Check whether DestroyControllerBlocked() really blocks until controller is destroyed
            size_t nactive = NetCore::Instance()->ControllersCount();
            NetCore::Instance()->DestroyControllerBlocked(serverId);
            TEST_VERIFY(NetCore::Instance()->ControllersCount() == nactive - 1);
            NetCore::Instance()->DestroyControllerBlocked(clientId);
            TEST_VERIFY(NetCore::Instance()->ControllersCount() == nactive - 2);
            serverId = NetCore::INVALID_TRACK_ID;
            clientId = NetCore::INVALID_TRACK_ID;
        }

        TestClass::TearDown(testName);
    }

    bool TestComplete(const String& testName) const override
    {
        if (testName == "TestEcho")
        {
            return echoTestDone;
        }
        return true;
    }

    DAVA_TEST (TestIPAddress)
    {
        // Test empty address
        TEST_VERIFY(true == IPAddress().IsUnspecified());
        TEST_VERIFY(0 == IPAddress().ToUInt());
        TEST_VERIFY("0.0.0.0" == IPAddress().ToString());

        // Test invalid address
        TEST_VERIFY(true == IPAddress("").IsUnspecified());
        TEST_VERIFY(true == IPAddress("invalid").IsUnspecified());
        TEST_VERIFY(true == IPAddress("300.0.1.2").IsUnspecified());
        TEST_VERIFY(true == IPAddress("08.08.0.1").IsUnspecified());

        // Test multicast address
        TEST_VERIFY(false == IPAddress("239.192.100.1").IsUnspecified());
        TEST_VERIFY(true == IPAddress("239.192.100.1").IsMulticast());
        TEST_VERIFY(false == IPAddress("192.168.0.4").IsMulticast());
        TEST_VERIFY(false == IPAddress("255.255.255.255").IsMulticast());
        TEST_VERIFY("239.192.100.1" == IPAddress("239.192.100.1").ToString());

        // Test address
        TEST_VERIFY(IPAddress("192.168.0.4") == IPAddress("192.168.0.4")); //-V501 test operator==
        TEST_VERIFY(String("192.168.0.4") == IPAddress("192.168.0.4").ToString());
        TEST_VERIFY(IPAddress("192.168.0.4").ToString() == IPAddress::FromString("192.168.0.4").ToString());
        TEST_VERIFY(false == IPAddress("192.168.0.4").IsUnspecified());
        TEST_VERIFY(false == IPAddress("255.255.255.255").IsUnspecified());
    }

    DAVA_TEST (TestEndpoint)
    {
        TEST_VERIFY(0 == Endpoint().Port());
        TEST_VERIFY(String("0.0.0.0:0") == Endpoint().ToString());

        TEST_VERIFY(1234 == Endpoint("192.168.1.45", 1234).Port());
        TEST_VERIFY(Endpoint("192.168.1.45", 1234).Address() == IPAddress("192.168.1.45"));
        TEST_VERIFY(Endpoint("192.168.1.45", 1234).Address() == IPAddress::FromString("192.168.1.45"));

        TEST_VERIFY(Endpoint("192.168.1.45", 1234) == Endpoint("192.168.1.45", 1234)); //-V501 test operator==
        TEST_VERIFY(false == (Endpoint("192.168.1.45", 1234) == Endpoint("192.168.1.45", 1235))); // Different ports
        TEST_VERIFY(false == (Endpoint("192.168.1.45", 1234) == Endpoint("192.168.1.46", 1234))); // Different addressess
    }

    DAVA_TEST (TestNetConfig)
    {
        TEST_VERIFY(SERVER_ROLE == NetConfig(SERVER_ROLE).Role());
        TEST_VERIFY(false == NetConfig(SERVER_ROLE).Validate());

        NetConfig config1(SERVER_ROLE);
        config1.AddTransport(TRANSPORT_TCP, Endpoint(9999));
        config1.AddTransport(TRANSPORT_TCP, Endpoint(8888));
        TEST_VERIFY(false == config1.Validate());
        TEST_VERIFY(2 == config1.Transports().size());
        config1.AddService(3);
        config1.AddService(1);
        config1.AddService(2);
        TEST_VERIFY(true == config1.Validate());
        TEST_VERIFY(2 == config1.Transports().size());
        TEST_VERIFY(3 == config1.Services().size());

        NetConfig config2 = config1.Mirror(IPAddress("192.168.1.20"));
        TEST_VERIFY(CLIENT_ROLE == config2.Role());
        TEST_VERIFY(true == config2.Validate());
        TEST_VERIFY(2 == config2.Transports().size());
        TEST_VERIFY(3 == config2.Services().size());
    }

    DAVA_TEST (TestEcho)
    {
        NetCore::Instance()->RegisterService(SERVICE_ECHO, MakeFunction(this, &NetworkTest::CreateEcho), MakeFunction(this, &NetworkTest::DeleteEcho));

        NetConfig serverConfig(SERVER_ROLE);
        serverConfig.AddTransport(TRANSPORT_TCP, Endpoint(ECHO_PORT));
        serverConfig.AddService(SERVICE_ECHO);

        NetConfig clientConfig = serverConfig.Mirror(IPAddress("127.0.0.1"));

        // Server config must be recreated first due to restrictions of service management
        serverId = NetCore::Instance()->CreateController(serverConfig, reinterpret_cast<void*>(ECHO_SERVER_CONTEXT));
        clientId = NetCore::Instance()->CreateController(clientConfig, reinterpret_cast<void*>(ECHO_CLIENT_CONTEXT));
    }

    IChannelListener* CreateEcho(uint32 serviceId, void* context)
    {
        if (ECHO_SERVER_CONTEXT == reinterpret_cast<intptr_t>(context))
            return &echoServer;
        else if (ECHO_CLIENT_CONTEXT == reinterpret_cast<intptr_t>(context))
            return &echoClient;
        return nullptr;
    }

    void DeleteEcho(IChannelListener * obj, void* context)
    {
        // Do nothing as services are members of NetworkTest
    }
};

#endif // !DAVA_NETWORK_DISABLE
