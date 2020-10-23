#pragma once

#include "Base/BaseTypes.h"
#include "Functional/Function.h"

namespace DAVA
{
namespace Net
{
struct IChannelListener;

using ServiceID = uint32;
using ServiceCreator = Function<IChannelListener*(ServiceID serviceId, void* context)>;
using ServiceDeleter = Function<void(IChannelListener* obj, void* context)>;

class ServiceRegistrar
{
private:
    struct Entry
    {
        static const size_t MAX_NAME_LENGTH = 32;

        Entry(ServiceID id, const char8* serviceName, ServiceCreator creatorFunc, ServiceDeleter deleterFunc);

        uint32 serviceId;
        char8 name[MAX_NAME_LENGTH];
        ServiceCreator creator;
        ServiceDeleter deleter;
    };

    friend bool operator==(const Entry& entry, uint32 serviceId);

public:
    bool Register(ServiceID serviceId, ServiceCreator creator, ServiceDeleter deleter, const char8* name = NULL);
    bool UnRegister(ServiceID serviceId);
    void UnregisterAll();
    bool IsRegistered(ServiceID serviceId) const;

    IChannelListener* Create(ServiceID serviceId, void* context) const;
    bool Delete(ServiceID serviceId, IChannelListener* obj, void* context) const;

    const char8* Name(ServiceID serviceId) const;

private:
    const Entry* FindEntry(ServiceID serviceId) const;

private:
    Vector<Entry> registrar;
};

//////////////////////////////////////////////////////////////////////////
inline ServiceRegistrar::Entry::Entry(ServiceID id, const char8* serviceName, ServiceCreator creatorFunc, ServiceDeleter deleterFunc)
    : serviceId(id)
    , creator(creatorFunc)
    , deleter(deleterFunc)
{
#if defined(__DAVAENGINE_WINDOWS__)
    strncpy_s(name, serviceName, _TRUNCATE);
#else
    strncpy(name, serviceName, MAX_NAME_LENGTH);
    name[MAX_NAME_LENGTH - 1] = '\0';
#endif
}

inline bool ServiceRegistrar::IsRegistered(ServiceID serviceId) const
{
    return std::find(registrar.begin(), registrar.end(), serviceId) != registrar.end();
}

inline bool operator==(const ServiceRegistrar::Entry& entry, ServiceID serviceId)
{
    return entry.serviceId == serviceId;
}

} // namespace Net
} // namespace DAVA
