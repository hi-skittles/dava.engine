#pragma once

#include "AnimationTrack.h"

#include "Base/BaseTypes.h"
#include "Base/BaseObject.h"
#include "Debug/DVAssert.h"
#include "FileSystem/FilePath.h"

namespace DAVA
{
class FilePath;
class AnimationClip : public BaseObject
{
public:
    static const uint32 ANIMATION_CLIP_FILE_SIGNATURE = DAVA_MAKEFOURCC('D', 'V', 'A', 'F');

    struct FileHeader
    {
        uint32 signature = 0;
        uint32 version = 0;
        uint32 crc32 = 0;
        uint32 dataSize = 0;
    };

    ~AnimationClip();

    static AnimationClip* Load(const FilePath& fileName);

    float32 GetDuration() const;

    uint32 GetTrackCount() const;
    const AnimationTrack* GetTrack(uint32 track) const;
    const AnimationTrack* FindTrack(const char* trackUID) const;

    const char* GetTrackUID(uint32 track) const;
    const char* GetTrackName(uint32 track) const;

    uint32 GetMarkerCount() const;
    float32 GetMarkerTime(uint32 marker) const;
    const char* GetMarkerName(uint32 marker) const;

    void Dump() const;
    void Dump(std::ostream& stream) const;

private:
    AnimationClip() = default;

    struct Node
    {
        AnimationTrack track;
        const char* uid = nullptr;
        const char* name = nullptr;
    };

    struct Marker
    {
        float32 time = 0.f;
        const char* name = nullptr;
    };

    FilePath filepath;

    Vector<Node> nodes;
    Vector<Marker> markers;

    float32 duration = 0.f;
    uint8* animationData = nullptr;
};

inline float32 AnimationClip::GetDuration() const
{
    return duration;
}

inline unsigned AnimationClip::GetTrackCount() const
{
    return uint32(nodes.size());
}

inline const AnimationTrack* AnimationClip::GetTrack(uint32 track) const
{
    DVASSERT(track < GetTrackCount());
    return &nodes[track].track;
}

inline const char* AnimationClip::GetTrackUID(uint32 track) const
{
    DVASSERT(track < GetTrackCount());
    return nodes[track].uid;
}

inline const char* AnimationClip::GetTrackName(uint32 track) const
{
    DVASSERT(track < GetTrackCount());
    return nodes[track].name;
}

inline uint32 AnimationClip::GetMarkerCount() const
{
    return uint32(markers.size());
}

inline float32 AnimationClip::GetMarkerTime(uint32 marker) const
{
    DVASSERT(marker < GetMarkerCount());
    return markers[marker].time;
}

inline const char* AnimationClip::GetMarkerName(uint32 marker) const
{
    DVASSERT(marker < GetMarkerCount());
    return markers[marker].name;
}
}