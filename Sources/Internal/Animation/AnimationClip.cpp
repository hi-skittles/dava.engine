#include "AnimationClip.h"
#include "AnimationTrack.h"

#include "DAVAConfig.h"
#include "Concurrency/Mutex.h"
#include "Concurrency/LockGuard.h"
#include "FileSystem/File.h"
#include "Logger/Logger.h"
#include "Utils/CRC32.h"

#include <sstream>

namespace DAVA
{
namespace AnimationClipDetails
{
static Map<FilePath, AnimationClip*> map;
static Mutex mutex;

const char* ReadAlignedString(uint8** dataBuff)
{
    const char* str = reinterpret_cast<const char*>(*dataBuff);
    uint32 stringBytes = uint32(strlen(str) + 1);
    if (stringBytes & 0x3)
    {
        stringBytes |= 0x3;
        ++stringBytes;
    }

    *dataBuff += stringBytes;

    return str;
}
}

AnimationClip::~AnimationClip()
{
    LockGuard<Mutex> guard(AnimationClipDetails::mutex);
    AnimationClipDetails::map.erase(filepath);

    SafeDeleteArray(animationData);
}

AnimationClip* AnimationClip::Load(const FilePath& fileName)
{
    {
        LockGuard<Mutex> guard(AnimationClipDetails::mutex);
        if (AnimationClipDetails::map.count(FILEPATH_MAP_KEY(fileName)) != 0)
            return SafeRetain(AnimationClipDetails::map[FILEPATH_MAP_KEY(fileName)]);
    }

    //binary file format described in 'AnimationBinaryFormat.md'
    AnimationClip* clip = nullptr;
    ScopedPtr<File> file(File::Create(fileName, File::OPEN | File::READ));
    if (file)
    {
        FileHeader header;
        file->Read(&header);

        if (header.signature == ANIMATION_CLIP_FILE_SIGNATURE && header.version == 1)
        {
            clip = new AnimationClip();
            clip->filepath = fileName;

            uint8* dataBuff = new uint8[header.dataSize];
            uint32 read = file->Read(dataBuff, header.dataSize);

            if (read != header.dataSize || CRC32::ForBuffer(dataBuff, header.dataSize) == header.crc32)
            {
                clip->animationData = dataBuff;

                clip->duration = *reinterpret_cast<float32*>(dataBuff);
                dataBuff += 4;

                uint32 nodeCount = *reinterpret_cast<uint32*>(dataBuff);
                dataBuff += 4;

                clip->nodes.resize(nodeCount);

                for (uint32 n = 0; n < nodeCount; ++n)
                {
                    clip->nodes[n].uid = AnimationClipDetails::ReadAlignedString(&dataBuff);
                    clip->nodes[n].name = AnimationClipDetails::ReadAlignedString(&dataBuff);

                    uint32 boundDataSize = clip->nodes[n].track.Bind(dataBuff); //-V595 pointer utilization
                    if (boundDataSize == 0)
                    {
                        SafeRelease(clip);
                        Logger::Error("[AnimationClip::Load] Failed to bind animation data for track#%d. File: %s", n, fileName.GetAbsolutePathname().c_str());
                        break;
                    }

                    dataBuff += boundDataSize;
                }

                if (clip != nullptr) //-V595 pointer utilization
                {
                    uint32 markerCount = *reinterpret_cast<uint32*>(dataBuff);
                    dataBuff += 4;

                    clip->markers.resize(markerCount);

                    for (uint32 m = 0; m < markerCount; ++m)
                    {
                        clip->markers[m].name = AnimationClipDetails::ReadAlignedString(&dataBuff);
                        clip->markers[m].time = *reinterpret_cast<float32*>(dataBuff);
                        dataBuff += 4;
                    }
                }
            }
            else
            {
                SafeRelease(clip);
                Logger::Error("[AnimationClip::Load] Mismatch CRC32 of animation data. Possibly file corrupted. File: %s", fileName.GetAbsolutePathname().c_str());
            }
        }
        else
        {
            Logger::Error("[AnimationClip::Load] Wrong animation file format. File: %s", fileName.GetAbsolutePathname().c_str());
        }
    }
    else
    {
        Logger::Error("[AnimationClip::Load] Can't open animation file. File: %s", fileName.GetAbsolutePathname().c_str());
    }

    if (clip)
    {
        LockGuard<Mutex> guard(AnimationClipDetails::mutex);
        AnimationClipDetails::map[FILEPATH_MAP_KEY(clip->filepath)] = clip;
    }

    return clip;
}

const AnimationTrack* AnimationClip::FindTrack(const char* trackUID) const
{
    for (const Node& n : nodes)
    {
        if (strcmp(n.uid, trackUID) == 0)
            return &n.track;
    }

    return nullptr;
}

void AnimationClip::Dump() const
{
    std::stringstream ss;
    Dump(ss);

    Logger::Info(ss.str().c_str());
}

void AnimationClip::Dump(std::ostream& stream) const
{
    using namespace std;

    stream << "AnimationClip:" << endl;
    stream << "Duration: " << duration << " s" << endl;
    stream << "Track Count: " << GetTrackCount() << endl;
    stream << endl;

    for (uint32 t = 0; t < GetTrackCount(); ++t)
    {
        stream << "Track #" << t << endl;
        stream << "    NodeName: " << nodes[t].name << endl;
        stream << "    NodeUID: " << nodes[t].uid << endl;

        uint32 channelCount = nodes[t].track.GetChannelsCount();

        stream << "    Channels: " << channelCount << endl;
        stream << endl;
        for (uint32 c = 0; c < channelCount; ++c)
        {
            stream << "    Channel #" << c << endl;
            stream << "        target: " << nodes[t].track.GetChannelTarget(c) << endl;
            stream << endl;
        }
    }

    stream.flush();
}
}
