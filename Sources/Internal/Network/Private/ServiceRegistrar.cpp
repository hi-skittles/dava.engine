#include <algorithm>

#include <Debug/DVAssert.h>

#include <Network/ServiceRegistrar.h>

namespace DAVA
{
namespace Net
{
bool ServiceRegistrar::Register(ServiceID serviceId, ServiceCreator creator, ServiceDeleter deleter, const char8* name)
{
    DVASSERT(creator != nullptr && deleter != nullptr);
    DVASSERT(!IsRegistered(serviceId));

    // Duplicate services are not allowed in registrar
    if (!IsRegistered(serviceId))
    {
        // If name hasn'y been set then generate name string based on service ID
        Array<char8, Entry::MAX_NAME_LENGTH> generatedName;
        if (NULL == name)
        {
            Snprintf(generatedName.data(), generatedName.size(), "service-%u", serviceId);
            name = generatedName.data();
        }
        Entry en(serviceId, name, creator, deleter);
        registrar.push_back(en);
        return true;
    }
    return false;
}

bool ServiceRegistrar::UnRegister(ServiceID serviceId)
{
    if (!IsRegistered(serviceId))
    {
        return false;
    }

    auto iter = std::remove(registrar.begin(), registrar.end(), serviceId);
    registrar.erase(iter);

    return true;
}

void ServiceRegistrar::UnregisterAll()
{
    registrar.clear();
}

IChannelListener* ServiceRegistrar::Create(ServiceID serviceId, void* context) const
{
    const Entry* entry = FindEntry(serviceId);
    return entry != NULL ? entry->creator(serviceId, context)
                           :
                           NULL;
}

bool ServiceRegistrar::Delete(ServiceID serviceId, IChannelListener* obj, void* context) const
{
    const Entry* entry = FindEntry(serviceId);
    if (entry != NULL)
    {
        entry->deleter(obj, context);
        return true;
    }
    return false;
}

const char8* ServiceRegistrar::Name(ServiceID serviceId) const
{
    const Entry* entry = FindEntry(serviceId);
    return entry != NULL ? entry->name
                           :
                           NULL;
}

const ServiceRegistrar::Entry* ServiceRegistrar::FindEntry(ServiceID serviceId) const
{
    Vector<Entry>::const_iterator i = std::find(registrar.begin(), registrar.end(), serviceId);
    return i != registrar.end() ? &*i : NULL;
}

} // namespace Net
} // namespace DAVA
