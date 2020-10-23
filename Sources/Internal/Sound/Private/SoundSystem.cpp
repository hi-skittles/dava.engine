#include "Sound/SoundSystem.h"
#include "Sound/Private/SoundStreamStub.h"
#include "Sound/Private/SoundEventStub.h"
#include "FileSystem/FileList.h"
#include "Scene3D/Entity.h"
#include "Scene3D/Systems/QualitySettingsSystem.h"
#include "FileSystem/YamlParser.h"
#include "FileSystem/YamlNode.h"
#include "Debug/ProfilerCPU.h"
#include "Debug/ProfilerMarkerNames.h"
#include "Engine/Engine.h"
#include "Concurrency/LockGuard.h"

namespace DAVA
{
SoundSystem::SoundSystem(Engine* e)
    : engine(e)
{
}

SoundSystem::~SoundSystem()
{
}

void SoundSystem::InitFromQualitySettings()
{
}

void SoundSystem::SetDebugMode(bool debug)
{
}

bool SoundSystem::IsDebugModeOn() const
{
    return false;
}

SoundStream* SoundSystem::CreateSoundStream(SoundStreamDelegate* streamDelegate, uint32 channelsCount)
{
    return new DAVA::SoundStreamStub();
}

SoundEvent* SoundSystem::CreateSoundEventByID(const FastName& eventName, const FastName& groupName)
{
    return new DAVA::SoundEventStub();
}

SoundEvent* SoundSystem::CreateSoundEventFromFile(const FilePath& fileName, const FastName& groupName, uint32 flags /* = SOUND_EVENT_DEFAULT */, int32 priority /* = 128 */)
{
    return new DAVA::SoundEventStub();
}

void SoundSystem::SerializeEvent(const SoundEvent* sEvent, KeyedArchive* toArchive)
{
}

SoundEvent* SoundSystem::DeserializeEvent(KeyedArchive* archive)
{
    return new DAVA::SoundEventStub();
}

SoundEvent* SoundSystem::CloneEvent(const SoundEvent* sEvent)
{
    return new DAVA::SoundEventStub();
}

void SoundSystem::LoadFEV(const FilePath& filePath)
{
}

void SoundSystem::UnloadFEV(const FilePath& filePath)
{
}

void SoundSystem::UnloadFMODProjects()
{
}

uint32 SoundSystem::GetMemoryUsageBytes() const
{
    uint32 memory = 0;
    return memory;
}

float32 SoundSystem::GetTotalCPUUsage() const
{
    float32 cpuTotal = 0.f;
    return cpuTotal;
}

int32 SoundSystem::GetChannelsUsed() const
{
    int32 channels = -1;
    return channels;
}

int32 SoundSystem::GetChannelsMax() const
{
    int32 softChannels = -1;
    return softChannels;
}

void SoundSystem::Mute(bool value)
{
}

void SoundSystem::SetCurrentLocale(const String& langID)
{
}

String SoundSystem::GetCurrentLocale() const
{
    return String();
}

void SoundSystem::SetListenerPosition(const Vector3& position)
{
}

void SoundSystem::SetListenerOrientation(const Vector3& forward, const Vector3& left)
{
}

void SoundSystem::GetAllEventsNames(Vector<String>& names)
{
}

void SoundSystem::PreloadFMODEventGroupData(const String& groupName)
{
}

void SoundSystem::ReleaseFMODEventGroupData(const String& groupName)
{
}

void SoundSystem::ReleaseAllEventWaveData()
{
}

void SoundSystem::SetAllGroupsVolume(float32 volume)
{
}

void SoundSystem::SetGroupVolume(const FastName& groupName, float32 volume)
{
}

float32 SoundSystem::GetGroupVolume(const FastName& groupName) const
{
    float32 ret = -1.f;
    return ret;
}

void SoundSystem::SetAllGroupsSpeed(float32 speed)
{
}

void SoundSystem::SetGroupSpeed(const FastName& groupName, float32 speed)
{
}

float32 SoundSystem::GetGroupSpeed(const FastName& groupName) const
{
    float32 ret = -1.f;
    return ret;
}

bool SoundSystem::IsSystemMusicPlaying()
{
    bool ret = false;
    return ret;
}

void SoundSystem::DuckSystemMusic(bool duck)
{
}

void SoundSystem::ParseSFXConfig(const FilePath& configPath)
{
}
}
