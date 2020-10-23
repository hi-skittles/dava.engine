#include "RhiCubeApp.h"

    #include "Render/RHI/rhi_Public.h"
    #include "Render/RHI/Common/rhi_Private.h"
    #include "Render/RHI/Common/dbg_StatSet.h"
    #include "Render/RHI/rhi_ShaderCache.h"
    #include "Render/RHI/rhi_ShaderSource.h"

    #include "Engine/Engine.h"
    #include "Render/RenderBase.h"

    #include "Render/RHI/dbg_Draw.h"

    #include "FileSystem/DynamicMemoryFile.h"

using namespace DAVA;

RhiCubeApp::RhiCubeApp(DAVA::Engine& engine)
{
    engine.gameLoopStarted.Connect(this, &RhiCubeApp::OnAppStarted);
    engine.windowCreated.Connect(this, &RhiCubeApp::OnWindowCreated);
    engine.gameLoopStopped.Connect(this, &RhiCubeApp::OnAppFinished);
    engine.suspended.Connect(this, &RhiCubeApp::OnSuspend);
    engine.resumed.Connect(this, &RhiCubeApp::OnResume);
    engine.beginFrame.Connect(this, &RhiCubeApp::BeginFrame);
    engine.endFrame.Connect(this, &RhiCubeApp::EndFrame);

    inited = false;
}

void RhiCubeApp::OnAppStarted()
{
/*
    //    const char * src = "../../Tools/ResourceEditor/Data/Materials/Shaders/Default/materials-vp.sl";
    const char* src = "../../Tools/ResourceEditor/Data/Materials/Shaders/Default/water-fp.sl";
    //    const char * src = "../../Tools/ResourceEditor/Data/Materials/Shaders/ShadowVolume/shadowvolume-vp.sl";
    File* file = File::Create(src, File::OPEN | File::READ);

    if (file)
    {
        rhi::ShaderSource vp(src);
        unsigned sz = file->GetSize();
        char buf[64 * 1024];

        DVASSERT(sz < sizeof(buf));
        file->Read(buf, sz);
        buf[sz] = '\0';

        std::vector<std::string> defines;

//        defines.push_back( "VERTEX_LIT" );
//        defines.push_back( "1" );
//        defines.push_back( "NORMALIZED_BLINN_PHONG" );
//        defines.push_back( "1" );        

        
//        defines.push_back("FOG_LINEAR");defines.push_back("1");
//        defines.push_back("SKINNING");defines.push_back("1");
//        defines.push_back("VERTEX_FOG");defines.push_back("1");

        defines.push_back("PIXEL_LIT");
        defines.push_back("1");
        defines.push_back("REAL_REFLECTION");
        defines.push_back("1");

        if (vp.Construct(rhi::PROG_FRAGMENT, buf, defines))
        {
            //            vp.InlineFunctions();
            vp.GetSourceCode(rhi::HostApi());
            vp.Dump();
        }
    }
*/
/*
{
    File*   file = File::CreateFromSystemPath( "../../Tools/ResourceEditor/Data/Materials/Shaders/Default/materials-vp.cg", File::OPEN|File::READ );
    
    if( file )
    {
        rhi::ShaderSource   vp;
        uint32              sz = file->GetSize();
        char                buf[64*1024];

        DVASSERT(sz < sizeof(buf));
        file->Read( buf, sz );
        buf[sz] = '\0';


        std::vector<std::string>    defines;
        
        defines.push_back( "VERTEX_LIT" );
        defines.push_back( "1" );
        if( vp.Construct( rhi::PROG_VERTEX, buf, defines ) )
        {
            vp.Dump();
        }
    }
}
{
    File*   file = File::CreateFromSystemPath( "../../Tools/ResourceEditor/Data/Materials/Shaders/Default/materials-fp.cg", File::OPEN|File::READ );
    
    if( file )
    {
        rhi::ShaderSource   fp;
        uint32              sz = file->GetSize();
        char                buf[64*1024];

        DVASSERT(sz < sizeof(buf));
        file->Read( buf, sz );
        buf[sz] = '\0';


        std::vector<std::string>    defines;
        
        defines.push_back( "VERTEX_LIT" );
        defines.push_back( "1" );
        if( fp.Construct( rhi::PROG_FRAGMENT, buf, defines ) )
        {
            fp.Dump();
        }
    }
}
*/

    #if defined(__DAVAENGINE_WIN32__)
/*    
    {
        KeyedArchive* opt = new KeyedArchive();
        char title[128] = "RHI Cube  -  ";

        switch (rhi::HostApi())
        {
        case rhi::RHI_DX9:
            strcat(title, "DX9");
            break;
        case rhi::RHI_DX11:
            strcat(title, "DX11");
            break;
        case rhi::RHI_GLES2:
            strcat(title, "GL");
            break;
        }

        opt->SetInt32("fullscreen", 0);
        opt->SetInt32("bpp", 32);
        opt->SetString(String("title"), String(title));

        DAVA::Core::Instance()->SetOptions(opt);
    }
*/
    #endif
}

void RhiCubeApp::OnWindowCreated(DAVA::Window* w)
{
    //    const char* src = "test-vp.sl";
    const char* src = "~res:/Materials/Shaders/Default/materials-vp.sl";
    //    const char* src = "bug.sl";

    File* file = File::Create(src, File::OPEN | File::READ);

    if (file)
    {
        rhi::ShaderSource vp(src);
        unsigned sz = unsigned(file->GetSize());
        char buf[64 * 1024];

        DVASSERT(sz < sizeof(buf));
        file->Read(buf, sz);
        buf[sz] = '\0';

        std::vector<std::string> defines;

        defines.push_back("VERTEX_LIT");
        defines.push_back("1");
        defines.push_back("NORMALIZED_BLINN_PHONG");
        defines.push_back("1");

        if (vp.Construct(rhi::PROG_VERTEX, buf, defines))
        //        if (vp.Construct(rhi::PROG_FRAGMENT, buf, defines))
        {
            //            vp.GetSourceCode(rhi::HostApi());
            //            vp.GetSourceCode(rhi::RHI_DX11);
            //            vp.GetSourceCode(rhi::RHI_METAL);
            //            vp.GetSourceCode(rhi::RHI_GLES2);
            //            vp.Dump();
        }
    }
    exit(0);

    DbgDraw::EnsureInited();

    //SetupTriangle();
    SetupCube();
    //SetupInstancedCube();
    //    SetupTank();
    rtInit();
    mrtInit();

    inited = true;
}

void RhiCubeApp::OnAppFinished()
{
}

void RhiCubeApp::OnSuspend()
{
}

void RhiCubeApp::OnResume()
{
}

void RhiCubeApp::BeginFrame()
{
}

void RhiCubeApp::Draw(DAVA::Window* window)
{
    if (!inited)
        return;

    //    sceneRenderTest->Render();
    //rhiDraw();
    //manticoreDraw();
    //DrawInstancedCube();
    //rtDraw();
    mrtDraw();
    //    visibilityTestDraw();
}

void
RhiCubeApp::EndFrame()
{
    //    SCOPED_NAMED_TIMING("GameCore::EndFrame");
    rhi::Present();

    // rendering stats
    /*
    {
        const unsigned id[] =
        {
          rhi::stat_DIP,
          rhi::stat_DP,
          rhi::stat_SET_PS,
          rhi::stat_SET_CB,
          rhi::stat_SET_TEX
        };
        unsigned max_nl = 0;

        for (unsigned i = 0; i != countof(id); ++i)
        {
            unsigned l = strlen(StatSet::StatFullName(id[i]));

            if (l > max_nl)
                max_nl = l;
        }

        const int w = VirtualCoordinatesSystem::Instance()->GetPhysicalScreenSize().dx;
        const int h = VirtualCoordinatesSystem::Instance()->GetPhysicalScreenSize().dy;
        const int x0 = w - 230;
        const int x1 = x0 + (max_nl + 1) * (DbgDraw::SmallCharW + 1);
        const int lh = DbgDraw::SmallCharH + 2;
        int y = h - 200;

        DbgDraw::SetSmallTextSize();
        for (unsigned i = 0; i != countof(id); ++i, y += lh)
        {
            //        const uint32_t  clr = (i&0x1) ? Color4f(0.4f,0.4f,0.8f,1) : Color4f(0.4f,0.4f,1.0f,1);
            const uint32_t clr = 0xFFFFFFFF;

            //        DbgDraw::FilledRect2D( x0-8, y, x0+50*(DbgDraw::SmallCharW+1), y+lh );
            DbgDraw::Text2D(x0, y, clr, StatSet::StatFullName(id[i]));
            DbgDraw::Text2D(x1, y, clr, "= %u", StatSet::StatValue(id[i]));
        }
    }
*/
}

void
RhiCubeApp::CreateDocumentsFolder()
{
}

void RhiCubeApp::SetupTriangle()
{
    triangle.vb = rhi::HVertexBuffer(rhi::VertexBuffer::Create(3 * sizeof(VertexP)));
    triangle.v_cnt = 3;
    triangle.ib = rhi::HIndexBuffer(rhi::IndexBuffer::Create(3 * sizeof(uint16)));

    VertexP* v = (VertexP*)rhi::VertexBuffer::Map(triangle.vb, 0, 3 * sizeof(VertexP));

    if (v)
    {
        v[0].x = -0.52f;
        v[0].y = -0.10f;
        v[0].z = 0.0f;

        v[1].x = 0.0f;
        v[1].y = 0.52f;
        v[1].z = 0.0f;

        v[2].x = 0.52f;
        v[2].y = -0.10f;
        v[2].z = 0.0f;

        rhi::VertexBuffer::Unmap(triangle.vb);
    }

    uint16 i[3] = { 0, 1, 2 };

    rhi::IndexBuffer::Update(triangle.ib, i, 0, 3 * sizeof(uint16));

    rhi::ShaderCache::UpdateProg(
    rhi::HostApi(), rhi::PROG_VERTEX, FastName("vp-simple"),
    "vertex_in\n"
    "{\n"
    "    float3 pos : POSITION;\n"
    "};\n"
    "vertex_out\n"
    "{\n"
    "    float4 pos : SV_POSITION;\n"
    "};\n"
    "\n"
    "vertex_out\n"
    "vp_main( vertex_in input )\n"
    "{\n"
    "    vertex_out output;"
    "    float3 in_pos = input.pos.xyz;"
    "    output.pos = float4(in_pos.x,in_pos.y,in_pos.z,1.0);\n"
    "    return output;\n"
    "}\n");
    rhi::ShaderCache::UpdateProg(
    rhi::HostApi(), rhi::PROG_FRAGMENT, FastName("fp-simple"),
    "fragment_in\n"
    "{\n"
    "};\n"
    "fragment_out\n"
    "{\n"
    "    float4 color : SV_TARGET;\n"
    "};\n"
    "property float4 Tint;\n"
    "fragment_out\n"
    "fp_main( fragment_in input )\n"
    "{\n"
    "    fragment_out output;\n"
    "    output.color = Tint;\n"
    //    "    output.color = float4(1.0,1.0,1.0,1.0);\n"
    "    return output;\n"
    "};\n");
    rhi::PipelineState::Descriptor psDesc;

    psDesc.vertexLayout.Clear();
    psDesc.vertexLayout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    psDesc.vprogUid = FastName("vp-simple");
    psDesc.fprogUid = FastName("fp-simple");

    triangle.ps = rhi::HPipelineState(rhi::PipelineState::Create(psDesc));
    triangle.fp_const = rhi::HConstBuffer(rhi::PipelineState::CreateFragmentConstBuffer(triangle.ps, 0));
}

void RhiCubeApp::SetupCube()
{
    //    cube.vb = rhi::VertexBuffer::Create( 3*2*6*sizeof(VertexPNT_ex) );
    cube.vb = rhi::HVertexBuffer(rhi::VertexBuffer::Create(3 * 2 * 6 * sizeof(VertexPNT)));
    //-    cube.ib = rhi::InvalidHandle;

    float sz = 0.2f;
    float u0 = 0.0f;
    float u1 = 1.0f;
    float v0 = 0.0f;
    float v1 = 1.0f;

    VertexPNT v[36] =
    {
      { -sz, -sz, -sz, 0, 0, -1, u0, v1 }, { -sz, sz, -sz, 0, 0, -1, u0, v0 }, { sz, -sz, -sz, 0, 0, -1, u1, v1 }, { -sz, sz, -sz, 0, 0, -1, u0, v0 }, { sz, sz, -sz, 0, 0, -1, u1, v0 }, { sz, -sz, -sz, 0, 0, -1, u1, v1 },

      { sz, -sz, -sz, 1, 0, 0, u0, v1 },
      { sz, sz, -sz, 1, 0, 0, u0, v0 },
      { sz, -sz, sz, 1, 0, 0, u1, v1 },
      { sz, sz, -sz, 1, 0, 0, u0, v0 },
      { sz, sz, sz, 1, 0, 0, u1, v0 },
      { sz, -sz, sz, 1, 0, 0, u1, v1 },

      { sz, -sz, sz, 0, 0, 1, u0, v1 },
      { sz, sz, sz, 0, 0, 1, u0, v0 },
      { -sz, -sz, sz, 0, 0, 1, u1, v1 },
      { sz, sz, sz, 0, 0, 1, u0, v0 },
      { -sz, sz, sz, 0, 0, 1, u1, v0 },
      { -sz, -sz, sz, 0, 0, 1, u1, v1 },

      { -sz, -sz, sz, -1, 0, 0, u0, v1 },
      { -sz, sz, sz, -1, 0, 0, u0, v0 },
      { -sz, sz, -sz, -1, 0, 0, u1, v0 },
      { -sz, sz, -sz, -1, 0, 0, u1, v0 },
      { -sz, -sz, -sz, -1, 0, 0, u1, v1 },
      { -sz, -sz, sz, -1, 0, 0, u0, v1 },

      { -sz, sz, -sz, 0, 1, 0, u0, v1 },
      { -sz, sz, sz, 0, 1, 0, u0, v0 },
      { sz, sz, -sz, 0, 1, 0, u1, v1 },
      { -sz, sz, sz, 0, 1, 0, u0, v0 },
      { sz, sz, sz, 0, 1, 0, u1, v0 },
      { sz, sz, -sz, 0, 1, 0, u1, v1 },

      { -sz, -sz, -sz, 0, -1, 0, u0, v0 },
      { sz, -sz, -sz, 0, -1, 0, u1, v0 },
      { -sz, -sz, sz, 0, -1, 0, u0, v1 },
      { sz, -sz, -sz, 0, -1, 0, u1, v0 },
      { sz, -sz, sz, 0, -1, 0, u1, v1 },
      { -sz, -sz, sz, 0, -1, 0, u0, v1 }
    };

    /*
    VertexPNT_ex    v[36] = 
    {
        { -sz,-sz,-sz,0, 0,0,-1, u0,v1 }, { -sz,sz,-sz,0, 0,0,-1, u0,v0 }, { sz,-sz,-sz,0, 0,0,-1, u1,v1 },
        { -sz,sz,-sz,0, 0,0,-1, u0,v0 }, { sz,sz,-sz,0, 0,0,-1, u1,v0 }, { sz,-sz,-sz,0, 0,0,-1, u1,v1 },

        { sz,-sz,-sz,0, 1,0,0, u0,v1 }, { sz,sz,-sz,0, 1,0,0, u0,v0 }, { sz,-sz,sz,0, 1,0,0, u1,v1 },
        { sz,sz,-sz,0, 1,0,0, u0,v0 }, { sz,sz,sz,0, 1,0,0, u1,v0 }, { sz,-sz,sz,0, 1,0,0, u1,v1 },
    
        { sz,-sz,sz,0, 0,0,1, u0,v1 }, { sz,sz,sz,0, 0,0,1, u0,v0 }, { -sz,-sz,sz,0, 0,0,1, u1,v1 },    
        { sz,sz,sz,0, 0,0,1, u0,v0 }, { -sz,sz,sz,0, 0,0,1, u1,v0 }, { -sz,-sz,sz,0, 0,0,1, u1,v1 },
    
        { -sz,-sz,sz,0, -1,0,0, u0,v1 }, { -sz,sz,sz,0, -1,0,0, u0,v0 }, { -sz,sz,-sz,0, -1,0,0, u1,v0 },
        { -sz,sz,-sz,0, -1,0,0, u1,v0 }, { -sz,-sz,-sz,0, -1,0,0, u1,v1 }, { -sz,-sz,sz,0, -1,0,0, u0,v1 },

        { -sz,sz,-sz,0, 0,1,0, u0,v1 }, { -sz,sz,sz,0, 0,1,0, u0,v0 }, { sz,sz,-sz,0, 0,1,0, u1,v1 },
        { -sz,sz,sz,0, 0,1,0, u0,v0 }, { sz,sz,sz,0, 0,1,0, u1,v0 }, { sz,sz,-sz,0, 0,1,0, u1,v1 },
                
        { -sz,-sz,-sz,0, 0,-1,0, u0,v0 }, { sz,-sz,-sz,0, 0,-1,0, u1,v0 }, { -sz,-sz,sz,0, 0,-1,0, u0,v1 },
        { sz,-sz,-sz,0, 0,-1,0, u1,v0 }, { sz,-sz,sz,0, 0,-1,0, u1,v1 }, { -sz,-sz,sz,0, 0,-1,0, u0,v1 }
    };
*/
    /*
    VertexPNT_ex    v[36] = 
    {
        { -sz,-sz,-sz,0, u0,v1, 0,0,-1 }, { -sz,sz,-sz,0, u0,v0, 0,0,-1 }, { sz,-sz,-sz,0, u1,v1, 0,0,-1 },
        { -sz,sz,-sz,0, u0,v0, 0,0,-1 }, { sz,sz,-sz,0, u1,v0, 0,0,-1 }, { sz,-sz,-sz,0, u1,v1, 0,0,-1 },

        { sz,-sz,-sz,0, u0,v1, 1,0,0 }, { sz,sz,-sz,0, u0,v0, 1,0,0 }, { sz,-sz,sz,0, u1,v1, 1,0,0 },
        { sz,sz,-sz,0, u0,v0, 1,0,0 }, { sz,sz,sz,0, u1,v0, 1,0,0 }, { sz,-sz,sz,0, u1,v1, 1,0,0 },
    
        { sz,-sz,sz,0, u0,v1, 0,0,1 }, { sz,sz,sz,0, u0,v0, 0,0,1 }, { -sz,-sz,sz,0, u1,v1, 0,0,1 },    
        { sz,sz,sz,0, u0,v0, 0,0,1 }, { -sz,sz,sz,0, u1,v0, 0,0,1 }, { -sz,-sz,sz,0, u1,v1, 0,0,1 },
    
        { -sz,-sz,sz,0, u0,v1, -1,0,0 }, { -sz,sz,sz,0, u0,v0, -1,0,0 }, { -sz,sz,-sz,0, u1,v0, -1,0,0 },
        { -sz,sz,-sz,0, u1,v0, -1,0,0 }, { -sz,-sz,-sz,0, u1,v1, -1,0,0 }, { -sz,-sz,sz,0, u0,v1, -1,0,0 },

        { -sz,sz,-sz,0, u0,v1, 0,1,0 }, { -sz,sz,sz,0, u0,v0, 0,1,0 }, { sz,sz,-sz,0, u1,v1, 0,1,0 },
        { -sz,sz,sz,0, u0,v0, 0,1,0 }, { sz,sz,sz,0, u1,v0, 0,1,0 }, { sz,sz,-sz,0, u1,v1, 0,1,0 },
                
        { -sz,-sz,-sz,0, u0,v0, 0,-1,0 }, { sz,-sz,-sz,0, u1,v0, 0,-1,0 }, { -sz,-sz,sz,0, u0,v1, 0,-1,0 },
        { sz,-sz,-sz,0, u1,v0, 0,-1,0 }, { sz,-sz,sz,0, u1,v1, 0,-1,0 }, { -sz,-sz,sz,0, u0,v1, 0,-1,0 }
    };
*/
    rhi::VertexBuffer::Update(cube.vb, v, 0, sizeof(v));

    rhi::Texture::Descriptor tdesc(128, 128, rhi::TEXTURE_FORMAT_R8G8B8A8);

    //    tdesc.autoGenMipmaps = true;
    cube.tex = rhi::HTexture(rhi::Texture::Create(tdesc));

    uint8* tex = (uint8*)(rhi::Texture::Map(cube.tex));

    if (tex)
    {
        uint8 color1[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
        uint8 color2[4] = { 0x80, 0x80, 0x80, 0xFF };
        //        uint8   color1[4] = { 0xFF, 0x00, 0x00, 0xFF };
        //        uint8   color2[4] = { 0x80, 0x00, 0x00, 0xFF };
        uint32 cell_size = 8;

        for (unsigned y = 0; y != 128; ++y)
        {
            for (unsigned x = 0; x != 128; ++x)
            {
                uint8* p = tex + y * sizeof(uint32) * 128 + x * sizeof(uint32);
                uint8* c = (((y / cell_size) & 0x1) ^ ((x / cell_size) & 0x1)) ? color1 : color2;

                memcpy(p, c, sizeof(uint32));
            }
        }

        rhi::Texture::Unmap(cube.tex);
    }

    rhi::SamplerState::Descriptor sdesc;

    sdesc.fragmentSamplerCount = 1;
    sdesc.fragmentSampler[0].addrU = rhi::TEXADDR_WRAP;
    sdesc.fragmentSampler[0].addrV = rhi::TEXADDR_WRAP;
    sdesc.fragmentSampler[0].minFilter = rhi::TEXFILTER_LINEAR;
    sdesc.fragmentSampler[0].magFilter = rhi::TEXFILTER_LINEAR;
    sdesc.fragmentSampler[0].mipFilter = rhi::TEXMIPFILTER_NONE;

    cube.samplerState = rhi::HSamplerState(rhi::SamplerState::Create(sdesc));

    rhi::ShaderCache::UpdateProg(
    rhi::HostApi(), rhi::PROG_VERTEX, FastName("vp-shaded"),
    "vertex_in\n"
    "{\n"
    "    float3 pos    : POSITION;\n"
    "    float3 normal : NORMAL;\n"
    "    float2 uv     : TEXCOORD;\n"
    "};\n"
    "\n"
    "vertex_out\n"
    "{\n"
    "    float4 pos    : SV_POSITION;\n"
    "    float2 uv     : TEXCOORD0;\n"
    "    float4 color  : TEXCOORD1;\n"
    "};\n"
    "\n"
    "[global] property float4x4 ViewProjection;\n"
    "[unique] property float4x4 World;\n"
    "\n"
    "vertex_out\n"
    "vp_main( vertex_in input )\n"
    "{\n"
    "    vertex_out output;\n"
    "\n"
    "    float4 wpos = mul( float4(input.pos.x,input.pos.y,input.pos.z,1.0), World );\n"
    //    "    float  i    = dot( float3(0,0,-1), normalize(mul(float3(input.normal),(float3x3)World)) );\n"
    "    float  i    = dot( float3(0,0,-1), normalize(mul( float4(input.normal.x,input.normal.y,input.normal.z,0.0), World).xyz) );\n"
    "\n"
    "    output.pos    = mul( wpos, ViewProjection );\n"
    "    output.uv     = input.uv;\n"
    "    output.color  = float4(i,i,i,i);\n"
    "\n"
    "    return output;\n"
    "}\n"
    );
    rhi::ShaderCache::UpdateProg(
    rhi::HostApi(), rhi::PROG_FRAGMENT, FastName("fp-shaded"),
    "fragment_in\n"
    "{\n"
    "    float2 uv     : TEXCOORD0;\n"
    "    float4 color  : TEXCOORD1;\n"
    "};\n"
    "\n"
    "fragment_out\n"
    "{\n"
    "    float4 color  : SV_Target;\n"
    "};\n"
    "\n"
    "[unique] property float4 Tint;\n"
    "uniform sampler2D Albedo;\n"
    "\n"
    "fragment_out\n"
    "fp_main( fragment_in input )\n"
    "{\n"
    "    fragment_out output;\n"
    "    float4       diffuse = tex2D( Albedo, input.uv );\n"
    "\n"
    "    output.color = diffuse * input.color * Tint;\n"
    //    "    output.color = input.color + 0.001*diffuse + 0.0001*Tint;\n"
    "    return output;\n"
    "}\n"
    );

    rhi::PipelineState::Descriptor psDesc;

    psDesc.vertexLayout.Clear();
    psDesc.vertexLayout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    psDesc.vertexLayout.AddElement(rhi::VS_NORMAL, 0, rhi::VDT_FLOAT, 3);
    psDesc.vertexLayout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);
    psDesc.vprogUid = FastName("vp-shaded");
    psDesc.fprogUid = FastName("fp-shaded");

    cube.ps = rhi::HPipelineState(rhi::PipelineState::Create(psDesc));
    cube.vp_const[0] = rhi::HConstBuffer(rhi::PipelineState::CreateVertexConstBuffer(cube.ps, 0));
    cube.vp_const[1] = rhi::HConstBuffer(rhi::PipelineState::CreateVertexConstBuffer(cube.ps, 1));
    cube.fp_const = rhi::HConstBuffer(rhi::PipelineState::CreateFragmentConstBuffer(cube.ps, 0));

    rhi::VertexLayout vb_layout;
    vb_layout.Clear();
    vb_layout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    vb_layout.AddElement(rhi::VS_PAD, 0, rhi::VDT_UINT8, 4);
    vb_layout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);
    vb_layout.AddElement(rhi::VS_NORMAL, 0, rhi::VDT_FLOAT, 3);
    cube.vb_layout = rhi::VertexLayout::UniqueId(vb_layout);
    cube.vb_layout = rhi::VertexLayout::InvalidUID;

    rhi::TextureSetDescriptor td;
    td.fragmentTextureCount = 1;
    td.fragmentTexture[0] = cube.tex;
    cube.texSet = rhi::AcquireTextureSet(td);

    cube_t0 = SystemTimer::GetMs();
    cube_angle = 0;
}

void RhiCubeApp::rtInit()
{
    rtQuad.vb = rhi::HVertexBuffer(rhi::VertexBuffer::Create(3 * 2 * sizeof(VertexPT)));

    const VertexPT v[2 * 3] =
    {
      { -1, 1, 0, 0, 1 }, { 1, 1, 0, 1, 1 }, { 1, -1, 0, 1, 0 }, { -1, 1, 0, 0, 1 }, { 1, -1, 0, 1, 0 }, { -1, -1, 0, 0, 0 }
    };

    rhi::VertexBuffer::Update(rtQuad.vb, v, 0, sizeof(v));

    rhi::PipelineState::Descriptor psDesc;

    psDesc.vertexLayout.Clear();
    psDesc.vertexLayout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    psDesc.vertexLayout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);
    psDesc.vprogUid = FastName("vp-copy");
    psDesc.fprogUid = FastName("fp-copy");

    rhi::ShaderCache::UpdateProg(
    rhi::HostApi(), rhi::PROG_VERTEX, FastName("vp-copy"),
    "vertex_in\n"
    "{\n"
    "    float4 position : POSITION;\n"
    "    float2 uv : TEXCOORD0;\n"
    "};\n"
    "vertex_out\n"
    "{\n"
    "    float4 position : SV_POSITION;\n"
    "    float2 uv : TEXCOORD0;\n"
    "};\n"
    "[unique] property float4x4 ViewProjection;\n"
    "[shared] property float4x4 World;\n"
    "vertex_out vp_main( vertex_in input )\n"
    "{\n"
    "    vertex_out output;\n"
    "    float4 wpos = mul( float4(input.position.xyz,1.0), World );\n"
    "    output.position = mul( wpos, ViewProjection );\n"
    "    output.uv = input.uv;\n"
    "    return output;\n"
    "}\n"
    );

    rhi::ShaderCache::UpdateProg(
    rhi::HostApi(), rhi::PROG_FRAGMENT, FastName("fp-copy"),
    "fragment_in\n"
    "{\n"
    "    float2 uv : TEXCOORD0;\n"
    "};\n"
    "fragment_out\n"
    "{\n"
    "    float4 color : SV_TARGET0;"
    "};\n"
    "uniform sampler2D Image;\n"
    "fragment_out fp_main( fragment_in input )\n"
    "{\n"
    "    fragment_out output;\n"
    "    output.color = tex2D( Image, input.uv );"
    "    return output;\n"
    "}\n"
    );

    rtQuad.ps = rhi::HPipelineState(rhi::PipelineState::Create(psDesc));
    rtQuad.vp_const[0] = rhi::HConstBuffer(rhi::PipelineState::CreateVertexConstBuffer(rtQuad.ps, 0));
    rtQuad.vp_const[1] = rhi::HConstBuffer(rhi::PipelineState::CreateVertexConstBuffer(rtQuad.ps, 1));

    rhi::SamplerState::Descriptor sdesc;

    sdesc.fragmentSamplerCount = 1;
    sdesc.fragmentSampler[0].addrU = rhi::TEXADDR_CLAMP;
    sdesc.fragmentSampler[0].addrV = rhi::TEXADDR_CLAMP;
    sdesc.fragmentSampler[0].minFilter = rhi::TEXFILTER_LINEAR;
    sdesc.fragmentSampler[0].magFilter = rhi::TEXFILTER_LINEAR;
    sdesc.fragmentSampler[0].mipFilter = rhi::TEXMIPFILTER_NONE;

    rtQuad.samplerState = rhi::HSamplerState(rhi::SamplerState::Create(sdesc));

    rhi::Texture::Descriptor colorDesc(512, 512, rhi::TEXTURE_FORMAT_R8G8B8A8);
    rhi::Texture::Descriptor depthDesc(512, 512, rhi::TEXTURE_FORMAT_D24S8);

    colorDesc.isRenderTarget = true;

    rtColor0 = rhi::Texture::Create(colorDesc);
    rtColor1 = rhi::Texture::Create(colorDesc);
    rtColor2 = rhi::Texture::Create(colorDesc);
    rtColor3 = rhi::Texture::Create(colorDesc);
    rtDepthStencil = rhi::Texture::Create(depthDesc);

    rhi::TextureSetDescriptor tsDesc;

    tsDesc.fragmentTextureCount = 1;
    tsDesc.fragmentTexture[0] = rhi::HTexture(rtColor0);

    rtQuadBatch0.vertexStreamCount = 1;
    rtQuadBatch0.vertexStream[0] = rtQuad.vb;
    rtQuadBatch0.vertexConstCount = 2;
    rtQuadBatch0.vertexConst[0] = rtQuad.vp_const[0];
    rtQuadBatch0.vertexConst[1] = rtQuad.vp_const[1];
    rtQuadBatch0.fragmentConstCount = 0;
    rtQuadBatch0.renderPipelineState = rtQuad.ps;
    rtQuadBatch0.samplerState = rtQuad.samplerState;
    rtQuadBatch0.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    rtQuadBatch0.primitiveCount = 2;
    rtQuadBatch0.textureSet = rhi::AcquireTextureSet(tsDesc);
}

void RhiCubeApp::mrtInit()
{
    cube_mrt.vb = cube.vb;
    cube_mrt.v_cnt = cube.v_cnt;
    cube_mrt.texSet = cube.texSet;
    cube_mrt.samplerState = cube.samplerState;
    cube_mrt.vb_layout = cube.vb_layout;

    rhi::ShaderCache::UpdateProg(
    rhi::HostApi(), rhi::PROG_VERTEX, FastName("vp-shaded-mrt"),
    "vertex_in\n"
    "{\n"
    "    float3 pos    : POSITION;\n"
    "    float3 normal : NORMAL;\n"
    "    float2 uv     : TEXCOORD;\n"
    "};\n"
    "\n"
    "vertex_out\n"
    "{\n"
    "    float4 pos    : SV_POSITION;\n"
    "    float2 uv     : TEXCOORD0;\n"
    "    float4 color  : TEXCOORD1;\n"
    "    float  depth  : TEXCOORD2;\n"
    "};\n"
    "\n"
    "[global] property float4x4 ViewProjection;\n"
    "[unique] property float4x4 World;\n"
    "\n"
    "vertex_out\n"
    "vp_main( vertex_in input )\n"
    "{\n"
    "    vertex_out output;\n"
    "\n"
    "    float4 wpos = mul( float4(input.pos.x,input.pos.y,input.pos.z,1.0), World );\n"
    //    "    float  i    = dot( float3(0,0,-1), normalize(mul(float3(input.normal),(float3x3)World)) );\n"
    "    float  i    = dot( float3(0,0,-1), normalize(mul( float4(input.normal.x,input.normal.y,input.normal.z,0.0), World).xyz) );\n"
    "\n"
    "    output.pos    = mul( wpos, ViewProjection );\n"
    "    output.uv     = input.uv;\n"
    "    output.color  = float4(i,i,i,i);\n"
    "    output.depth  = wpos.z;\n"
    "\n"
    "    return output;\n"
    "}\n"
    );
    rhi::ShaderCache::UpdateProg(
    rhi::HostApi(), rhi::PROG_FRAGMENT, FastName("fp-shaded-mrt"),
    "fragment_in\n"
    "{\n"
    "    float2 uv     : TEXCOORD0;\n"
    "    float4 color  : TEXCOORD1;\n"
    "    float  depth  : TEXCOORD2;\n"
    "};\n"
    "\n"
    "fragment_out\n"
    "{\n"
    "    float4 color0  : SV_TARGET0;\n"
    "    float4 color1  : SV_TARGET1;\n"
    "    float4 color2  : SV_TARGET2;\n"
    "    float4 color3  : SV_TARGET3;\n"
    "};\n"
    "\n"
    "[unique] property float4 Tint;\n"
    "uniform sampler2D Albedo;\n"
    "\n"
    "fragment_out\n"
    "fp_main( fragment_in input )\n"
    "{\n"
    "    fragment_out output;\n"
    "    float4       diffuse = tex2D( Albedo, input.uv );\n"
    "\n"
    "    output.color0 = diffuse * input.color * Tint;\n"
    "    output.color1 = input.color;\n"
    "    output.color2 = input.color * Tint;\n"
    "    output.color3 = float4(input.depth,0,0,1.0);\n"
    "    return output;\n"
    "}\n"
    );

    rhi::PipelineState::Descriptor psDesc;

    psDesc.vertexLayout.Clear();
    psDesc.vertexLayout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    psDesc.vertexLayout.AddElement(rhi::VS_NORMAL, 0, rhi::VDT_FLOAT, 3);
    psDesc.vertexLayout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);
    psDesc.vprogUid = FastName("vp-shaded-mrt");
    psDesc.fprogUid = FastName("fp-shaded-mrt");

    cube_mrt.ps = rhi::HPipelineState(rhi::PipelineState::Create(psDesc));
    cube_mrt.vp_const[0] = rhi::HConstBuffer(rhi::PipelineState::CreateVertexConstBuffer(cube_mrt.ps, 0));
    cube_mrt.vp_const[1] = rhi::HConstBuffer(rhi::PipelineState::CreateVertexConstBuffer(cube_mrt.ps, 1));
    cube_mrt.fp_const = rhi::HConstBuffer(rhi::PipelineState::CreateFragmentConstBuffer(cube_mrt.ps, 0));

    rhi::TextureSetDescriptor tsDesc;

    tsDesc.fragmentTextureCount = 1;
    tsDesc.fragmentTexture[0] = rhi::HTexture(rtColor1);

    rtQuadBatch1.vertexStreamCount = 1;
    rtQuadBatch1.vertexStream[0] = rtQuad.vb;
    rtQuadBatch1.vertexConstCount = 2;
    rtQuadBatch1.vertexConst[0] = rtQuad.vp_const[0];
    rtQuadBatch1.vertexConst[1] = rtQuad.vp_const[1];
    rtQuadBatch1.fragmentConstCount = 0;
    rtQuadBatch1.renderPipelineState = rtQuad.ps;
    rtQuadBatch1.samplerState = rtQuad.samplerState;
    rtQuadBatch1.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    rtQuadBatch1.primitiveCount = 2;
    rtQuadBatch1.textureSet = rhi::AcquireTextureSet(tsDesc);

    tsDesc.fragmentTextureCount = 1;
    tsDesc.fragmentTexture[0] = rhi::HTexture(rtColor2);

    rtQuadBatch2.vertexStreamCount = 1;
    rtQuadBatch2.vertexStream[0] = rtQuad.vb;
    rtQuadBatch2.vertexConstCount = 2;
    rtQuadBatch2.vertexConst[0] = rtQuad.vp_const[0];
    rtQuadBatch2.vertexConst[1] = rtQuad.vp_const[1];
    rtQuadBatch2.fragmentConstCount = 0;
    rtQuadBatch2.renderPipelineState = rtQuad.ps;
    rtQuadBatch2.samplerState = rtQuad.samplerState;
    rtQuadBatch2.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    rtQuadBatch2.primitiveCount = 2;
    rtQuadBatch2.textureSet = rhi::AcquireTextureSet(tsDesc);

    tsDesc.fragmentTextureCount = 1;
    tsDesc.fragmentTexture[0] = rhi::HTexture(rtColor3);

    rtQuadBatch3.vertexStreamCount = 1;
    rtQuadBatch3.vertexStream[0] = rtQuad.vb;
    rtQuadBatch3.vertexConstCount = 2;
    rtQuadBatch3.vertexConst[0] = rtQuad.vp_const[0];
    rtQuadBatch3.vertexConst[1] = rtQuad.vp_const[1];
    rtQuadBatch3.fragmentConstCount = 0;
    rtQuadBatch3.renderPipelineState = rtQuad.ps;
    rtQuadBatch3.samplerState = rtQuad.samplerState;
    rtQuadBatch3.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    rtQuadBatch3.primitiveCount = 2;
    rtQuadBatch3.textureSet = rhi::AcquireTextureSet(tsDesc);
}

void RhiCubeApp::SetupTank()
{
    SceneFileV2* sceneFile = new SceneFileV2();
    sceneFile->EnableDebugLog(false);
    SceneArchive* archive = sceneFile->LoadSceneArchive("~res:/3d/test.sc2");
    if (!archive)
        return;
    for (int32 i = 0, sz = archive->dataNodes.size(); i < sz; ++i)
    {
        String name = archive->dataNodes[i]->GetString("##name");
        if (name == "PolygonGroup")
        {
            KeyedArchive* keyedArchive = archive->dataNodes[i];
            int32 vertexFormat = keyedArchive->GetInt32("vertexFormat");
            int32 vertexStride = GetVertexSize(vertexFormat);
            int32 vertexCount = keyedArchive->GetInt32("vertexCount");
            int32 indexCount = keyedArchive->GetInt32("indexCount");
            int32 textureCoordCount = keyedArchive->GetInt32("textureCoordCount");
            int32 cubeTextureCoordCount = keyedArchive->GetInt32("cubeTextureCoordCount");
            if (vertexFormat != (EVF_VERTEX | EVF_NORMAL | EVF_TEXCOORD0))
                continue; //for now only this format
            int32 formatPacking = keyedArchive->GetInt32("packing");

            {
                int size = keyedArchive->GetByteArraySize("vertices");
                if (size != vertexCount * vertexStride)
                {
                    Logger::Error("PolygonGroup::Load - Something is going wrong, size of vertex array is incorrect");
                    return;
                }

                const uint8* archiveData = keyedArchive->GetByteArray("vertices");
                rhi::Handle vb = rhi::VertexBuffer::Create(vertexCount * vertexStride);
                rhi::VertexBuffer::Update(vb, archiveData, 0, vertexCount * vertexStride);
                tank.vb.push_back(vb);
                /*uint8 *meshData = new uint8[vertexCount * vertexStride];
                Memcpy(meshData, archiveData, size); //all streams in data required - just copy*/
            }

            int32 indexFormat = keyedArchive->GetInt32("indexFormat");
            {
                int size = keyedArchive->GetByteArraySize("indices");
                uint16* indexArray = new uint16[indexCount];
                const uint8* archiveData = keyedArchive->GetByteArray("indices");
                rhi::Handle ib = rhi::IndexBuffer::Create(rhi::IndexBuffer::Descriptor(indexCount * INDEX_FORMAT_SIZE[indexFormat]));
                rhi::IndexBuffer::Update(ib, archiveData, 0, indexCount * INDEX_FORMAT_SIZE[indexFormat]);
                tank.ib.push_back(ib);
                tank.indCount.push_back(indexCount);
                /*memcpy(indexArray, archiveData, indexCount * INDEX_FORMAT_SIZE[indexFormat]);*/
            }
        }
    }

    Vector<Image*> images;

    ImageSystem::Load("~res:/3d/test.png", images, 0);
    if (images.size())
    {
        Image* img = images[0];
        PixelFormat format = img->GetPixelFormat();
        uint32 w = img->GetWidth();
        uint32 h = img->GetHeight();
        tank.tex = rhi::Texture::Create(rhi::Texture::Descriptor(w, h, rhi::TEXTURE_FORMAT_R8G8B8A8));
        uint8* tex = (uint8*)(rhi::Texture::Map(tank.tex));
        memcpy(tex, img->GetData(), w * h * 4);
        rhi::Texture::Unmap(tank.tex);
    }

    rhi::PipelineState::Descriptor psDesc;

    psDesc.vertexLayout.Clear();
    psDesc.vertexLayout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    psDesc.vertexLayout.AddElement(rhi::VS_NORMAL, 0, rhi::VDT_FLOAT, 3);
    psDesc.vertexLayout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);
    psDesc.vprogUid = FastName("vp-shaded");
    psDesc.fprogUid = FastName("fp-shaded");

    tank.ps = rhi::PipelineState::Create(psDesc);
    tank.vp_const[0] = rhi::PipelineState::CreateVertexConstBuffer(tank.ps, 0);
    tank.vp_const[1] = rhi::PipelineState::CreateVertexConstBuffer(tank.ps, 1);
    tank.fp_const = rhi::PipelineState::CreateFragmentConstBuffer(tank.ps, 0);

    /*rhi::Handle ps;
    rhi::Handle vp_const[2];
    rhi::Handle fp_const;
    rhi::Handle tex;*/
}

void RhiCubeApp::DrawTank()
{
    /*
    rhi::RenderPassConfig   pass_desc;

    pass_desc.colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
    pass_desc.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    pass_desc.colorBuffer[0].clearColor[0] = 0.25f;
    pass_desc.colorBuffer[0].clearColor[1] = 0.25f;
    pass_desc.colorBuffer[0].clearColor[2] = 0.35f;
    pass_desc.colorBuffer[0].clearColor[3] = 1.0f;
    pass_desc.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    pass_desc.depthStencilBuffer.storeAction = rhi::STOREACTION_STORE;

    rhi::Handle cb[1];
    rhi::Handle pass = rhi::RenderPass::Allocate(pass_desc, 1, cb);
    rhi::RenderPass::Begin(pass);
    rhi::CommandBuffer::Begin(cb[0]);    

    float angle = 0.001f*float(SystemTimer::GetMs()) * (30.0f*3.1415f / 180.0f);    

    float clr[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    Matrix4 world;
    Matrix4 view_proj;

    world.Identity();
    world.CreateRotation(Vector3(0, 1, 0), angle);    
    world.SetTranslationVector(Vector3(0, 0, 25));
    //world *= Matrix4::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

    view_proj.Identity();
    view_proj.BuildProjectionFovLH(75.0f, float(VirtualCoordinatesSystem::Instance()->GetPhysicalScreenSize().dx) / float(VirtualCoordinatesSystem::Instance()->GetPhysicalScreenSize().dy), 1.0f, 1000.0f);


    rhi::ConstBuffer::SetConst(tank.fp_const, 0, 1, clr);
    rhi::ConstBuffer::SetConst(tank.vp_const[0], 0, 4, view_proj.data);
    rhi::ConstBuffer::SetConst(tank.vp_const[1], 0, 4, world.data);

    rhi::CommandBuffer::SetPipelineState(cb[0], tank.ps);
    rhi::CommandBuffer::SetVertexConstBuffer(cb[0], 0, tank.vp_const[0]);
    rhi::CommandBuffer::SetVertexConstBuffer(cb[0], 1, tank.vp_const[1]);
    rhi::CommandBuffer::SetFragmentConstBuffer(cb[0], 0, tank.fp_const);
    rhi::CommandBuffer::SetFragmentTexture(cb[0], 0, tank.tex);    

    for (int32 i = 0, sz = tank.vb.size(); i < sz; ++i)
    {
        rhi::CommandBuffer::SetVertexData(cb[0], tank.vb[i]);
        rhi::CommandBuffer::SetIndices(cb[0], tank.ib[i]);
        rhi::CommandBuffer::DrawIndexedPrimitive(cb[0], rhi::PRIMITIVE_TRIANGLELIST, tank.indCount[i]);
    }


    rhi::CommandBuffer::End(cb[0]);
    rhi::RenderPass::End(pass);
*/
}

void RhiCubeApp::SetupInstancedCube()
{
    icube.vb = rhi::HVertexBuffer(rhi::VertexBuffer::Create(3 * 2 * 6 * sizeof(VertexPNT)));

    float sz = 0.2f;
    float u0 = 0.0f;
    float u1 = 1.0f;
    float v0 = 0.0f;
    float v1 = 1.0f;

    VertexPNT v[36] =
    {
      { -sz, -sz, -sz, 0, 0, -1, u0, v1 }, { -sz, sz, -sz, 0, 0, -1, u0, v0 }, { sz, -sz, -sz, 0, 0, -1, u1, v1 }, { -sz, sz, -sz, 0, 0, -1, u0, v0 }, { sz, sz, -sz, 0, 0, -1, u1, v0 }, { sz, -sz, -sz, 0, 0, -1, u1, v1 },

      { sz, -sz, -sz, 1, 0, 0, u0, v1 },
      { sz, sz, -sz, 1, 0, 0, u0, v0 },
      { sz, -sz, sz, 1, 0, 0, u1, v1 },
      { sz, sz, -sz, 1, 0, 0, u0, v0 },
      { sz, sz, sz, 1, 0, 0, u1, v0 },
      { sz, -sz, sz, 1, 0, 0, u1, v1 },

      { sz, -sz, sz, 0, 0, 1, u0, v1 },
      { sz, sz, sz, 0, 0, 1, u0, v0 },
      { -sz, -sz, sz, 0, 0, 1, u1, v1 },
      { sz, sz, sz, 0, 0, 1, u0, v0 },
      { -sz, sz, sz, 0, 0, 1, u1, v0 },
      { -sz, -sz, sz, 0, 0, 1, u1, v1 },

      { -sz, -sz, sz, -1, 0, 0, u0, v1 },
      { -sz, sz, sz, -1, 0, 0, u0, v0 },
      { -sz, sz, -sz, -1, 0, 0, u1, v0 },
      { -sz, sz, -sz, -1, 0, 0, u1, v0 },
      { -sz, -sz, -sz, -1, 0, 0, u1, v1 },
      { -sz, -sz, sz, -1, 0, 0, u0, v1 },

      { -sz, sz, -sz, 0, 1, 0, u0, v1 },
      { -sz, sz, sz, 0, 1, 0, u0, v0 },
      { sz, sz, -sz, 0, 1, 0, u1, v1 },
      { -sz, sz, sz, 0, 1, 0, u0, v0 },
      { sz, sz, sz, 0, 1, 0, u1, v0 },
      { sz, sz, -sz, 0, 1, 0, u1, v1 },

      { -sz, -sz, -sz, 0, -1, 0, u0, v0 },
      { sz, -sz, -sz, 0, -1, 0, u1, v0 },
      { -sz, -sz, sz, 0, -1, 0, u0, v1 },
      { sz, -sz, -sz, 0, -1, 0, u1, v0 },
      { sz, -sz, sz, 0, -1, 0, u1, v1 },
      { -sz, -sz, sz, 0, -1, 0, u0, v1 }
    };
    rhi::VertexBuffer::Update(icube.vb, v, 0, sizeof(v));

    {
        icube.vb2 = rhi::HVertexBuffer(rhi::VertexBuffer::Create(10000 * (4 * 4 * sizeof(float) + sizeof(uint32))));
    }

    rhi::Texture::Descriptor tdesc(128, 128, rhi::TEXTURE_FORMAT_R8G8B8A8);

    //    tdesc.autoGenMipmaps = true;
    icube.tex = rhi::HTexture(rhi::Texture::Create(tdesc));

    uint8* tex = (uint8*)(rhi::Texture::Map(icube.tex));

    if (tex)
    {
        uint8 color1[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
        uint8 color2[4] = { 0x80, 0x80, 0x80, 0xFF };
        //        uint8   color1[4] = { 0xFF, 0x00, 0x00, 0xFF };
        //        uint8   color2[4] = { 0x80, 0x00, 0x00, 0xFF };
        uint32 cell_size = 8;

        for (unsigned y = 0; y != 128; ++y)
        {
            for (unsigned x = 0; x != 128; ++x)
            {
                uint8* p = tex + y * sizeof(uint32) * 128 + x * sizeof(uint32);
                uint8* c = (((y / cell_size) & 0x1) ^ ((x / cell_size) & 0x1)) ? color1 : color2;

                memcpy(p, c, sizeof(uint32));
            }
        }

        rhi::Texture::Unmap(icube.tex);
    }

    rhi::SamplerState::Descriptor sdesc;

    sdesc.fragmentSamplerCount = 1;
    sdesc.fragmentSampler[0].addrU = rhi::TEXADDR_WRAP;
    sdesc.fragmentSampler[0].addrV = rhi::TEXADDR_WRAP;
    sdesc.fragmentSampler[0].minFilter = rhi::TEXFILTER_LINEAR;
    sdesc.fragmentSampler[0].magFilter = rhi::TEXFILTER_LINEAR;
    sdesc.fragmentSampler[0].mipFilter = rhi::TEXMIPFILTER_NONE;

    icube.samplerState = rhi::HSamplerState(rhi::SamplerState::Create(sdesc));

    rhi::ShaderCache::UpdateProg(
    rhi::HostApi(), rhi::PROG_VERTEX, FastName("vp-i-shaded"),
    "VPROG_IN_BEGIN\n"
    "    VPROG_IN_STREAM_VERTEX\n"
    "      VPROG_IN_POSITION\n"
    "      VPROG_IN_NORMAL\n"
    "      VPROG_IN_TEXCOORD\n"
    "    VPROG_IN_STREAM_INSTANCE\n"
    "      VPROG_IN_TEXCOORD4(4)\n"
    "      VPROG_IN_TEXCOORD5(4)\n"
    "      VPROG_IN_TEXCOORD6(4)\n"
    "      VPROG_IN_TEXCOORD7(4)\n"
    "      VPROG_IN_COLOR\n"
    "VPROG_IN_END\n"
    "\n"
    "VPROG_OUT_BEGIN\n"
    "    VPROG_OUT_POSITION\n"
    "    VPROG_OUT_TEXCOORD0(uv,2)\n"
    "    VPROG_OUT_TEXCOORD1(color,4)\n"
    "VPROG_OUT_END\n"
    "\n"
    "DECL_VPROG_BUFFER(0,16)\n"
    "\n"
    "VPROG_BEGIN\n"
    "\n"
    "    float3 in_pos      = VP_IN_POSITION.xyz;\n"
    "    float3 in_normal   = VP_IN_NORMAL;\n"
    "    float2 in_texcoord = VP_IN_TEXCOORD;\n"
    "    float4 in_color    = VP_IN_COLOR;\n"
    "    float4x4 ViewProjection = float4x4( VP_Buffer0[0], VP_Buffer0[1], VP_Buffer0[2], VP_Buffer0[3] );\n"
    "    float4x4 World = float4x4( VP_IN_TEXCOORD4, VP_IN_TEXCOORD5, VP_IN_TEXCOORD6, VP_IN_TEXCOORD7 );\n"
    "    float3x3 World3 = float3x3( VP_IN_TEXCOORD4.xyz, VP_IN_TEXCOORD5.xyz, VP_IN_TEXCOORD6.xyz );"
    "    float4 wpos = mul( float4(in_pos.x,in_pos.y,in_pos.z,1.0), World );\n"
    "    float i   = dot( float3(0,0,-1), normalize(mul(float3(in_normal),World3)) );\n"
    "    VP_OUT_POSITION   = mul( wpos, ViewProjection );\n"
    "    VP_OUT(uv)        = in_texcoord;\n"
    "    VP_OUT(color)     = float4(i,i,i,1.0) * in_color;\n"
    "\n"
    "VPROG_END\n");
    rhi::ShaderCache::UpdateProg(
    rhi::HostApi(), rhi::PROG_FRAGMENT, FastName("fp-i-shaded"),
    "FPROG_IN_BEGIN\n"
    "FPROG_IN_TEXCOORD0(uv,2)\n"
    "FPROG_IN_TEXCOORD1(color,4)\n"
    "FPROG_IN_END\n"
    "\n"
    "FPROG_OUT_BEGIN\n"
    "    FPROG_OUT_COLOR\n"
    "FPROG_OUT_END\n"
    "\n"
    "DECL_FP_SAMPLER2D(0)\n"
    "\n"
    "\n"
    //    "DECL_FPROG_BUFFER(0,4)\n"
    "\n"
    "FPROG_BEGIN\n"
    "    float4  diffuse = FP_TEXTURE2D( 0, FP_IN(uv) );\n"
    "    FP_OUT_COLOR = diffuse * FP_IN(color);\n"
    "FPROG_END\n");

    rhi::PipelineState::Descriptor psDesc;

    psDesc.vertexLayout.Clear();
    psDesc.vertexLayout.AddElement(rhi::VS_POSITION, 0, rhi::VDT_FLOAT, 3);
    psDesc.vertexLayout.AddElement(rhi::VS_NORMAL, 0, rhi::VDT_FLOAT, 3);
    psDesc.vertexLayout.AddElement(rhi::VS_TEXCOORD, 0, rhi::VDT_FLOAT, 2);
    psDesc.vertexLayout.AddStream(rhi::VDF_PER_INSTANCE);
    psDesc.vertexLayout.AddElement(rhi::VS_TEXCOORD, 4, rhi::VDT_FLOAT, 4);
    psDesc.vertexLayout.AddElement(rhi::VS_TEXCOORD, 5, rhi::VDT_FLOAT, 4);
    psDesc.vertexLayout.AddElement(rhi::VS_TEXCOORD, 6, rhi::VDT_FLOAT, 4);
    psDesc.vertexLayout.AddElement(rhi::VS_TEXCOORD, 7, rhi::VDT_FLOAT, 4);
    psDesc.vertexLayout.AddElement(rhi::VS_COLOR, 0, rhi::VDT_UINT8N, 4);
    psDesc.vertexLayout.Dump();
    psDesc.vprogUid = FastName("vp-i-shaded");
    psDesc.fprogUid = FastName("fp-i-shaded");

    icube.ps = rhi::HPipelineState(rhi::PipelineState::Create(psDesc));
    icube.vp_const[0] = rhi::HConstBuffer(rhi::PipelineState::CreateVertexConstBuffer(icube.ps, 0));
    //    icube.vp_const[1] = rhi::HConstBuffer(rhi::PipelineState::CreateVertexConstBuffer(icube.ps, 1));
    //    icube.fp_const = rhi::HConstBuffer(rhi::PipelineState::CreateFragmentConstBuffer(icube.ps, 0));

    rhi::TextureSetDescriptor td;
    td.fragmentTextureCount = 1;
    td.fragmentTexture[0] = icube.tex;
    icube.texSet = rhi::AcquireTextureSet(td);

    icube_t0 = SystemTimer::GetMs();
    icube_angle = 0;
}

void RhiCubeApp::DrawInstancedCube()
{
//    SCOPED_NAMED_TIMING("app-draw");
    #define USE_SECOND_CB 1

    rhi::RenderPassConfig pass_desc;
    float clr[4] = { 1.0f, 0.6f, 0.0f, 1.0f };
    Matrix4 view;
    Matrix4 projection;
    VirtualCoordinatesSystem* vcs = DAVA::UIControlSystem::Instance()->vcs;

    StatSet::ResetAll();

    projection.Identity();
    view.BuildProjectionFovLH(75.0f, float(vcs->GetPhysicalScreenSize().dx) / float(vcs->GetPhysicalScreenSize().dy), 1.0f, 1000.0f);

    DbgDraw::SetScreenSize(vcs->GetPhysicalScreenSize().dx, vcs->GetPhysicalScreenSize().dy);

    {
        char title[128] = "RHI Cube  -  ";

        switch (rhi::HostApi())
        {
        case rhi::RHI_DX9:
            strcat(title, "DX9");
            break;
        case rhi::RHI_DX11:
            strcat(title, "DX11");
            break;
        case rhi::RHI_GLES2:
            strcat(title, "GL");
            break;
        case rhi::RHI_METAL:
            strcat(title, "Metal");
            break;
        }
        DbgDraw::SetNormalTextSize();
        //    DbgDraw::SetSmallTextSize();
        DbgDraw::Text2D(10, 50, 0xFFFFFFFF, title);
    }

    pass_desc.colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
    pass_desc.colorBuffer[0].storeAction = rhi::STOREACTION_NONE;
    pass_desc.colorBuffer[0].clearColor[0] = 0.25f;
    pass_desc.colorBuffer[0].clearColor[1] = 0.25f;
    pass_desc.colorBuffer[0].clearColor[2] = 0.35f;
    pass_desc.colorBuffer[0].clearColor[3] = 1.0f;
    pass_desc.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    pass_desc.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;

    rhi::HPacketList pl[2];
    #if USE_SECOND_CB
    rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 2, pl);
    #else
    rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 1, pl);
    #endif

    rhi::RenderPass::Begin(pass);
    rhi::BeginPacketList(pl[0]);

    uint64 icube_t1 = SystemTimer::GetMs();
    uint64 dt = icube_t1 - icube_t0;

    icube_angle += 0.001f * float(dt) * (30.0f * 3.1415f / 180.0f);
    icube_t0 = icube_t1;

    Matrix4 world;
    Matrix4 view_proj;

    world.Identity();
    world.CreateRotation(Vector3(0, 1, 0), icube_angle);
    //    world.CreateRotation( Vector3(1,0,0), icube_angle );
    world.SetTranslationVector(Vector3(0, -0.7f, 5));
    //world *= Matrix4::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

    view_proj.Identity();
    view_proj.BuildProjectionFovLH(75.0f, float(vcs->GetPhysicalScreenSize().dx) / float(vcs->GetPhysicalScreenSize().dy), 1.0f, 1000.0f);

    //    rhi::ConstBuffer::SetConst(icube.fp_const, 0, 1, clr);
    rhi::ConstBuffer::SetConst(icube.vp_const[0], 0, 4, view_proj.data);
    //    rhi::ConstBuffer::SetConst(icube.vp_const[1], 0, 4, world.data);

    rhi::Packet packet;

    packet.vertexStreamCount = 2;
    packet.vertexStream[0] = icube.vb;
    packet.vertexStream[1] = icube.vb2;
    packet.renderPipelineState = icube.ps;
    packet.vertexConstCount = 1;
    packet.vertexConst[0] = icube.vp_const[0];
    //    packet.vertexConst[1] = icube.vp_const[1];
    packet.fragmentConstCount = 0;
    //    packet.fragmentConst[0] = icube.fp_const;
    packet.textureSet = icube.texSet;
    packet.samplerState = icube.samplerState;
    packet.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    packet.primitiveCount = 12;

    //    rhi::UpdateConstBuffer4fv(icube.fp_const, 0, clr, 1);
    rhi::UpdateConstBuffer4fv(icube.vp_const[0], 0, view_proj.data, 4);
//    rhi::UpdateConstBuffer4fv(icube.vp_const[1], 0, world.data, 4);
//    rhi::AddPacket(pl[0], packet);

    #if USE_SECOND_CB
    {
        const unsigned row_cnt = 200;
        const unsigned col_cnt = 12;
        const float w = 0.5f * float(col_cnt);

        rhi::BeginPacketList(pl[1]);

        packet.instanceCount = row_cnt * col_cnt;

        unsigned inst_sz = (4 * 4 * sizeof(float) + sizeof(uint32));
        uint8* inst = (uint8*)rhi::MapVertexBuffer(icube.vb2, 0, packet.instanceCount * inst_sz);
        for (unsigned z = 0; z != row_cnt; ++z)
        {
            for (unsigned i = 0; i != col_cnt; ++i)
            {
                const uint32 clr = (z * row_cnt + i + 1) * 0x775511; // 0x15015

                world.Identity();
                world.CreateRotation(Vector3(1, 0, 0), icube_angle);
                world.SetTranslationVector(Vector3(-0.5f * w + float(i) * (w / float(col_cnt)), 1 - z * 0.4f, 10 + float(z) * w));

                memcpy(inst, world.data, 4 * 4 * sizeof(float));
                memcpy(inst + 4 * 4 * sizeof(float), &clr, sizeof(uint32));

                inst += inst_sz;
            }
        }
        rhi::UnmapVertexBuffer(icube.vb2);
        rhi::AddPacket(pl[1], packet);
        rhi::EndPacketList(pl[1]);
    }
    #endif

    DbgDraw::FlushBatched(pl[0]);

    rhi::EndPacketList(pl[0]);

    rhi::RenderPass::End(pass);

    #undef USE_SECOND_CB
}

void RhiCubeApp::rhiDraw()
{
//    SCOPED_NAMED_TIMING("RhiCubeApp::Draw");
//-    ApplicationCore::BeginFrame();

#define DRAW_TANK 0


#if DRAW_TANK
    DrawTank();
    return;
#endif


#define USE_SECOND_CB 0

    rhi::RenderPassConfig pass_desc;

    pass_desc.colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
    pass_desc.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
    pass_desc.colorBuffer[0].clearColor[0] = 0.25f;
    pass_desc.colorBuffer[0].clearColor[1] = 0.25f;
    pass_desc.colorBuffer[0].clearColor[2] = 0.35f;
    pass_desc.colorBuffer[0].clearColor[3] = 1.0f;
    pass_desc.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    pass_desc.depthStencilBuffer.storeAction = rhi::STOREACTION_STORE;

    rhi::Handle cb[2];
#if USE_SECOND_CB
    rhi::Handle pass = rhi::RenderPass::Allocate(pass_desc, 2, cb);
#else
    rhi::Handle pass = rhi::RenderPass::Allocate(pass_desc, 1, cb);
#endif
    float clr[4] = { 1.0f, 0.6f, 0.0f, 1.0f };

    rhi::RenderPass::Begin(pass);
    rhi::CommandBuffer::Begin(cb[0]);

#if 1

    rhi::ConstBuffer::SetConst(triangle.fp_const, 0, 1, clr);

    rhi::CommandBuffer::SetPipelineState(cb[0], triangle.ps);
    rhi::CommandBuffer::SetVertexData(cb[0], triangle.vb);
    rhi::CommandBuffer::SetIndices(cb[0], triangle.ib);
    rhi::CommandBuffer::SetFragmentConstBuffer(cb[0], 0, triangle.fp_const);
    rhi::CommandBuffer::DrawIndexedPrimitive(cb[0], rhi::PRIMITIVE_TRIANGLELIST, 1, 3);
    
#else

    uint64 cube_t1 = SystemTimer::GetMs();
    uint64 dt = cube_t1 - cube_t0;

    cube_angle += 0.001f * float(dt) * (30.0f * 3.1415f / 180.0f);
    cube_t0 = cube_t1;

    Matrix4 world;
    Matrix4 view_proj;

    world.Identity();
    world.CreateRotation(Vector3(0, 1, 0), cube_angle);
    //    world.CreateRotation( Vector3(1,0,0), cube_angle );
    world.SetTranslationVector(Vector3(0, 0, 5));
    //world *= Matrix4::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

    view_proj.Identity();
    view_proj.BuildProjectionFovLH(75.0f, float(VirtualCoordinatesSystem::Instance()->GetPhysicalScreenSize().dx) / float(VirtualCoordinatesSystem::Instance()->GetPhysicalScreenSize().dy), 1.0f, 1000.0f);

    rhi::ConstBuffer::SetConst(cube.fp_const, 0, 1, clr);
    rhi::ConstBuffer::SetConst(cube.vp_const[0], 0, 4, view_proj.data);
    rhi::ConstBuffer::SetConst(cube.vp_const[1], 0, 4, world.data);

    rhi::CommandBuffer::SetPipelineState(cb[0], cube.ps);
    rhi::CommandBuffer::SetVertexConstBuffer(cb[0], 0, cube.vp_const[0]);
    rhi::CommandBuffer::SetVertexConstBuffer(cb[0], 1, cube.vp_const[1]);
    rhi::CommandBuffer::SetFragmentConstBuffer(cb[0], 0, cube.fp_const);
    rhi::CommandBuffer::SetFragmentTexture(cb[0], 0, cube.tex);
    rhi::CommandBuffer::SetVertexData(cb[0], cube.vb);
    rhi::CommandBuffer::DrawPrimitive(cb[0], rhi::PRIMITIVE_TRIANGLELIST, 12);    

    #if USE_SECOND_CB
    {
        const float w = 3.0f;
        const unsigned n = 5;

        rhi::CommandBuffer::Begin(cb[1]);
        for (unsigned i = 0; i != n; ++i)
        {
            const uint32 c = (i + 1) * 0x775511; // 0x15015
            const uint8* cc = (const uint8*)(&c);
            const float clr2[] = { float(cc[2]) / 255.0f, float(cc[1]) / 255.0f, float(cc[0]) / 255.0f, 1.0f };

            world.Identity();
            world.CreateRotation(Vector3(1, 0, 0), cube_angle);
            world.SetTranslationVector(Vector3(-0.5f * w + float(i) * (w / float(n)), 1, 10));

            rhi::ConstBuffer::SetConst(cube.fp_const, 0, 1, clr2);
            rhi::ConstBuffer::SetConst(cube.vp_const[1], 0, 4, world.data);

            rhi::CommandBuffer::SetPipelineState(cb[1], cube.ps);
            rhi::CommandBuffer::SetVertexData(cb[1], cube.vb);
            rhi::CommandBuffer::SetVertexConstBuffer(cb[1], 0, cube.vp_const[0]);
            rhi::CommandBuffer::SetVertexConstBuffer(cb[1], 1, cube.vp_const[1]);
            rhi::CommandBuffer::SetFragmentConstBuffer(cb[1], 0, cube.fp_const);
            rhi::CommandBuffer::SetFragmentTexture(cb[1], 0, cube.tex);
            rhi::CommandBuffer::DrawPrimitive(cb[1], rhi::PRIMITIVE_TRIANGLELIST, 12);
        }
        rhi::CommandBuffer::End(cb[1]);
    }
    #endif
    
#endif

    rhi::CommandBuffer::End(cb[0]);

    rhi::RenderPass::End(pass);

    #undef USE_SECOND_CB
}

void RhiCubeApp::manticoreDraw()
{
//    SCOPED_NAMED_TIMING("app-draw");
    #define USE_SECOND_CB 0

    rhi::RenderPassConfig pass_desc;
    float clr[4] = { 1.0f, 0.6f, 0.0f, 1.0f };
    Matrix4 view;
    Matrix4 projection;
    VirtualCoordinatesSystem* vcs = DAVA::UIControlSystem::Instance()->vcs;

    StatSet::ResetAll();

    projection.Identity();
    view.BuildProjectionFovLH(75.0f, float(vcs->GetPhysicalScreenSize().dx) / float(vcs->GetPhysicalScreenSize().dy), 1.0f, 1000.0f);

    DbgDraw::SetScreenSize(vcs->GetPhysicalScreenSize().dx, vcs->GetPhysicalScreenSize().dy);

    {
        char title[128] = "RHI Cube  -  ";

        switch (rhi::HostApi())
        {
        case rhi::RHI_DX9:
            strcat(title, "DX9");
            break;
        case rhi::RHI_DX11:
            strcat(title, "DX11");
            break;
        case rhi::RHI_GLES2:
            strcat(title, "GL");
            break;
        case rhi::RHI_METAL:
            strcat(title, "Metal");
            break;
        }
        //        DbgDraw::SetNormalTextSize();
        //    DbgDraw::SetSmallTextSize();
        DbgDraw::Text2D(10, 50, 0xFFFFFFFF, title);
    }

    pass_desc.colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
    pass_desc.colorBuffer[0].storeAction = rhi::STOREACTION_NONE;
    pass_desc.colorBuffer[0].clearColor[0] = 0.25f;
    pass_desc.colorBuffer[0].clearColor[1] = 0.25f;
    pass_desc.colorBuffer[0].clearColor[2] = 0.35f;
    pass_desc.colorBuffer[0].clearColor[3] = 1.0f;
    pass_desc.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    pass_desc.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;

    rhi::HPacketList pl[2];
    #if USE_SECOND_CB
    rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 2, pl);
    #else
    rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 1, pl);
    #endif

    /*
    if (perfQuerySetFired)
    {
        bool ready = false;
        bool valid = false;

        rhi::GetPerfQuerySetStatus(perfQuerySet, &ready, &valid);

        if (ready && valid)
        {
            uint64 freq = 0;
            uint64 t0, t1;

            rhi::GetPerfQuerySetFreq(perfQuerySet, &freq);
            //            Logger::Info("perf-query:  freq= %u",uint32(freq));

            rhi::GetPerfQuerySetFrameTimestamps(perfQuerySet, &t0, &t1);

            Logger::Info("GPU frame = %.3f ms", float(t1 - t0) / float(freq / 1000));

            perfQuerySetFired = false;
        }
    }

    if (!perfQuerySetFired)
    {
        rhi::ResetPerfQuerySet(perfQuerySet);
        rhi::SetFramePerfQuerySet(perfQuerySet);
        perfQuerySetFired = true;
    }
*/
    rhi::RenderPass::Begin(pass);
    rhi::BeginPacketList(pl[0]);

#if 0
    
    rhi::Packet packet;

    packet.vertexStreamCount    = 1;
    packet.vertexStream[0]      = triangle.vb;
    packet.vertexCount          = triangle.v_cnt;
    packet.indexBuffer          = triangle.ib;
    packet.renderPipelineState  = triangle.ps;
    packet.vertexConstCount     = 0;
    packet.fragmentConstCount   = 1;
    packet.fragmentConst[0]     = triangle.fp_const;
    packet.primitiveType        = rhi::PRIMITIVE_TRIANGLELIST;
    packet.primitiveCount       = 1;

    rhi::ConstBuffer::SetConst( triangle.fp_const, 0, 1, clr );
    rhi::AddPacket( pl[0], packet );

#else

    uint64 cube_t1 = SystemTimer::GetMs();
    uint64 dt = cube_t1 - cube_t0;

    cube_angle += 0.001f * float(dt) * (30.0f * 3.1415f / 180.0f);
    cube_t0 = cube_t1;

    Matrix4 world;
    Matrix4 view_proj;

    world.Identity();
    world.CreateRotation(Vector3(0, 1, 0), cube_angle);
    //    world.CreateRotation( Vector3(1,0,0), cube_angle );
    world.SetTranslationVector(Vector3(0, -0.7f, 5));
    //world *= Matrix4::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

    view_proj.Identity();
    view_proj.BuildProjectionFovLH(75.0f, float(vcs->GetPhysicalScreenSize().dx) / float(vcs->GetPhysicalScreenSize().dy), 1.0f, 1000.0f);

    rhi::ConstBuffer::SetConst(cube.fp_const, 0, 1, clr);
    rhi::ConstBuffer::SetConst(cube.vp_const[0], 0, 4, view_proj.data);
    rhi::ConstBuffer::SetConst(cube.vp_const[1], 0, 4, world.data);

    rhi::Packet packet;

    packet.vertexStreamCount = 1;
    packet.vertexStream[0] = cube.vb;
    ///    packet.vertexLayoutUID = cube.vb_layout;
    packet.renderPipelineState = cube.ps;
    packet.vertexConstCount = 2;
    packet.vertexConst[0] = cube.vp_const[0];
    packet.vertexConst[1] = cube.vp_const[1];
    packet.fragmentConstCount = 1;
    packet.fragmentConst[0] = cube.fp_const;
    packet.textureSet = cube.texSet;
    packet.samplerState = cube.samplerState;
    packet.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    packet.primitiveCount = 12;

    rhi::UpdateConstBuffer4fv(cube.fp_const, 0, clr, 1);
    rhi::UpdateConstBuffer4fv(cube.vp_const[0], 0, view_proj.data, 4);
    rhi::UpdateConstBuffer4fv(cube.vp_const[1], 0, world.data, 4);
    rhi::AddPacket(pl[0], packet);

    #if USE_SECOND_CB
    {
        //        packet.options |= rhi::Packet::OPT_WIREFRAME;
        const unsigned row_cnt = 200;
        const unsigned col_cnt = 12;
        //const unsigned  row_cnt = 1;
        //const unsigned  col_cnt = 2;
        const float w = 0.5f * float(col_cnt);

        rhi::BeginPacketList(pl[1]);
        for (unsigned z = 0; z != row_cnt; ++z)
        {
            for (unsigned i = 0; i != col_cnt; ++i)
            {
                const uint32 c = (z * row_cnt + i + 1) * 0x775511; // 0x15015
                const uint8* cc = (const uint8*)(&c);
                const float clr2[] = { float(cc[2]) / 255.0f, float(cc[1]) / 255.0f, float(cc[0]) / 255.0f, 1.0f };

                START_NAMED_TIMING("app.cb--upd");
                world.Identity();
                world.CreateRotation(Vector3(1, 0, 0), cube_angle);
                world.SetTranslationVector(Vector3(-0.5f * w + float(i) * (w / float(col_cnt)), 1 - z * 0.4f, 10 + float(z) * w));

                rhi::UpdateConstBuffer4fv(cube.fp_const, 0, clr2, 1);
                //            rhi::UpdateConstBuffer( cube.vp_const[0], 0, view_proj.data, 4 );
                rhi::UpdateConstBuffer4fv(cube.vp_const[1], 0, world.data, 4);
                STOP_NAMED_TIMING("app.cb--upd");
                rhi::AddPacket(pl[1], packet);
            }
        }
        rhi::EndPacketList(pl[1]);
    }
    #endif

#endif

    DbgDraw::FlushBatched(pl[0]);

    rhi::EndPacketList(pl[0]);

    rhi::RenderPass::End(pass);

    #undef USE_SECOND_CB
}

void RhiCubeApp::visibilityTestDraw()
{
    rhi::RenderPassConfig pass_desc;
    float clr[4] = { 1.0f, 0.6f, 0.0f, 1.0f };
    Matrix4 view;
    Matrix4 projection;
    static bool visiblityTestDone = false;
    static int visiblityTestRepeatTTW = 0;
    static rhi::HQueryBuffer visibilityBuffer(rhi::InvalidHandle);
    VirtualCoordinatesSystem* vcs = DAVA::UIControlSystem::Instance()->vcs;

    StatSet::ResetAll();

    projection.Identity();
    view.BuildProjectionFovLH(75.0f, float(vcs->GetPhysicalScreenSize().dx) / float(vcs->GetPhysicalScreenSize().dy), 1.0f, 1000.0f);

    DbgDraw::SetScreenSize(vcs->GetPhysicalScreenSize().dx, vcs->GetPhysicalScreenSize().dy);

    {
        char title[128] = "RHI Cube  -  ";

        switch (rhi::HostApi())
        {
        case rhi::RHI_DX9:
            strcat(title, "DX9");
            break;
        case rhi::RHI_DX11:
            strcat(title, "DX11");
            break;
        case rhi::RHI_GLES2:
            strcat(title, "GL");
            break;
        case rhi::RHI_METAL:
            strcat(title, "Metal");
            break;
        }
        DbgDraw::SetNormalTextSize();
        //    DbgDraw::SetSmallTextSize();
        DbgDraw::Text2D(10, 50, 0xFFFFFFFF, title);

        if (visiblityTestDone)
        {
            for (unsigned i = 0; i != 2; ++i)
            {
                if (rhi::QueryIsReady(visibilityBuffer, i))
                {
                    DbgDraw::Text2D(10, 80 + i * (DbgDraw::NormalCharH + 1), 0xFFFFFFFF, "obj#%u = %i", i, rhi::QueryValue(visibilityBuffer, i));
                }
                else
                {
                    DbgDraw::Text2D(10, 80 + i * (DbgDraw::NormalCharH + 1), 0xFFFFFFFF, "obj#%u = not ready", i);
                }
            }
        }
    }

    pass_desc.colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
    pass_desc.colorBuffer[0].storeAction = rhi::STOREACTION_NONE;
    pass_desc.colorBuffer[0].clearColor[0] = 0.25f;
    pass_desc.colorBuffer[0].clearColor[1] = 0.25f;
    pass_desc.colorBuffer[0].clearColor[2] = 0.35f;
    pass_desc.colorBuffer[0].clearColor[3] = 1.0f;
    pass_desc.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
    pass_desc.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;

    if (!visiblityTestDone)
    {
        if (!visibilityBuffer.IsValid())
        {
            visibilityBuffer = rhi::CreateQueryBuffer(16);
        }

        pass_desc.queryBuffer = visibilityBuffer;
    }

    rhi::HPacketList pl[2];
    rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 1, pl);

    rhi::RenderPass::Begin(pass);
    rhi::BeginPacketList(pl[0]);

    uint64 cube_t1 = SystemTimer::GetMs();
    uint64 dt = cube_t1 - cube_t0;

    cube_angle += 0.001f * float(dt) * (30.0f * 3.1415f / 180.0f);
    cube_t0 = cube_t1;

    Matrix4 world;
    Matrix4 view_proj;

    world.Identity();
    world.CreateRotation(Vector3(0, 1, 0), cube_angle);
    //    world.CreateRotation( Vector3(1,0,0), cube_angle );
    world.SetTranslationVector(Vector3(0, 0, 5));
    //world *= Matrix4::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

    view_proj.Identity();
    view_proj.BuildProjectionFovLH(75.0f, float(vcs->GetPhysicalScreenSize().dx) / float(vcs->GetPhysicalScreenSize().dy), 1.0f, 1000.0f);

    rhi::ConstBuffer::SetConst(cube.fp_const, 0, 1, clr);
    rhi::ConstBuffer::SetConst(cube.vp_const[0], 0, 4, view_proj.data);
    rhi::ConstBuffer::SetConst(cube.vp_const[1], 0, 4, world.data);

    rhi::Packet packet;

    packet.vertexStreamCount = 1;
    packet.vertexStream[0] = cube.vb;
    packet.vertexLayoutUID = cube.vb_layout;
    //-    packet.indexBuffer          = rhi::InvalidHandle;
    packet.renderPipelineState = cube.ps;
    packet.vertexConstCount = 2;
    packet.vertexConst[0] = cube.vp_const[0];
    packet.vertexConst[1] = cube.vp_const[1];
    packet.fragmentConstCount = 1;
    packet.fragmentConst[0] = cube.fp_const;
    packet.textureSet = cube.texSet;
    packet.samplerState = cube.samplerState;
    packet.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
    packet.primitiveCount = 12;

    rhi::UpdateConstBuffer4fv(cube.fp_const, 0, clr, 1);
    rhi::UpdateConstBuffer4fv(cube.vp_const[0], 0, view_proj.data, 4);
    rhi::UpdateConstBuffer4fv(cube.vp_const[1], 0, world.data, 4);
    if (!visiblityTestDone)
        packet.queryIndex = 0;
    rhi::AddPacket(pl[0], packet);

    world.SetTranslationVector(Vector3(0, 0, 20));
    rhi::UpdateConstBuffer4fv(cube.vp_const[1], 0, world.data, 4);
    if (!visiblityTestDone)
        packet.queryIndex = 1;
    rhi::AddPacket(pl[0], packet);

    DbgDraw::FlushBatched(pl[0]);

    rhi::EndPacketList(pl[0]);

    rhi::RenderPass::End(pass);

    if (!visiblityTestDone)
    {
        visiblityTestDone = true;
        visiblityTestRepeatTTW = 5;
    }

    if (visiblityTestDone)
    {
        if (--visiblityTestRepeatTTW < 0)
            visiblityTestDone = false;
    }
}

void RhiCubeApp::rtDraw()
{
    #define USE_SECOND_CB 1
    #define USE_RT 1

    // draw scene into render-target
    {
        rhi::RenderPassConfig pass_desc;
        float clr[4] = { 1.0f, 0.6f, 0.0f, 1.0f };

    #if USE_RT
        pass_desc.colorBuffer[0].texture = rtColor0;
        pass_desc.depthStencilBuffer.texture = rtDepthStencil;
    #endif
        pass_desc.colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
        pass_desc.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
        pass_desc.colorBuffer[0].clearColor[0] = 0.25f;
        pass_desc.colorBuffer[0].clearColor[1] = 0.25f;
        pass_desc.colorBuffer[0].clearColor[2] = 0.35f;
        pass_desc.colorBuffer[0].clearColor[3] = 1.0f;
        pass_desc.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
        pass_desc.depthStencilBuffer.storeAction = rhi::STOREACTION_STORE;

        rhi::HPacketList pl[2];
    #if USE_SECOND_CB
        rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 2, pl);
    #else
        rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 1, pl);
    #endif

        rhi::RenderPass::Begin(pass);
        rhi::BeginPacketList(pl[0]);

        uint64 cube_t1 = SystemTimer::GetMs();
        uint64 dt = cube_t1 - cube_t0;

        cube_angle += 0.001f * float(dt) * (30.0f * 3.1415f / 180.0f);
        cube_t0 = cube_t1;

        Matrix4 world;
        Matrix4 view_proj;
        VirtualCoordinatesSystem* vcs = DAVA::UIControlSystem::Instance()->vcs;

        world.Identity();
        world.CreateRotation(Vector3(0, 1, 0), cube_angle);
        //    world.CreateRotation( Vector3(1,0,0), cube_angle );
        world.SetTranslationVector(Vector3(0, 0, 5));
        //world *= Matrix4::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

        view_proj.Identity();
        view_proj.BuildProjectionFovLH(75.0f, float(vcs->GetPhysicalScreenSize().dx) / float(vcs->GetPhysicalScreenSize().dy), 1.0f, 1000.0f);

        rhi::ConstBuffer::SetConst(cube.fp_const, 0, 1, clr);
        rhi::ConstBuffer::SetConst(cube.vp_const[0], 0, 4, view_proj.data);
        rhi::ConstBuffer::SetConst(cube.vp_const[1], 0, 4, world.data);

        rhi::Packet packet;

        packet.vertexStreamCount = 1;
        packet.vertexStream[0] = cube.vb;
        packet.renderPipelineState = cube.ps;
        packet.vertexConstCount = 2;
        packet.vertexConst[0] = cube.vp_const[0];
        packet.vertexConst[1] = cube.vp_const[1];
        packet.fragmentConstCount = 1;
        packet.fragmentConst[0] = cube.fp_const;
        packet.textureSet = cube.texSet;
        packet.samplerState = cube.samplerState;
        packet.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
        packet.primitiveCount = 12;

        rhi::UpdateConstBuffer4fv(cube.fp_const, 0, clr, 1);
        rhi::UpdateConstBuffer4fv(cube.vp_const[0], 0, view_proj.data, 4);
        rhi::UpdateConstBuffer4fv(cube.vp_const[1], 0, world.data, 4);
        rhi::AddPacket(pl[0], packet);

    #if USE_SECOND_CB
        {
            const float w = 5.0f;
            const unsigned row_cnt = 30;
            const unsigned col_cnt = 7;

            rhi::BeginPacketList(pl[1]);
            for (unsigned z = 0; z != row_cnt; ++z)
            {
                for (unsigned i = 0; i != col_cnt; ++i)
                {
                    const uint32 c = (z * col_cnt + i + 1) * 0x775511; // 0x15015
                    const uint8* cc = (const uint8*)(&c);
                    const float clr2[] = { float(cc[2]) / 255.0f, float(cc[1]) / 255.0f, float(cc[0]) / 255.0f, 1.0f };

                    world.Identity();
                    world.CreateRotation(Vector3(1, 0, 0), cube_angle);
                    world.SetTranslationVector(Vector3(-0.5f * w + float(i) * (w / float(col_cnt)), 1, 10 + float(z) * w));

                    rhi::UpdateConstBuffer4fv(cube.fp_const, 0, clr2, 1);
                    //            rhi::UpdateConstBuffer( cube.vp_const[0], 0, view_proj.data, 4 );
                    rhi::UpdateConstBuffer4fv(cube.vp_const[1], 0, world.data, 4);
                    rhi::AddPacket(pl[1], packet);
                }
            }
            rhi::EndPacketList(pl[1]);
        }
    #endif

        rhi::EndPacketList(pl[0]);
        rhi::RenderPass::End(pass);
    }

// draw render-target contents on-screen
    #if USE_RT
    {
        rhi::RenderPassConfig pass_desc;
        VirtualCoordinatesSystem* vcs = DAVA::UIControlSystem::Instance()->vcs;

        pass_desc.colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
        pass_desc.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
        pass_desc.colorBuffer[0].clearColor[0] = 0.15f;
        pass_desc.colorBuffer[0].clearColor[1] = 0.15f;
        pass_desc.colorBuffer[0].clearColor[2] = 0.25f;
        pass_desc.colorBuffer[0].clearColor[3] = 1.0f;
        pass_desc.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
        pass_desc.depthStencilBuffer.storeAction = rhi::STOREACTION_STORE;

        rhi::HPacketList pl;
        rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 1, &pl);

        rhi::RenderPass::Begin(pass);
        rhi::BeginPacketList(pl);

        Matrix4 world;
        Matrix4 view_proj;
        float ratio = float(vcs->GetPhysicalScreenSize().dx) / float(vcs->GetPhysicalScreenSize().dy);

        world = Matrix4::MakeRotation(Vector3(0, 1, 0), (30.0f * 3.1415f / 180.0f)) * Matrix4::MakeScale(Vector3(ratio, 1, 1));
        world.SetTranslationVector(Vector3(-2, 0, 15));

        view_proj.Identity();
        view_proj.BuildProjectionFovLH(75.0f, ratio, 1.0f, 1000.0f);

        rhi::ConstBuffer::SetConst(rtQuad.vp_const[0], 0, 4, view_proj.data);
        rhi::ConstBuffer::SetConst(rtQuad.vp_const[1], 0, 4, world.data);

        rhi::AddPacket(pl, rtQuadBatch0);

        rhi::EndPacketList(pl);
        rhi::RenderPass::End(pass);
    }
    #endif

    #undef USE_SECOND_CB
    #undef USE_RT
}

void RhiCubeApp::mrtDraw()
{
    #define USE_SECOND_CB 0
    #define USE_RT 1

    VirtualCoordinatesSystem* vcs = DAVA::UIControlSystem::Instance()->vcs;

    // draw scene into render-target
    {
        rhi::RenderPassConfig pass_desc;
        float clr[4] = { 1.0f, 0.6f, 0.0f, 1.0f };

    #if USE_RT
        pass_desc.colorBuffer[0].texture = rtColor0;
        pass_desc.colorBuffer[1].texture = rtColor1;
        pass_desc.colorBuffer[2].texture = rtColor2;
        pass_desc.colorBuffer[3].texture = rtColor3;
        pass_desc.depthStencilBuffer.texture = rtDepthStencil;

        pass_desc.colorBuffer[1].loadAction = rhi::LOADACTION_CLEAR;
        pass_desc.colorBuffer[1].storeAction = rhi::STOREACTION_STORE;
        pass_desc.colorBuffer[1].clearColor[0] = 0.15f;
        pass_desc.colorBuffer[1].clearColor[1] = 0.15f;
        pass_desc.colorBuffer[1].clearColor[2] = 0.15f;
        pass_desc.colorBuffer[1].clearColor[3] = 1.0f;

        pass_desc.colorBuffer[2].loadAction = rhi::LOADACTION_CLEAR;
        pass_desc.colorBuffer[2].storeAction = rhi::STOREACTION_STORE;
        pass_desc.colorBuffer[2].clearColor[0] = 0.20f;
        pass_desc.colorBuffer[2].clearColor[1] = 0.12f;
        pass_desc.colorBuffer[2].clearColor[2] = 0.00f;
        pass_desc.colorBuffer[2].clearColor[3] = 1.0f;

        pass_desc.colorBuffer[3].loadAction = rhi::LOADACTION_CLEAR;
        pass_desc.colorBuffer[3].storeAction = rhi::STOREACTION_STORE;
        pass_desc.colorBuffer[3].clearColor[0] = 0.00f;
        pass_desc.colorBuffer[3].clearColor[1] = 0.00f;
        pass_desc.colorBuffer[3].clearColor[2] = 0.05f;
        pass_desc.colorBuffer[3].clearColor[3] = 1.0f;
    #endif
        pass_desc.colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
        pass_desc.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
        pass_desc.colorBuffer[0].clearColor[0] = 0.25f;
        pass_desc.colorBuffer[0].clearColor[1] = 0.25f;
        pass_desc.colorBuffer[0].clearColor[2] = 0.35f;
        pass_desc.colorBuffer[0].clearColor[3] = 1.0f;
        pass_desc.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
        pass_desc.depthStencilBuffer.storeAction = rhi::STOREACTION_NONE;

        rhi::HPacketList pl[2];
    #if USE_SECOND_CB
        rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 2, pl);
    #else
        rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 1, pl);
    #endif

        rhi::RenderPass::Begin(pass);
        rhi::BeginPacketList(pl[0]);

        uint64 cube_t1 = SystemTimer::GetMs();
        uint64 dt = cube_t1 - cube_t0;

        cube_angle += 0.001f * float(dt) * (30.0f * 3.1415f / 180.0f);
        cube_t0 = cube_t1;

        Matrix4 world;
        Matrix4 view_proj;

        world.Identity();
        world.CreateRotation(Vector3(0, 1, 0), cube_angle);
        //    world.CreateRotation( Vector3(1,0,0), cube_angle );
        world.SetTranslationVector(Vector3(0, 0, 5));
        //world *= Matrix4::MakeScale(Vector3(0.5f, 0.5f, 0.5f));

        view_proj.Identity();
        view_proj.BuildProjectionFovLH(75.0f, float(vcs->GetPhysicalScreenSize().dx) / float(vcs->GetPhysicalScreenSize().dy), 1.0f, 1000.0f);

        rhi::ConstBuffer::SetConst(cube_mrt.fp_const, 0, 1, clr);
        rhi::ConstBuffer::SetConst(cube_mrt.vp_const[0], 0, 4, view_proj.data);
        rhi::ConstBuffer::SetConst(cube_mrt.vp_const[1], 0, 4, world.data);

        rhi::Packet packet;

        packet.vertexStreamCount = 1;
        packet.vertexStream[0] = cube_mrt.vb;
        packet.renderPipelineState = cube_mrt.ps;
        packet.vertexConstCount = 2;
        packet.vertexConst[0] = cube_mrt.vp_const[0];
        packet.vertexConst[1] = cube_mrt.vp_const[1];
        packet.fragmentConstCount = 1;
        packet.fragmentConst[0] = cube_mrt.fp_const;
        packet.textureSet = cube_mrt.texSet;
        packet.samplerState = cube_mrt.samplerState;
        packet.primitiveType = rhi::PRIMITIVE_TRIANGLELIST;
        packet.primitiveCount = 12;

        rhi::UpdateConstBuffer4fv(cube_mrt.fp_const, 0, clr, 1);
        rhi::UpdateConstBuffer4fv(cube_mrt.vp_const[0], 0, view_proj.data, 4);
        rhi::UpdateConstBuffer4fv(cube_mrt.vp_const[1], 0, world.data, 4);
        rhi::AddPacket(pl[0], packet);

    #if USE_SECOND_CB
        {
            const float w = 5.0f;
            const unsigned row_cnt = 30;
            const unsigned col_cnt = 7;

            rhi::BeginPacketList(pl[1]);
            for (unsigned z = 0; z != row_cnt; ++z)
            {
                for (unsigned i = 0; i != col_cnt; ++i)
                {
                    const uint32 c = (z * col_cnt + i + 1) * 0x775511; // 0x15015
                    const uint8* cc = (const uint8*)(&c);
                    const float clr2[] = { float(cc[2]) / 255.0f, float(cc[1]) / 255.0f, float(cc[0]) / 255.0f, 1.0f };

                    world.Identity();
                    world.CreateRotation(Vector3(1, 0, 0), cube_angle);
                    world.SetTranslationVector(Vector3(-0.5f * w + float(i) * (w / float(col_cnt)), 1, 10 + float(z) * w));

                    rhi::UpdateConstBuffer4fv(cube_mrt.fp_const, 0, clr2, 1);
                    //            rhi::UpdateConstBuffer( cube.vp_const[0], 0, view_proj.data, 4 );
                    rhi::UpdateConstBuffer4fv(cube_mrt.vp_const[1], 0, world.data, 4);
                    rhi::AddPacket(pl[1], packet);
                }
            }
            rhi::EndPacketList(pl[1]);
        }
    #endif

        rhi::EndPacketList(pl[0]);
        rhi::RenderPass::End(pass);
    }

// draw render-target contents on-screen
    #if USE_RT
    {
        rhi::RenderPassConfig pass_desc;

        pass_desc.colorBuffer[0].loadAction = rhi::LOADACTION_CLEAR;
        pass_desc.colorBuffer[0].storeAction = rhi::STOREACTION_STORE;
        pass_desc.colorBuffer[0].clearColor[0] = 0.15f;
        pass_desc.colorBuffer[0].clearColor[1] = 0.15f;
        pass_desc.colorBuffer[0].clearColor[2] = 0.25f;
        pass_desc.colorBuffer[0].clearColor[3] = 1.0f;
        pass_desc.depthStencilBuffer.loadAction = rhi::LOADACTION_CLEAR;
        pass_desc.depthStencilBuffer.storeAction = rhi::STOREACTION_STORE;

        rhi::HPacketList pl;
        rhi::HRenderPass pass = rhi::AllocateRenderPass(pass_desc, 1, &pl);

        rhi::RenderPass::Begin(pass);
        rhi::BeginPacketList(pl);

        Matrix4 world;
        Matrix4 view_proj;
        float ratio = float(vcs->GetPhysicalScreenSize().dx) / float(vcs->GetPhysicalScreenSize().dy);

        // target-0
        {
            world = Matrix4::MakeRotation(Vector3(0, 1, 0), (30.0f * 3.1415f / 180.0f)) * Matrix4::MakeScale(Vector3(ratio, 1, 1));
            world.SetTranslationVector(Vector3(-2, -1.5, 15));

            view_proj.Identity();
            view_proj.BuildProjectionFovLH(75.0f, ratio, 1.0f, 1000.0f);

            rhi::ConstBuffer::SetConst(rtQuad.vp_const[0], 0, 4, view_proj.data);
            rhi::ConstBuffer::SetConst(rtQuad.vp_const[1], 0, 4, world.data);

            rhi::AddPacket(pl, rtQuadBatch0);
        }

        // target-1
        {
            world = Matrix4::MakeRotation(Vector3(0, 1, 0), (-30.0f * 3.1415f / 180.0f)) * Matrix4::MakeScale(Vector3(ratio, 1, 1));
            world.SetTranslationVector(Vector3(2, -1.5, 15));
            rhi::ConstBuffer::SetConst(rtQuad.vp_const[1], 0, 4, world.data);
            rhi::AddPacket(pl, rtQuadBatch1);
        }

        // target-2
        {
            world = Matrix4::MakeRotation(Vector3(0, 1, 0), (-30.0f * 3.1415f / 180.0f)) * Matrix4::MakeScale(Vector3(ratio, 1, 1));
            world.SetTranslationVector(Vector3(-2, 1.5, 15));
            rhi::ConstBuffer::SetConst(rtQuad.vp_const[1], 0, 4, world.data);
            rhi::AddPacket(pl, rtQuadBatch2);
        }

        // target-3
        {
            world = Matrix4::MakeRotation(Vector3(0, 1, 0), (30.0f * 3.1415f / 180.0f)) * Matrix4::MakeScale(Vector3(ratio, 1, 1));
            world.SetTranslationVector(Vector3(2, 1.5, 15));
            rhi::ConstBuffer::SetConst(rtQuad.vp_const[1], 0, 4, world.data);
            rhi::AddPacket(pl, rtQuadBatch3);
        }

        rhi::EndPacketList(pl);
        rhi::RenderPass::End(pass);
    }
    #endif

    #undef USE_SECOND_CB
    #undef USE_RT
}

void RhiCubeApp::ScreenShotCallback(uint32 width, uint32 height, const void* rgba)
{
    DAVA::Logger::Info("saving screenshot");

    DAVA::Image* img = DAVA::Image::CreateFromData(width, height, FORMAT_RGBA8888, (const uint8*)rgba);

    if (img)
    {
        static int n = 0;
        char fname[128];

        Snprintf(fname, sizeof(fname) - 1, "~doc:/screenshot-%02i.png", ++n);
        img->Save(fname);
        DAVA::Logger::Info("saved screenshot \"%s\"", fname);
    }
}

DAVA::KeyedArchive* CreateOptions()
{
    DAVA::KeyedArchive* appOptions = new DAVA::KeyedArchive();

    appOptions->SetInt32("shader_const_buffer_size", 4 * 1024 * 1024);

    appOptions->SetInt32("max_index_buffer_count", 3 * 1024);
    appOptions->SetInt32("max_vertex_buffer_count", 3 * 1024);
    appOptions->SetInt32("max_const_buffer_count", 16 * 1024);
    appOptions->SetInt32("max_texture_count", 2048);
    appOptions->SetInt32("max_texture_set_count", 2048);
    appOptions->SetInt32("max_sampler_state_count", 128);
    appOptions->SetInt32("max_pipeline_state_count", 1024);
    appOptions->SetInt32("max_depthstencil_state_count", 256);
    appOptions->SetInt32("max_render_pass_count", 64);
    appOptions->SetInt32("max_command_buffer_count", 64);
    appOptions->SetInt32("max_packet_list_count", 64);

#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
    //appOptions->SetInt32("renderer", rhi::RHI_METAL);
    appOptions->SetInt32("rhi_threaded_frame_count", 1);
    appOptions->SetBool("iPhone_autodetectScreenScaleFactor", true);

#elif defined(__DAVAENGINE_WIN_UAP__)
    appOptions->SetInt32("bpp", 32);
    appOptions->SetInt32("renderer", rhi::RHI_DX11);
    appOptions->SetInt32("rhi_threaded_frame_count", 1);

#else
#if defined(__DAVAENGINE_WIN32__)
    //appOptions->SetInt32("renderer", rhi::RHI_DX9);
    //appOptions->SetInt32("renderer", rhi::RHI_DX9);
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
    appOptions->SetInt32("rhi_threaded_frame_count", 1);
#elif defined(__DAVAENGINE_MACOS__)
    appOptions->SetInt32("renderer", rhi::RHI_GLES2);
#endif

    //appOptions->SetInt("fullscreen.width",    1280);
    //appOptions->SetInt("fullscreen.height", 800);

    appOptions->SetInt32("bpp", 32);
#endif

    return appOptions;
}

int DAVAMain(DAVA::Vector<DAVA::String> cmdline)
{
    DAVA::Vector<DAVA::String> modules =
    {
      "JobManager"
    };
    DAVA::Engine e;
    e.Init(DAVA::eEngineRunMode::GUI_STANDALONE, modules, CreateOptions());

    RhiCubeApp app(e);
    return e.Run();
}
