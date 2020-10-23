#include "Analytics/Analytics.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
namespace Analytics
{
void Core::Start()
{
    isStarted = true;
}

void Core::Stop()
{
    isStarted = false;
}

bool Core::IsStarted() const
{
    return isStarted;
}

void Core::SetConfig(const KeyedArchive* newConfig)
{
    if (newConfig == nullptr || !newConfig->IsKeyExists("events"))
    {
        DVASSERT(false, "Illegal config");
        return;
    }

    config.Set(new KeyedArchive(*newConfig));

    // check on/off option
    if (config->IsKeyExists("started"))
    {
        if (config->GetBool("started"))
        {
            Start();
        }
        else
        {
            Stop();
        }
    }

    for (auto& backend : backends)
    {
        backend.second->ConfigChanged(*config);
    }
}

const KeyedArchive* Core::GetConfig() const
{
    return config.Get();
}

void Core::AddBackend(const String& name, std::unique_ptr<IBackend> backend)
{
    DVASSERT(backends.find(name) == backends.end(),
             Format("Backend with name %s is already exist", name.c_str()).c_str());

    if (backend == nullptr)
    {
        DVASSERT(false, "Empty backend");
        return;
    }

    backends[name] = std::move(backend);
}

void Core::RemoveBackend(const String& name)
{
    backends.erase(name);
}

bool CheckEventPass(const KeyedArchive& config, const AnalyticsEvent& event)
{
    // check all-passing option
    if (config.GetBool("all"))
    {
        return true;
    }

    const KeyedArchive::UnderlyingMap& map = config.GetArchieveData();
    for (const auto& entry : map)
    {
        if (entry.first == event.name)
        {
            return true;
        }
    }

    return false;
}

bool Core::PostEvent(const AnalyticsEvent& event) const
{
    if (!IsStarted() || backends.empty() || config == nullptr)
    {
        return false;
    }

    // common filter
    bool isEventPasses = CheckEventPass(*config->GetArchive("events"), event);
    if (!isEventPasses)
    {
        return false;
    }

    // per-backend filter
    bool result = false;
    const KeyedArchive* backendsConfig = config->GetArchive("backend_events");
    for (const auto& backend : backends)
    {
        bool needProcessEvent = backendsConfig == nullptr;

        if (backendsConfig)
        {
            const KeyedArchive* backendConfig = backendsConfig->GetArchive(backend.first);

            if (backendConfig)
            {
                needProcessEvent = CheckEventPass(*backendConfig, event);
            }
        }

        if (needProcessEvent)
        {
            backend.second->ProcessEvent(event);
            result = true;
        }
    }

    return result;
}

} // namespace Analytics
} // namespace DAVA
