#pragma once

#include "../Common/rhi_Private.h"

namespace rhi
{
struct GLCommand;

class ProgGLES2
{
public:
    ProgGLES2(ProgType t);
    virtual ~ProgGLES2();

    bool Construct(const char* src_data);
    void Destroy();

    unsigned ShaderUid() const;
    void GetProgParams(unsigned progUid);
    unsigned SamplerCount() const;

    unsigned ConstBufferCount() const;
    Handle InstanceConstBuffer(unsigned bufIndex) const;

    void SetupTextureUnits(uint32 baseUnit, GLCommand* commands, uint32& commandsCount) const;

    static void InvalidateAllConstBufferInstances();

    class
    ConstBuf
    {
    public:
        struct Desc
        {
        };

        ConstBuf()
            : glProg(0)
            , location(-1)
            , count(0)
            , data(nullptr)
            , inst(nullptr)
            , lastInst(nullptr)
            , frame(static_cast<uint32>(CurFrame - 1))
        {
        }
        ~ConstBuf()
        {
            ConstBuf::Destroy();
        }

        bool Construct(uint32 prog, void** lastBoundData, unsigned loc, unsigned count);
        void Destroy();

        unsigned ConstCount() const;
        bool SetConst(unsigned const_i, unsigned count, const float* cdata);
        bool SetConst(unsigned const_i, unsigned const_sub_i, const float* cdata, unsigned data_count);

        const void* Instance() const;
        void SetToRHI(uint32 progUid, const void* instData) const;

        static void AdvanceFrame();

    private:
        void ReallocIfneeded();

        uint32 glProg;
        uint16 location;
        #if RHI_GL__USE_UNIFORMBUFFER_OBJECT
        unsigned ubo;
        #endif

        uint16 count;
        float* data;
        mutable float* inst;
        mutable void** lastInst;
        mutable uint32 frame;

        #if RHI_GL__USE_STATIC_CONST_BUFFER_OPTIMIZATION
        mutable uint32 isStatic : 1;
        mutable uint32 isUsedInDrawCall : 1;
        mutable uint32 lastmodifiedFrame;
        mutable std::vector<float*> altData;
        mutable std::vector<uint32> altDataAllocationFrame;
        #endif

        #if RHI_GL__DEBUG_CONST_BUFFERS
        mutable uint32 isTrueStatic : 1;
        mutable uint32 instCount;
        #endif

        friend class ProgGLES2;
        static uint32 CurFrame;
    };

private:
    struct
    ConstBufInfo
    {
        unsigned location;
        unsigned count;
    };

    ConstBufInfo cbuf[MAX_CONST_BUFFER_COUNT];
    void* cbufLastBoundData[MAX_CONST_BUFFER_COUNT];
    unsigned texunitLoc[16]; //-V730_NOINIT

    unsigned shader;
    uint32 prog;
    const ProgType type;
    unsigned texunitCount;
    mutable unsigned texunitInited : 1;
};

} // namespace rhi
