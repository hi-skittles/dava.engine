#pragma once

#include <DAVAEngine.h>
#include <Base/ScopedPtr.h>
#include <Base/Vector.h>
#include <FileSystem/FilePath.h>

#include "Render/RHI/rhi_Type.h"
#include "Render/RHI/Common/rhi_Private.h"
#include "Render/RHI/rhi_Public.h"

namespace DAVA
{
class Engine;
class Window;
}

using DAVA::uint32;
using DAVA::uint64;

class RhiCubeApp final
{
public:
    RhiCubeApp(DAVA::Engine& e);

    void OnAppStarted();
    void OnWindowCreated(DAVA::Window* w);
    void OnAppFinished();

    void OnSuspend();
    void OnResume();

    void BeginFrame();
    void Draw(DAVA::Window* window);
    void EndFrame();

private:
    void CreateDocumentsFolder();

    void SetupTriangle();
    void SetupCube();
    void rtInit();
    void mrtInit();

    void SetupInstancedCube();
    void DrawInstancedCube();

    void SetupTank();
    void DrawTank();

    void manticoreDraw();
    void rhiDraw();
    void rtDraw();
    void mrtDraw();
    void visibilityTestDraw();

    static void ScreenShotCallback(uint32 width, uint32 height, const void* rgba);

    bool inited;

    struct
    VertexP
    {
        float x, y, z;
    };

    struct
    VertexPNT
    {
        float x, y, z;
        float nx, ny, nz;
        float u, v;
    };

    struct
    VertexPNT_ex
    {
        float x, y, z;
        uint32 pad;
        float u, v;
        float nx, ny, nz;
    };

    struct
    VertexPT
    {
        float x, y, z;
        float u, v;
    };

    struct
    Object
    {
        rhi::HVertexBuffer vb;
        rhi::HVertexBuffer vb2;
        uint32 vb_layout;
        uint32 v_cnt;
        rhi::HIndexBuffer ib;
        rhi::HPipelineState ps;
        rhi::HConstBuffer vp_const[2];
        rhi::HConstBuffer fp_const;
        rhi::HTexture tex;
        rhi::HTextureSet texSet;
        rhi::HSamplerState samplerState;
    };

    Object triangle;

    Object cube;
    Object cube_mrt;
    uint64 cube_t0;
    float cube_angle;

    Object icube;
    uint64 icube_t0;
    float icube_angle;

    Object rtQuad;
    rhi::Packet rtQuadBatch0;
    rhi::Packet rtQuadBatch1;
    rhi::Packet rtQuadBatch2;
    rhi::Packet rtQuadBatch3;
    rhi::Handle rtColor0;
    rhi::Handle rtColor1;
    rhi::Handle rtColor2;
    rhi::Handle rtColor3;
    rhi::Handle rtDepthStencil;

    struct Tank
    {
        DAVA::Vector<rhi::Handle> vb;
        DAVA::Vector<rhi::Handle> ib;
        DAVA::Vector<uint32> indCount;
        rhi::Handle ps;
        rhi::Handle vp_const[2];
        rhi::Handle fp_const;
        rhi::Handle tex;
    };

    Tank tank;

    /*
    rhi::HPerfQuerySet  perfQuerySet[3];
    bool                perfQuerySetUsed[3];
    bool                perfQuerySetReady[3];
    unsigned            curPerfQuerySet;
    unsigned            firedPerfQuerySet;
*/
    //    rhi::HPerfQuerySet perfQuerySet;
    bool perfQuerySetFired;
};
