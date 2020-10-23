#pragma once

#include "Base/Singleton.h"
#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Base/FastName.h"
#include "FileSystem/FilePath.h"
#include "Base/EventDispatcher.h"
#include "Base/FastName.h"
#include "Sound/SoundEvent.h"
#include "Concurrency/Mutex.h"
#include "Sound/SoundStream.h"

namespace DAVA
{
class Engine;
class Component;
class SoundSystem : public Singleton<SoundSystem>
{
public:
    SoundSystem(Engine* e);
    Engine* engine = nullptr;

    virtual ~SoundSystem();

    virtual SoundStream* CreateSoundStream(SoundStreamDelegate* streamDelegate, uint32 channelsCount);

    virtual SoundEvent* CreateSoundEventByID(const FastName& eventName, const FastName& groupName);
    virtual SoundEvent* CreateSoundEventFromFile(const FilePath& fileName, const FastName& groupName, uint32 createFlags = SoundEvent::SOUND_EVENT_CREATE_DEFAULT, int32 priority = 128);

    virtual void SerializeEvent(const SoundEvent* sEvent, KeyedArchive* toArchive);
    virtual SoundEvent* DeserializeEvent(KeyedArchive* archive);
    virtual SoundEvent* CloneEvent(const SoundEvent* sEvent);

    virtual void Mute(bool value);

    virtual void SetCurrentLocale(const String& langID);
    virtual String GetCurrentLocale() const;

    virtual void SetListenerPosition(const Vector3& position);
    virtual void SetListenerOrientation(const Vector3& forward, const Vector3& left);

    virtual void SetAllGroupsVolume(float32 volume);
    virtual void SetGroupVolume(const FastName& groupName, float32 volume);
    virtual float32 GetGroupVolume(const FastName& groupName) const;

    virtual void SetAllGroupsSpeed(float32 speed);
    virtual void SetGroupSpeed(const FastName& groupName, float32 speed);
    virtual float32 GetGroupSpeed(const FastName& groupName) const;

    virtual void InitFromQualitySettings();

    virtual void SetDebugMode(bool debug = true);
    virtual bool IsDebugModeOn() const;

    virtual bool IsSystemMusicPlaying();

    virtual void DuckSystemMusic(bool duck);

public:
    virtual void LoadFEV(const FilePath& filePath);
    virtual void UnloadFEV(const FilePath& filePath);
    virtual void UnloadFMODProjects();

    virtual void PreloadFMODEventGroupData(const String& groupName);
    virtual void ReleaseFMODEventGroupData(const String& groupName);
    virtual void ReleaseAllEventWaveData();

    virtual void GetAllEventsNames(Vector<String>& names);

    virtual uint32 GetMemoryUsageBytes() const;
    virtual float32 GetTotalCPUUsage() const;
    virtual int32 GetChannelsUsed() const;
    virtual int32 GetChannelsMax() const;

protected:
    virtual void ParseSFXConfig(const FilePath& configPath);
};

SoundSystem* CreateSoundSystem(Engine* e);
};
