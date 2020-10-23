#ifndef __DAVAENGINE_REPLAY_H__
#define __DAVAENGINE_REPLAY_H__

#include "Base/Singleton.h"
#include "Base/BaseTypes.h"
#include "UI/UIEvent.h"
#include "FileSystem/File.h"
#include "Logger/Logger.h"

namespace DAVA
{
class File;
class Replay : public Singleton<Replay>
{
public:
    static inline bool IsRecord();
    static inline bool IsPlayback();

    Replay();
    virtual ~Replay();

    enum eValueType
    {
        VALUE_FRAMETIME = 0,
        VALUE_EVENTS_COUNT,
        VALUE_SEED
    };

    void StartRecord(const FilePath& dirName);
    void StopRecord();
    void RecordFrame(float32 frameTime);
    void RecordEventsCount(int32 eventsCount);
    void RecordEvent(const UIEvent* ev);
    void RecordSeed(const uint32 seed);
    void PauseReplay(bool isPause);

    void StartPlayback(const FilePath& dirName);
    float32 PlayFrameTime();
    int32 PlayEventsCount();
    UIEvent PlayEvent();
    void PlaySeed();
    bool IsEvent();

    bool ReplayPaused()
    {
        return pauseReplay;
    }

private:
    static bool isRecord;
    static bool isPlayback;

    File* file;

    template <class T>
    void Write(T value);

    template <class T>
    T Read();

    bool skipType;
    bool pauseReplay;
};

inline bool Replay::IsRecord()
{
    return (isRecord && !Replay::Instance()->ReplayPaused());
}

inline bool Replay::IsPlayback()
{
    return (isPlayback && !Replay::Instance()->ReplayPaused());
}

template <class T>
void Replay::Write(T value)
{
    file->Write(&value, sizeof(T));
}

template <class T>
T Replay::Read()
{
    T value;
    file->Read(&value, sizeof(T));

    if (file->GetPos() == file->GetSize())
    {
        isPlayback = false;
        Logger::FrameworkDebug("replay ended");
    }

    return value;
}
};

#endif // __DAVAENGINE_REPLAY_H__
