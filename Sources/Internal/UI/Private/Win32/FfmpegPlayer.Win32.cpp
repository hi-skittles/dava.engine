#include "UI/Private/Win32/FfmpegPlayer.Win32.h"
#include "Concurrency/LockGuard.h"
#include "Concurrency/Thread.h"

#if defined(__DAVAENGINE_WIN32__)
#if !defined(DISABLE_NATIVE_MOVIEVIEW)

#include "Concurrency/LockGuard.h"
#include "Concurrency/Thread.h"
#include "Engine/Engine.h"
#include "Logger/Logger.h"
#include "Render/Image/Image.h"
#include "Sound/SoundSystem.h"

namespace DAVA
{
FfmpegPlayer::~FfmpegPlayer()
{
    Stop();
    CloseMovie();
}

// Initialize the control.
void FfmpegPlayer::Initialize(const Rect&)
{
    AV::av_log_set_level(AV_LOG_ERROR);
    AV::av_log_set_callback([](void* avcl, int level, const char* fmt, va_list vl) {
        // We need to check ffmpeg log level here
        if (level > AV::av_log_get_level())
        {
            return;
        }

        Logger* log = GetEngineContext()->logger;
        if (nullptr != log)
        {
            Logger::eLogLevel l = Logger::LEVEL_DEBUG;
            switch (level)
            {
            case AV_LOG_DEBUG:
            case AV_LOG_TRACE:
                l = Logger::LEVEL_FRAMEWORK;
                break;
            case AV_LOG_INFO:
            case AV_LOG_VERBOSE:
                l = Logger::LEVEL_INFO;
                break;
            case AV_LOG_WARNING:
                l = Logger::LEVEL_WARNING;
                break;
            case AV_LOG_PANIC:
            case AV_LOG_FATAL:
            case AV_LOG_ERROR:
            default:
                l = Logger::LEVEL_ERROR;
                break;
            }
            log->Logv(l, fmt, vl);
        }
    });

    static bool isFFMGEGInited = false;

    if (!isFFMGEGInited)
    {
        AV::av_register_all();
        isFFMGEGInited = true;
    }
}

AV::AVFormatContext* FfmpegPlayer::CreateContext(const FilePath& path)
{
    AV::AVFormatContext* context = AV::avformat_alloc_context();
    bool isSuccess = true;

    int openRes = AV::avformat_open_input(&context, path.GetAbsolutePathname().c_str(), nullptr, nullptr);
    if (openRes != 0 || !context)
    {
        Logger::FrameworkDebug("Couldn't open input stream.\n");
        isSuccess = false;
    }

    if (!isSuccess || AV::avformat_find_stream_info(context, nullptr) < 0)
    {
        Logger::FrameworkDebug("Couldn't find stream information.\n");
        isSuccess = false;
    }

    if (!isSuccess)
    {
        avformat_close_input(&context);
    }
    return context;
}

// Start/stop the video playback.
void FfmpegPlayer::Play()
{
    switch (state)
    {
    case STOPPED:
    {
        movieContext = CreateContext(moviePath);
        if (nullptr == movieContext)
        {
            Logger::FrameworkDebug("Can't Open video.");
            return;
        }

        isVideoSubsystemInited = InitVideo();
        if (!isVideoSubsystemInited)
        {
            Logger::FrameworkDebug("Can't init video decoder.");
        }

        isAudioSubsystemInited = InitAudio();
        if (!isAudioSubsystemInited)
        {
            Logger::FrameworkDebug("Can't init audio decoder.");
        }

        // read some packets to fill audio buffers before init fmod
        state = PREFETCHING;

        if (isAudioSubsystemInited || isVideoSubsystemInited)
        {
            readingDataThread = Thread::Create(MakeFunction(this, &FfmpegPlayer::ReadingThread));
            readingDataThread->Start();
        }
    }
    break;

    case PAUSED:
    {
        state = PLAYING;
        PlayAudio();
    }
    break;

    case PLAYING:
    default:
        break;
    }
}

void FfmpegPlayer::PlayAudio()
{
    if (soundStream)
    {
        soundStream->Play();
    }
    frameTimer = GetTime();
}

// Open the Movie.
void FfmpegPlayer::OpenMovie(const FilePath& moviePath_, const OpenMovieParams&)
{
    Stop();
    moviePath = moviePath_;
}

void FfmpegPlayer::CloseMovie()
{
    videoShown = false;
    audioListen = false;
    audioClock = 0;
    frameLastPts = 0.f;
    frameLastDelay = 40e-3;
    videoClock = 0.f;
    mediaFileEOF = false;

    SafeDelete(soundStream);

    if (readingDataThread)
    {
        readingDataThread->Cancel();
        readingDataThread->Join();
        SafeRelease(readingDataThread);
    }

    if (videoDecodingThread)
    {
        videoDecodingThread->Cancel();
        videoDecodingThread->Join();
        SafeRelease(videoDecodingThread);
    }
    if (audioDecodingThread)
    {
        audioDecodingThread->Cancel();
        audioDecodingThread->Join();
        SafeRelease(audioDecodingThread);
    }

    ClearBuffers();

    if (rgbDecodedScaledFrame)
    {
        av_frame_free(&rgbDecodedScaledFrame);
        rgbDecodedScaledFrame = nullptr;
    }

    if (videoCodecContext)
    {
        AV::avcodec_close(videoCodecContext);
        videoCodecContext = nullptr;
    }

    if (audioCodecContext)
    {
        AV::avcodec_close(audioCodecContext);
        audioCodecContext = nullptr;
    }

    if (movieContext)
    {
        AV::avformat_close_input(&movieContext);
        movieContext = nullptr;
    }
}

bool FfmpegPlayer::InitVideo()
{
    for (unsigned int i = 0; i < movieContext->nb_streams; i++)
    {
        if (movieContext->streams[i]->codec->codec_type == AV::AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex = i;
            break;
        }
    }
    if (-1 == videoStreamIndex)
    {
        Logger::FrameworkDebug("Didn't find a video stream.\n");
        return false;
    }

    AV::AVRational avfps = movieContext->streams[videoStreamIndex]->avg_frame_rate;
    videoFramerate = avfps.num / static_cast<float64>(avfps.den);

    videoCodecContext = movieContext->streams[videoStreamIndex]->codec;
    AV::AVCodec* videoCodec = AV::avcodec_find_decoder(videoCodecContext->codec_id);
    if (nullptr == videoCodec)
    {
        Logger::FrameworkDebug("Codec not found.\n");
        return false;
    }
    if (AV::avcodec_open2(videoCodecContext, videoCodec, nullptr) < 0)
    {
        Logger::FrameworkDebug("Could not open codec.\n");
        return false;
    }

    frameHeight = videoCodecContext->height;
    frameWidth = videoCodecContext->width;
    frameBufferSize = ImageUtils::GetSizeInBytes(frameWidth, frameHeight, pixelFormat);

    DVASSERT(nullptr == videoDecodingThread);

    videoDecodingThread = Thread::Create(MakeFunction(this, &FfmpegPlayer::VideoDecodingThread));
    videoDecodingThread->Start();

    return true;
}

void FfmpegPlayer::SortPacketsByVideoAndAudio(AV::AVPacket* packet)
{
    DVASSERT(packet != nullptr);
    //isVideoSubsystemInited
    if (packet->stream_index == videoStreamIndex && isVideoSubsystemInited)
    {
        LockGuard<Mutex> locker(videoPacketsMutex);
        videoPackets.push_back(packet);
    }
    else
    if (packet->stream_index == audioStreamIndex && isAudioSubsystemInited)
    {
        LockGuard<Mutex> locker(audioPacketsMutex);
        currentPrefetchedPacketsCount++;
        audioPackets.push_back(packet);
    }
    else
    {
        AV::av_packet_free(&packet);
    }
}

float64 FfmpegPlayer::GetAudioClock() const
{
    float64 pts = audioClock; /* maintained in the audio thread */
    uint32 bytesPerSec = 0;
    uint32 n = audioCodecContext->channels * 2;
    if (movieContext->streams[audioStreamIndex])
    {
        bytesPerSec = audioCodecContext->sample_rate * n;
    }
    if (bytesPerSec)
    {
        pts -= static_cast<float64>(audioBufSize) / bytesPerSec;
    }
    return pts;
}

void FfmpegPlayer::ClearBuffers()
{
    DVASSERT(PLAYING != state && PAUSED != state);
    {
        LockGuard<Mutex> audioLock(audioPacketsMutex);
        for (AV::AVPacket* packet : audioPackets)
        {
            AV::av_packet_free(&packet);
        }
        audioPackets.clear();
    }
    {
        LockGuard<Mutex> videoLock(videoPacketsMutex);
        for (AV::AVPacket* packet : videoPackets)
        {
            AV::av_packet_free(&packet);
        }
        videoPackets.clear();
    }
    pcmBuffer.Clear();
    lastFrameData.data.clear();
    currentPrefetchedPacketsCount = 0;
}

void FfmpegPlayer::PcmDataCallback(uint8* data, uint32 datalen)
{
    pcmBuffer.Read(static_cast<uint8*>(data), datalen);
}

bool FfmpegPlayer::InitAudio()
{
    for (unsigned int i = 0; i < movieContext->nb_streams; i++)
    {
        if (movieContext->streams[i]->codec->codec_type == AV::AVMEDIA_TYPE_AUDIO)
        {
            audioStreamIndex = i;
            break;
        }
    }
    if (audioStreamIndex == -1)
    {
        Logger::FrameworkDebug("Didn't find an audio stream.\n");
        return false; // false;
    }

    // Get a pointer to the codec context for the audio stream
    audioCodecContext = movieContext->streams[audioStreamIndex]->codec;

    // Find the decoder for the audio stream
    AV::AVCodec* audioCodec = AV::avcodec_find_decoder(audioCodecContext->codec_id);
    if (audioCodec == nullptr)
    {
        Logger::FrameworkDebug("Audio codec not found.");
        return false;
    }

    // Open codec
    if (avcodec_open2(audioCodecContext, audioCodec, nullptr) < 0)
    {
        Logger::FrameworkDebug("Could not open audio codec.");
        return false;
    }

    //Out Audio Param
    uint64_t outChannelLayout = AV_CH_LAYOUT_STEREO;
    AV::AVSampleFormat outSampleFmt = AV::AV_SAMPLE_FMT_S16;
    int outSampleRate = static_cast<int>(SoundStream::GetDefaultSampleRate());
    outChannels = AV::av_get_channel_layout_nb_channels(outChannelLayout);
    audioBufSize = outSampleRate;

    //FIX:Some Codec's Context Information is missing
    int64_t inChannelLayout = AV::av_get_default_channel_layout(audioCodecContext->channels);

    audioConvertContext = AV::swr_alloc_set_opts(audioConvertContext, outChannelLayout, outSampleFmt, outSampleRate, inChannelLayout, audioCodecContext->sample_fmt, audioCodecContext->sample_rate, 0, nullptr);
    AV::swr_init(audioConvertContext);

    //nb_samples: AAC-1024 MP3-1152
    // If audioCodecContext->frame_size == 0 that means dynamic number of samplers in audio frames
    int outNbSamples = audioCodecContext->frame_size;
    if (outNbSamples > 0)
    {
        if (outSampleRate != audioCodecContext->sample_rate)
        {
            int64 calc_samples = AV::av_rescale_rnd(AV::swr_get_delay(audioConvertContext, audioCodecContext->sample_rate) + outNbSamples, outSampleRate, audioCodecContext->sample_rate, AV::AV_ROUND_DOWN);
            DVASSERT(calc_samples >= static_cast<int64>(std::numeric_limits<int32>::min()) && calc_samples <= static_cast<int64>(std::numeric_limits<int32>::max()), "Too big value of calculated sample rate!");
            outNbSamples = (int32)calc_samples;
        }
        outAudioBufferSize = static_cast<uint32>(av_samples_get_buffer_size(nullptr, outChannels, outNbSamples, outSampleFmt, 1));
    }
    else
    {
        DVASSERT(false, "Unsupported audio stream. Non constant number of samplers in audio stream!");
        Logger::Error("Unsupported audio stream. Non constant number of samplers in audio stream!");
        outAudioBufferSize = 0;
    }

    DVASSERT(nullptr == audioDecodingThread);

    audioDecodingThread = Thread::Create(MakeFunction(this, &FfmpegPlayer::AudioDecodingThread));
    audioDecodingThread->Start();

    return true;
}

float64 FfmpegPlayer::GetMasterClock() const
{
    if (isAudioSubsystemInited)
    {
        return GetAudioClock();
    }
    else
    {
        return (frameTimer - GetTime());
    }
}

float64 FfmpegPlayer::SyncVideoClock(AV::AVFrame* srcFrame, float64 pts)
{
    float64 frameDelay;

    if (pts != 0)
    {
        /* if we have pts, set video clock to it */
        videoClock = pts;
    }
    else
    {
        /* if we aren't given a pts, set it to the clock */
        pts = videoClock;
    }

    /* update the video clock */
    frameDelay = AV::av_q2d(videoCodecContext->time_base);
    /* if we are repeating a frame, adjust clock accordingly */
    frameDelay += srcFrame->repeat_pict * (frameDelay * 0.5);
    videoClock += frameDelay;

    return pts;
}

FfmpegPlayer::DecodedFrameBuffer* FfmpegPlayer::DecodeVideoPacket(AV::AVPacket* packet)
{
    DVASSERT(nullptr != packet);

    int32 gotPicture;
    AV::AVFrame* decodedFrame = AV::av_frame_alloc();
    int32 ret = AV::avcodec_decode_video2(videoCodecContext, decodedFrame, &gotPicture, packet);

    DecodedFrameBuffer* frameBuffer = nullptr;
    if (ret >= 0 && gotPicture)
    {
        // rgbTextureBufferHolder is a pointer to pointer to uint8. Used to obtain data directly to our rgbTextureBuffer
        AV::SwsContext* imgConvertCtx = AV::sws_getContext(videoCodecContext->width, videoCodecContext->height, videoCodecContext->pix_fmt, videoCodecContext->width, videoCodecContext->height, avPixelFormat, SWS_BICUBIC, nullptr, nullptr, nullptr);

        rgbDecodedScaledFrame = AV::av_frame_alloc();

        uint32 pixelSize = PixelFormatDescriptor::GetPixelFormatSizeInBits(pixelFormat);
        const int imgBufferSize = AV::av_image_get_buffer_size(avPixelFormat, videoCodecContext->width, videoCodecContext->height, pixelSize);

        uint8* outBuffer = reinterpret_cast<uint8*>(AV::av_mallocz(imgBufferSize));
        Memset(outBuffer, imgBufferSize, 1);
        AV::av_image_fill_arrays(rgbDecodedScaledFrame->data, rgbDecodedScaledFrame->linesize, outBuffer, avPixelFormat, videoCodecContext->width, videoCodecContext->height, 1);
        AV::av_free(outBuffer);

        float64 effectivePTS = GetPTSForFrame(decodedFrame, packet, videoStreamIndex);
        effectivePTS = SyncVideoClock(decodedFrame, effectivePTS);
        frameLastPts = effectivePTS;

        // released at UpdateVideo()
        frameBuffer = new DecodedFrameBuffer(frameBufferSize, pixelFormat, effectivePTS);
        // a trick to get converted data into one buffer with textureBufferSize because it could be larger than frame frame size.
        uint8* rgbTextureBufferHolder[1];
        rgbTextureBufferHolder[0] = frameBuffer->data.data();

        const uint32 scaledHeight = AV::sws_scale(imgConvertCtx, decodedFrame->data, decodedFrame->linesize, 0, frameHeight, rgbTextureBufferHolder, rgbDecodedScaledFrame->linesize);

        AV::av_frame_free(&rgbDecodedScaledFrame);
        AV::sws_freeContext(imgConvertCtx);
    }

    AV::av_frame_free(&decodedFrame);

    return frameBuffer;
}

void FfmpegPlayer::UpdateVideo(DecodedFrameBuffer* frameBuffer)
{
    DVASSERT(nullptr != frameBuffer);

    float64 timeBeforeCalc = GetTime();

    float64 delay = frameBuffer->pts - frameLastPts; /* the pts from last time */
    if (delay <= 0.0 || delay >= 1.0)
    {
        /* if incorrect delay, use previous one */
        delay = frameLastDelay;
    }
    /* save for next time */
    frameLastDelay = delay;
    frameLastPts = frameBuffer->pts;

    /* update delay to sync to audio */
    float64 referenceClock = GetMasterClock();

    float64 diff = frameBuffer->pts - referenceClock;

    /* Skip or repeat the frame. Take delay into account
        FFPlay still doesn't "know if this is the best guess." */
    float64 syncThreshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
    if (std::abs(diff) < AV_NOSYNC_THRESHOLD)
    {
        if (diff <= -syncThreshold)
        {
            delay = 0;
        }
        else if (diff >= syncThreshold)
        {
            delay = 2 * delay;
        }
    }
    frameTimer += delay;

    /* computer the REAL delay */
    float64 actualDelay = frameTimer - GetTime();
    if (actualDelay < 0.010)
    {
        /* Really it should skip the picture instead */
        actualDelay = 0.001;
    }

    UpdateDrawData(frameBuffer);

    uint32 sleepLessFor = static_cast<uint32>(GetTime() - timeBeforeCalc);
    uint32 sleepTime = static_cast<uint32>(actualDelay * 1000 + 0.5) - sleepLessFor;
    Thread::Sleep(sleepTime);
}

float64 FfmpegPlayer::GetPTSForFrame(AV::AVFrame* frame, AV::AVPacket* packet, uint32 stream)
{
    int64 pts;
    if (packet->dts != AV_NOPTS_VALUE)
    {
        pts = AV::av_frame_get_best_effort_timestamp(frame);
    }
    else
    {
        pts = 0;
    }

    float64 effectivePTS = static_cast<float64>(pts) * AV::av_q2d(videoCodecContext->time_base);
    return effectivePTS;
}

FfmpegPlayer::DrawVideoFrameData FfmpegPlayer::GetDrawData()
{
    LockGuard<Mutex> lock(lastFrameLocker);
    return lastFrameData;
}

PixelFormat FfmpegPlayer::GetPixelFormat() const
{
    return pixelFormat;
}

Vector2 FfmpegPlayer::GetResolution() const
{
    return Vector2(static_cast<float32>(frameWidth), static_cast<float32>(frameHeight));
}

FfmpegPlayer::PlayState FfmpegPlayer::GetState() const
{
    return state;
}

void FfmpegPlayer::UpdateDrawData(DecodedFrameBuffer* buffer)
{
    DVASSERT(nullptr != buffer);

    LockGuard<Mutex> lock(lastFrameLocker);

    lastFrameData.data = buffer->data;
    lastFrameData.format = pixelFormat;
    lastFrameData.frameHeight = videoCodecContext->height;
    lastFrameData.frameWidth = videoCodecContext->width;
}

void FfmpegPlayer::DecodeAudio(AV::AVPacket* packet, float64 timeElapsed)
{
    DVASSERT(packet->stream_index == audioStreamIndex);
    int got_data;
    AV::AVFrame* audioFrame = AV::av_frame_alloc();
    SCOPE_EXIT
    {
        AV::av_frame_free(&audioFrame);
    };

    int ret = avcodec_decode_audio4(audioCodecContext, audioFrame, &got_data, packet);
    if (ret < 0)
    {
        Logger::FrameworkDebug("Error in decoding audio frame.\n");
        return;
    }

    Vector<uint8> outAudioBuffer;
    if (got_data > 0)
    {
        outAudioBuffer.resize(maxAudioFrameSize * 2);
        uint8* data = outAudioBuffer.data();
        AV::swr_convert(audioConvertContext, &data, maxAudioFrameSize, (const uint8_t**)audioFrame->data, audioFrame->nb_samples);
    }
    else
    {
        Logger::FrameworkDebug("Convert audio NO DATA");
        return;
    }

    int64_t pts;
    if (packet->dts != AV_NOPTS_VALUE)
    {
        pts = packet->pts;
    }
    else
    {
        pts = 0;
    }
    if (pts != AV_NOPTS_VALUE)
    {
        audioClock = AV::av_q2d(audioCodecContext->time_base) * pts;
    }

    if (outAudioBufferSize > 0)
    {
        pcmBuffer.Write(outAudioBuffer.data(), outAudioBufferSize);
    }
}

void FfmpegPlayer::AudioDecodingThread()
{
    Thread* thread = Thread::Current();
    if (nullptr == thread)
    {
        return;
    }

    do
    {
        if (0 == currentPrefetchedPacketsCount || PAUSED == state || STOPPED == state)
        {
            if (mediaFileEOF)
            {
                audioListen = true;
            }
            Thread::Yield();
            continue;
        }

        DVASSERT(PLAYING == state || PREFETCHING == state);

        audioPacketsMutex.Lock();
        auto size = audioPackets.size();
        audioPacketsMutex.Unlock();

        if (size > 0)
        {
            audioPacketsMutex.Lock();
            AV::AVPacket* audioPacket = audioPackets.front();
            audioPackets.pop_front();
            currentPrefetchedPacketsCount--;
            audioPacketsMutex.Unlock();

            DecodeAudio(audioPacket, 0);

            AV::av_packet_free(&audioPacket);
        }
        else
        {
            Thread::Yield();
        }

    } while (!thread->IsCancelling());
}

void FfmpegPlayer::VideoDecodingThread()
{
    Thread* thread = Thread::Current();
    if (nullptr == thread)
    {
        return;
    }

    do
    {
        if (PLAYING != state)
        {
            Thread::Yield();
            continue;
        }

        videoPacketsMutex.Lock();
        auto size = videoPackets.size();
        videoPacketsMutex.Unlock();

        if (size > 0)
        {
            videoPacketsMutex.Lock();
            AV::AVPacket* videoPacket = videoPackets.front();
            videoPackets.pop_front();
            videoPacketsMutex.Unlock();

            DecodedFrameBuffer* frameBuffer = DecodeVideoPacket(videoPacket);
            AV::av_packet_free(&videoPacket);

            if (frameBuffer)
            {
                UpdateVideo(frameBuffer);
                SafeDelete(frameBuffer);
            }
        }
        else
        {
            if (mediaFileEOF)
            {
                videoShown = true;
            }
            Thread::Yield();
        }

    } while (!thread->IsCancelling());
}

void FfmpegPlayer::PrefetchData(uint32 dataSize)
{
    int retRead = 0;
    while (retRead >= 0 && currentPrefetchedPacketsCount < dataSize)
    {
        AV::AVPacket* packet = AV::av_packet_alloc();
        if (nullptr == packet)
        {
            Logger::FrameworkDebug("Can't allocate AVPacket!");
            DVASSERT(false && "Can't allocate AVPacket!");
            return;
        }
        av_init_packet(packet);
        retRead = AV::av_read_frame(movieContext, packet);

        mediaFileEOF = retRead < 0;

        if (mediaFileEOF)
        {
            AV::av_packet_free(&packet);
        }
        else
        {
            SortPacketsByVideoAndAudio(packet);
        }
    }
}

void FfmpegPlayer::ReadingThread()
{
    Thread* thread = Thread::Current();
    if (nullptr == thread)
    {
        return;
    }

    do
    {
        switch (state)
        {
        case PREFETCHING:
        {
            PrefetchData(maxAudioPacketsPrefetchedCount);
            // wait until just prefetched packets was decoded and moded to pcmBuffer
            while (currentPrefetchedPacketsCount > 0 && !mediaFileEOF)
            {
                if (thread->IsCancelling())
                {
                    return;
                }
                Thread::Yield();
            }

            if (isAudioSubsystemInited)
            {
                // could return nullptr. Then we will have no sound.
                soundStream = SoundSystem::Instance()->CreateSoundStream(this, outChannels);
            }

            state = PLAYING;

            PlayAudio();
        }
        break;
        case PLAYING:
            PrefetchData(maxAudioPacketsPrefetchedCount - currentPrefetchedPacketsCount);
        default:
            break;
        }

        Thread::Yield();
    } while (!thread->IsCancelling());
}

float64 FfmpegPlayer::GetTime() const
{
    return (AV::av_gettime() / 1000000.0);
}

// Pause/resume the playback.
void FfmpegPlayer::Pause()
{
    if (PLAYING != state)
    {
        return;
    }

    state = PAUSED;

    if (soundStream)
    {
        soundStream->Pause();
    }
}

void FfmpegPlayer::Resume()
{
    if (PAUSED == state)
    {
        Play();
    }
}

// Whether the movie is being played?
bool FfmpegPlayer::IsPlaying() const
{
    return PLAYING == state;
}

void FfmpegPlayer::Update()
{
    if (STOPPED != state && videoShown && audioListen)
    {
        Stop();
    }
}

void FfmpegPlayer::Stop()
{
    if (STOPPED == state)
    {
        return;
    }

    Pause();
    state = STOPPED;
    CloseMovie();
}
}

#endif // !DISABLE_NATIVE_MOVIEVIEW
#endif // __DAVAENGINE_WIN32__
