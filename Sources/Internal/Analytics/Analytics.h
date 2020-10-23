#pragma once

#include "Analytics/AnalyticsEvent.h"
#include "Base/BaseTypes.h"
#include "FileSystem/KeyedArchive.h"

namespace DAVA
{
class KeyedArchive;

namespace Analytics
{
/**
Analytics backend interface.
Backend should handle event and store or send to analytics service like Google Analytics.

Example of simple backend:
\code
struct PrintBackend : public IBackend
{
    void ConfigChanged(const KeyedArchive& config) override {}
    void ProcessEvent(const AnalyticsEvent& event) override 
    {
        printf("Event name: %s", event.name.c_str());
    }
}
\endcode
*/
struct IBackend
{
    virtual ~IBackend() = default;

    /** Handle changing of analytics system config. */
    virtual void ConfigChanged(const KeyedArchive& config) = 0;

    /** Handle analytics event. */
    virtual void ProcessEvent(const AnalyticsEvent& event) = 0;
};

/**
Analytics core class.
Entry point for all analytics events. Core filters it and sends to backends.
For events processing Core should be configured and set to started state; backends should be added.
*/
class Core
{
public:
    /** Allow event processing (set to started state). */
    void Start();

    /** Disallow event processing (set to not-started state). */
    void Stop();

    /** Return true if event processing allowed (core is set to started state). */
    bool IsStarted() const;

    /**
    Set config to analytics system.
    Config should contain:
    - options for configuration of the system and backends
        Has key-value form, so access to it looks like config->GetString("option")
        Option "started" has bool type and needed to set Core to started or not-started state
    - common events filtration config
        Required config, has key "events" and subtree with key-value options form
        Subtree contains list of allowed event names
        Bool option "all" used to allow all events
    - per-backend events filtration config
        Optional config, has key "backend_events" and contain list of subtrees for events filtration
        such as mentioned before

    Example of config creation by code:
    \code
    // Create main config archive
    RefPtr<KeyedArchive> config(new KeyedArchive);
    // Set started state
    config->SetBool("started", true);
    // Set option for backend
    config->SetString("google_id", "ID#777");

    // Create common events filtration config
    RefPtr<KeyedArchive> events(new KeyedArchive);
    // Allow all events
    events->SetBool("all", true);
    config->SetArchive("events", events.Get());

    // Create per-backend events filtration config
    RefPtr<KeyedArchive> backendEvents(new KeyedArchive);

    // Create "google analytics" backend config and disallow all events
    RefPtr<KeyedArchive> googleAnalyticsBackend(new KeyedArchive);
    googleAnalyticsBackend->SetBool("all", false);
    backendEvents->SetArchive("google_analytics", googleAnalyticsBackend.Get());

    // Create "printf" backend config and allow only hangar events
    RefPtr<KeyedArchive> printfBackend(new KeyedArchive);
    printfBackend->SetString("Hangar", "");
    backendEvents->SetArchive("printf", printfBackend.Get());

    config->SetArchive("backend_events", backendEvents.Get());

    // Apply config to core
    GetCore().SetConfig(config.Get());
    \endcode

    Another example, loading the save config from yaml file.
    Yaml file "~doc:/AnalyticsConfig.yaml":
    \code
    keyedArchive:
    started:
        bool: true
    google_id:
        string: "ID#777"
    events:
        keyedArchive:
            all:
                bool: true
    backend_events:
        keyedArchive:
            google_analytics:
                keyedArchive:
                    all:
                        bool: false
            printf:
                keyedArchive:
                    Hangar:
                        string: ""
    \endcode
    Load config from yaml and apply it:
    \code
    RefPtr<KeyedArchive> config(new KeyedArchive);
    config->LoadFromYamlFile("~doc:/AnalyticsConfig.yaml");
    GetCore().SetConfig(config.Get());
    \endcode

    New config will be discarded if it doesn't exist (newConfig == nullptr) or 
    common events filtration config doesn't exist.
    */
    void SetConfig(const KeyedArchive* newConfig);

    /** Return config if exists, nullptr otherwise. */
    const KeyedArchive* GetConfig() const;

    /** 
    Add backend with unique name to Core. 
    Undefined behavior will happen if backend name is non-unique.
    Method has no effect if backend is empty (backend == nullptr).
    */
    void AddBackend(const String& name, std::unique_ptr<IBackend> backend);

    /**
    Remove backend with specified name from Core.
    Method does nothing if Core doesn't contain the backend with specified name.
    */
    void RemoveBackend(const String& name);

    /** 
    Post event for filtration and processing.
    Method filters events by using common and per-backend filtration config and then sends them to backends.
    If Core has not-started state, is not configured or has no backends, any event will not pass.
    If event don't satisfy common filtration config, it will not pass.
    If per-backend filtration config doesn't exist, only common filtration config.
    will be applied to event and event will be sent to all backends.
    Otherwise, event will be sent to only those backends whose config it satisfies.
    Return true if event was sent to at least one backend.
    */
    bool PostEvent(const AnalyticsEvent& event) const;

private:
    RefPtr<KeyedArchive> config;
    UnorderedMap<String, std::unique_ptr<IBackend>> backends;
    bool isStarted = false;
};

} // namespace Analytics
} // namespace DAVA
