#include "../dbg_Draw.h"
#include "../rhi_ShaderSource.h"
#include "../rhi_ShaderCache.h"
#include "rhi_Utils.h"

#include "Math/Matrix4.h"
#include "Math/Vector.h"
using DAVA::Vector3;

#include "Render/Renderer.h"

#include <stdio.h>
#include <stdarg.h>

#if defined(__DAVAENGINE_POSIX__)
#define _vsnprintf vsnprintf
#endif

//==============================================================================

#include "dbg_FontNormal.cxx"
#include "dbg_FontSmall.cxx"

//==============================================================================

static const char* vp__dbg_ptc =
"vertex_in\n"
"{\n"
"    float3 pos   : POSITION;\n"
"    float2 uv    : TEXCOORD;\n"
//"    uint4 color  : COLOR;\n"
"    float4 color  : COLOR;\n"
"};\n"
"vertex_out\n"
"{\n"
"    float4 pos   : SV_POSITION;\n"
"    float2 uv    : TEXCOORD;\n"
"    float4 color : COLOR;\n"
"};\n"
"\n"
"[unique][dynamic] property float4x4   XForm;\n"
"\n"
"vertex_out\n"
"vp_main( vertex_in input )\n"
"{\n"
"    vertex_out output;\n"
"   output.pos   = mul( float4(input.pos.xyz,1.0), XForm );\n"
"   output.uv    = input.uv;\n"
"   output.color = input.color;"
"    return output;\n"
"}\n";

static const char* fp__dbg_ptc =
"fragment_in\n"
"{\n"
"    float2 uv    : TEXCOORD;\n"
"    float4 color : COLOR;\n"
"};\n"
"fragment_out\n"
"{\n"
"    float4 color : SV_TARGET;\n"
"};\n"
"\n"
"uniform sampler2D Image;\n"
"\n"
"fragment_out\n"
"fp_main( fragment_in input )\n"
"{\n"
"    fragment_out output;\n"
"    float4       image = tex2D( Image, input.uv );"
"    output.color = image * input.color;\n"
"    return output;\n"
"}\n"
"blending { src=src_alpha dst=inv_src_alpha }\n"
;

static const char* vp__dbg_pc =
"vertex_in\n"
"{\n"
"    float3 pos   : POSITION;\n"
"    float4 color : COLOR;\n"
"};\n"
"vertex_out\n"
"{\n"
"    float4 pos   : SV_POSITION;\n"
"    float4 color : COLOR;\n"
"};\n"
"\n"
"[unique][dynamic] property float4x4   XForm;\n"
"\n"
"vertex_out\n"
"vp_main( vertex_in input )\n"
"{\n"
"    vertex_out output;\n"
"    output.pos   = mul( float4(input.pos.xyz,1.0), XForm );\n"
"    output.color = float4(input.color);"
"    return output;\n"
"}\n";

static const char* fp__dbg_pc =
"fragment_in\n"
"{\n"
"    float4 color : COLOR;\n"
"};\n"
"fragment_out\n"
"{\n"
"    float4 color : SV_TARGET;\n"
"};\n"
"\n"
"fragment_out\n"
"fp_main( fragment_in input )\n"
"{\n"
"    fragment_out output;\n"
"    output.color = input.color;\n"
"    return output;\n"
"}\n"
"blending { src=src_alpha dst=inv_src_alpha }\n"
;

namespace DAVA
{
//------------------------------------------------------------------------------

DbgDraw*
DbgDraw::Instance()
{
    static DbgDraw inst;

    return &inst;
}

//------------------------------------------------------------------------------

void DbgDraw::BufferBase::set_small_text_size()
{
    _small_text = true;
}

//------------------------------------------------------------------------------

void DbgDraw::BufferBase::set_normal_text_size()
{
    _small_text = false;
}

//==============================================================================

template <typename Vertex, rhi::PrimitiveType Prim>
inline DbgDraw::Buffer<Vertex, Prim>::Buffer(const char* const name)
    : _cur_vb_i(0)
    , _vb_size(0)
    , _cur_v(nullptr)
    , _end_v(nullptr)
    , _v_cnt(0)
    , _name(name)
    , _need_grow(false)
    , _grow_ttw(0)
{
}

//------------------------------------------------------------------------------

template <typename Vertex, rhi::PrimitiveType Prim>
inline DbgDraw::Buffer<Vertex, Prim>::~Buffer()
{
    destroy();
}

//------------------------------------------------------------------------------

template <typename Vertex, rhi::PrimitiveType Prim>
inline bool
DbgDraw::Buffer<Vertex, Prim>::construct(unsigned max_vertex_count)
{
    bool success = true;

    for (unsigned i = 0; i < countof(_vb); ++i)
    {
        if (_vb[i] == rhi::InvalidHandle)
        {
            rhi::VertexBuffer::Descriptor descr = rhi::VertexBuffer::Descriptor(max_vertex_count * sizeof(Vertex));
            descr.usage = rhi::USAGE_DYNAMICDRAW;
            descr.needRestore = false;
            _vb[i] = rhi::CreateVertexBuffer(descr);

            success = _vb[i] != rhi::InvalidHandle;
        }
    }

    _vb_size = max_vertex_count * sizeof(Vertex);
    _cur_v = nullptr;
    _end_v = nullptr;
    _v_cnt = 0;

    return success;
}

//------------------------------------------------------------------------------

template <typename Vertex, rhi::PrimitiveType Prim>
inline void
DbgDraw::Buffer<Vertex, Prim>::destroy()
{
    for (unsigned i = 0; i < countof(_vb); ++i)
    {
        if (_vb[i] != rhi::InvalidHandle)
            rhi::DeleteVertexBuffer(_vb[i]);

        _vb[i] = rhi::HVertexBuffer(rhi::InvalidHandle);
    }

    _vb_size = 0;
    _cur_v = nullptr;
    _end_v = nullptr;
    _v_cnt = 0;
}

//------------------------------------------------------------------------------

template <typename Vertex, rhi::PrimitiveType Prim>
inline void
DbgDraw::Buffer<Vertex, Prim>::_grow()
{
    if (!_need_grow)
        return;

    if (_grow_ttw)
    {
        --_grow_ttw;
        return;
    }

    DVASSERT(!_cur_v);

    bool success = true;
    unsigned add_sz = _vb_size;
    unsigned vb_sz = _vb_size + add_sz;
    rhi::HVertexBuffer vb[VBCount];

    for (unsigned i = 0; i < countof(_vb); ++i)
    {
        rhi::VertexBuffer::Descriptor descr = rhi::VertexBuffer::Descriptor(vb_sz);
        descr.usage = rhi::USAGE_DYNAMICDRAW;
        descr.needRestore = false;
        vb[i] = rhi::CreateVertexBuffer(descr);

        if (!vb[i].IsValid())
        {
            success = false;
            break;
        }
    }

    if (success)
    {
        for (unsigned i = 0; i < countof(_vb); ++i)
        {
            rhi::DeleteVertexBuffer(_vb[i]);
            _vb[i] = vb[i];
        }

        _vb_size = vb_sz;
        //        Log::Note( "DbgDraw", "\"%s\" VB grown to %s\n", _name, FormattedInt(vb_sz).text() );
    }
    else
    {
        //        Log::Note( "DbgDraw", "FAILED to grow \"%s\" VB to %s\n", _name, FormattedInt(vb_sz).text() );
    }

    _need_grow = false;
}

//------------------------------------------------------------------------------

template <typename Vertex, rhi::PrimitiveType Prim>
inline Vertex*
DbgDraw::Buffer<Vertex, Prim>::alloc_vertices(unsigned count)
{
    Vertex* v = nullptr;

    if (_vb[_cur_vb_i] != rhi::InvalidHandle && !(_need_grow))
    {
        if (!_cur_v)
        {
            _cur_v = reinterpret_cast<Vertex*>(rhi::MapVertexBuffer(_vb[_cur_vb_i], 0, _vb_size));
            _end_v = reinterpret_cast<Vertex*>(reinterpret_cast<uint8*>(_cur_v) + _vb_size);
        }

        if (_cur_v && _cur_v + count < _end_v)
        {
            v = _cur_v;
            _cur_v += count;
            _v_cnt += count;
        }
        else
        {
            if (!_need_grow)
            {
                _need_grow = true;
                _grow_ttw = 2;
            }
        }
    }

    return v;
}

//------------------------------------------------------------------------------

template <>
inline unsigned
DbgDraw::Buffer<DbgDraw::Vertex_PC, rhi::PRIMITIVE_LINELIST>::_prim_count(unsigned v_cnt) const
{
    return v_cnt / 2;
}

//------------------------------------------------------------------------------

template <>
inline unsigned
DbgDraw::Buffer<DbgDraw::Vertex_PC, rhi::PRIMITIVE_TRIANGLELIST>::_prim_count(unsigned v_cnt) const
{
    return v_cnt / 3;
}

//------------------------------------------------------------------------------

template <>
inline unsigned
DbgDraw::Buffer<DbgDraw::Vertex_PTC, rhi::PRIMITIVE_TRIANGLELIST>::_prim_count(unsigned v_cnt) const
{
    return v_cnt / 3;
}

//------------------------------------------------------------------------------
/*
template <typename Vertex,rhi::PrimitiveType Prim>
inline void
DbgDraw::Buffer<Vertex,Prim>::flush_3d( rhi::Handle cmd_buf, const Matrix4f& view, const Matrix4f& projection )
{
    if( _vb[_cur_vb_i] != rhi::InvalidHandle )
    {    
        if( _cur_v )
            VertexBuffer_Unmap( _vb[_cur_vb_i] );
        _cur_v = null_ptr;

        if( _v_cnt )
        {
            BufferBase::_setup_render( view, projection, Vertex::Format, draw3D, cmd_buf );

            unsigned    max_v_cnt =
            #if L_PLATFORM == L_PLATFORM_PSP2
                                    ( Prim == rhi::primTriangleList  ||  Prim == Render::primitiveLineList ) 
                                    ? 0x0000FFFFU
                                    : 0xFFFFFFFFU;
            #else
                                    0xFFFFFFFFU;
            #endif

            unsigned    v_cnt           = _v_cnt;
            unsigned    vert_per_prim   = (Prim == rhi::primTriangleList)  ? 3  : 2;
            unsigned    batch_v_cnt     = _prim_count(max_v_cnt) * vert_per_prim;
            unsigned    offset          = 0;

            while( v_cnt > batch_v_cnt )
            {
                // CRAP: use offset !!!
                rhi::SetStreamSource( cmd_buf, _vb[_cur_vb_i], 0 );
                rhi::DrawPrimitive( cmd_buf, Prim, _prim_count(batch_v_cnt) );
                offset += batch_v_cnt * sizeof(Vertex);
                v_cnt -= batch_v_cnt;
            }

///            rhi::SetStreamSource( cmd_buf, _vb[_cur_vb_i], 0 );
///            rhi::DrawPrimitive( cmd_buf, Prim, _prim_count(v_cnt) );

            BufferBase::_restore_render( cmd_buf );
        }

        _v_cnt = 0;
        _grow( render );

        if( ++_cur_vb_i >= countof(_vb) )
            _cur_vb_i = 0;
    }
}


//------------------------------------------------------------------------------

template <typename Vertex,rhi::PrimitiveType Prim>
inline void
DbgDraw::Buffer<Vertex,Prim>::flush_2d( rhi::Handle cmd_buf )
{
    if( _vb[_cur_vb_i] != DAVA::InvalidIndex )
    {    
        if( _cur_v )
            rhi::VertexBuffer_Unmap( _vb[_cur_vb_i] );
        _cur_v = null_ptr;

        if( _v_cnt )
        {
            BufferBase::_setup_render( Matrix4f(), Matrix4f(), Vertex::Format, draw2D, cmd_buf );

            rhi::SetStreamSource( cmd_buf, _vb[_cur_vb_i], 0 );
            rhi::DrawPrimitive( cmd_buf, Prim, _prim_count(_v_cnt) );
            BufferBase::_restore_render( cmd_buf );
        }

        _v_cnt = 0;
        _grow();

        if( ++_cur_vb_i >= countof(_vb) )
            _cur_vb_i = 0;
    }
}
*/

//------------------------------------------------------------------------------

template <typename Vertex, rhi::PrimitiveType Prim>
inline void
DbgDraw::Buffer<Vertex, Prim>::flush_batched_2d(rhi::HPacketList batch_buf)
{
    if (_vb[_cur_vb_i] != DAVA::InvalidIndex)
    {
        if (_cur_v)
            rhi::UnmapVertexBuffer(_vb[_cur_vb_i]);
        _cur_v = nullptr;

        if (_v_cnt)
        {
            DbgDraw* dd = Instance();
            DVASSERT(dd->_inited);
            DVASSERT(dd->_wnd_w);
            DVASSERT(dd->_wnd_h);
            rhi::Packet batch;
            Matrix4 ortho(
            2.0f / dd->_wnd_w, 0.0f, 0.0f, -1.0f,
            0.0f, 2.0f / dd->_wnd_h, 0.0f, 1.0f,
            0.0f, 0.0f, -(0.0f) / (0.0f - 1.0f), 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
            ortho.Transpose();

            batch.vertexStreamCount = 1;
            batch.vertexStream[0] = _vb[_cur_vb_i];
            //-            batch.indexBuffer        = rhi::InvalidHandle;
            batch.vertexConstCount = 1;
            batch.fragmentConstCount = 0;
            batch.primitiveType = Prim;
            batch.primitiveCount = _prim_count(_v_cnt);

            switch (Vertex::Format)
            {
            case Vertex_PC::Format:
                batch.renderPipelineState = dd->_pc_pipeline_state;
                batch.vertexConst[0] = dd->_pc_const;
                rhi::UpdateConstBuffer4fv(dd->_pc_const, 0, ortho.data, 4);
                break;

            case Vertex_PTC::Format:
                batch.renderPipelineState = dd->_ptc_pipeline_state;
                batch.vertexConst[0] = dd->_ptc_const;
                batch.depthStencilState = dd->_ptc_depth_state;
                batch.samplerState = dd->_ptc_sampler_state;
                batch.textureSet = (_small_text) ? dd->_texset_small_font : dd->_texset_normal_font;
                rhi::UpdateConstBuffer4fv(dd->_ptc_const, 0, ortho.data, 4);
                break;
            }

            rhi::AddPacket(batch_buf, batch);
        }

        _v_cnt = 0;
        _grow();

        if (++_cur_vb_i >= countof(_vb))
            _cur_vb_i = 0;
    }
}

//==============================================================================
//
//  publics:

DbgDraw::DbgDraw()
    : _line_buf1("line1.3D")
    , _line_buf2("line2.3D")
    , _tri3d_buf("tri.3D")
    , _tri2d_buf("tri.2D")
    , _normal_text2d_buf("text.normal")
    , _small_text2d_buf("text.small")
    , _line2d_buf("line.2D")
    , _inited(false)
    , _no_depth_test(false)
    , _small_text(false)
    ,
    //  _draw_immediately(false),
    _wnd_w(0)
    , _wnd_h(0)
{
}

//------------------------------------------------------------------------------

DbgDraw::~DbgDraw()
{
}

//------------------------------------------------------------------------------

void DbgDraw::EnsureInited()
{
    DbgDraw* dd = Instance();

    if (!dd->_inited)
    {
        dd->_init();
        dd->_inited = true;
    }
}

//------------------------------------------------------------------------------

void DbgDraw::SetScreenSize(uint32 w, uint32 h)
{
    DbgDraw* dd = Instance();

    dd->_wnd_w = w;
    dd->_wnd_h = h;
}

//------------------------------------------------------------------------------

void DbgDraw::FlushBatched(rhi::HPacketList batchBuf)
{
    DbgDraw* dd = Instance();

    EnsureInited();

    //    dd->_generate_permanent_marks();
    //    dd->_generate_permanent_text();

    dd->_tri2d_buf.flush_batched_2d(batchBuf);
    dd->_line2d_buf.flush_batched_2d(batchBuf);

    dd->_normal_text2d_buf.flush_batched_2d(batchBuf);
    dd->_small_text2d_buf.flush_batched_2d(batchBuf);
}

//------------------------------------------------------------------------------

void DbgDraw::SetNormalTextSize()
{
    Instance()->_small_text = false;
}

//------------------------------------------------------------------------------

void DbgDraw::SetSmallTextSize()
{
    Instance()->_small_text = true;
}

//------------------------------------------------------------------------------

unsigned
DbgDraw::Text2D(int x, int y, uint32 color, const char* format, ...)
{
    va_list arglist;
    char text[MaxTextLength + 1] = { 0 };
    va_start(arglist, format);
    int len = _vsnprintf(text, MaxTextLength, format, arglist);
    va_end(arglist);

    if (len == 0)
        return 0;

    DbgDraw* dd = Instance();
    DVASSERT(dd->_inited);
    unsigned v_cnt = len * 2 * 3;
    Vertex_PTC* tv = (dd->_small_text) ? dd->_small_text2d_buf.alloc_vertices(v_cnt) : dd->_normal_text2d_buf.alloc_vertices(v_cnt);
    float left = float(x);
    float top = -float(y);

    if (!tv)
        return 0;

    // make exact texel-to-pixel match
    switch (rhi::HostApi())
    {
    case rhi::RHI_DX9:
        left += 0.5f;
        top += 0.5f;
        break;

    case rhi::RHI_DX11:
        break; // DO NOT do half-pixel offset

    case rhi::RHI_GLES2:
        //            left  -= 0.5f;
        //            top   -= 0.5f;
        break;

    case rhi::RHI_METAL:
        //            left  -= 0.5f;
        //            top   -= 0.5f;
        break;
    default:
        break; // to shut up goddamn warning
    }

    const float char_w = (dd->_small_text) ? float(SmallCharW) : float(NormalCharW);
    const float char_h = (dd->_small_text) ? float(SmallCharH) : float(NormalCharH);
    const float tex_char_w = char_w / float(FontTextureSize);
    const float tex_char_h = char_h / float(FontTextureSize);
    Vertex_PTC* v = tv;
    float x0 = left;

    for (const char *t = text; *t; ++t, x0 += char_w)
    {
        int symbol = *t - '!';
        float tex_char_x = 0.0f;
        float tex_char_y = 0.0f;
        float x1 = x0 + char_w;
        float y1 = top - char_h;

        if ((symbol < 0) || (symbol > ('~' - '!')))
        {
            tex_char_x = 1.0f - tex_char_w;
            tex_char_y = 1.0f - tex_char_h;
        }
        else
        {
            tex_char_x = (float(symbol % 12) * char_w) / float(FontTextureSize);
            tex_char_y = (float(symbol / 12) * char_h) / float(FontTextureSize);
        }

        v->x = x0;
        v->y = y1;
        v->z = 0;
        v->u = tex_char_x;
        v->v = tex_char_y + tex_char_h;
        v->color = color;
        ++v;

        v->x = x0;
        v->y = top;
        v->z = 0;
        v->u = tex_char_x;
        v->v = tex_char_y;
        v->color = color;
        ++v;

        v->x = x1;
        v->y = y1;
        v->z = 0;
        v->u = tex_char_x + tex_char_w;
        v->v = tex_char_y + tex_char_h;
        v->color = color;
        ++v;

        v->x = x0;
        v->y = top;
        v->z = 0;
        v->u = tex_char_x;
        v->v = tex_char_y;
        v->color = color;
        ++v;

        v->x = x1;
        v->y = top;
        v->z = 0;
        v->u = tex_char_x + tex_char_w;
        v->v = tex_char_y;
        v->color = color;
        ++v;

        v->x = x1;
        v->y = y1;
        v->z = 0;
        v->u = tex_char_x + tex_char_w;
        v->v = tex_char_y + tex_char_h;
        v->color = color;
        ++v;
    } // for each character

    return len;
}

//------------------------------------------------------------------------------

unsigned DbgDraw::MultilineText2D(int x, int y, uint32 color, const char* format, ...)
{
    va_list arglist;
    char text[2048] = { 0 };
    va_start(arglist, format);
    int len = _vsnprintf(text, countof(text) - 1, format, arglist);
    va_end(arglist);

    if (len == 0)
        return 0;

    char* line = text;
    int y0 = y;
    int h = (Instance()->_small_text) ? SmallCharH : NormalCharH;
    int line_cnt = 1;

    for (char* t = text; *t; ++t)
    {
        if (*t == '\n')
        {
            *t = '\0';
            ++t;
            DbgDraw::Text2D(x, y0, color, line);
            line = t;

            y0 += h;
            ++line_cnt;
        }
    }
    DbgDraw::Text2D(x, y0, color, line);

    return line_cnt;
}

//------------------------------------------------------------------------------

void DbgDraw::FilledRect2D(int left, int top, int right, int bottom, uint32 color)
{
    DbgDraw* dd = Instance();
    DVASSERT(dd->_inited);
    unsigned v_cnt = 2 * 3;
    Vertex_PC* v = dd->_tri2d_buf.alloc_vertices(v_cnt);
    float x0 = float(left);
    float x1 = float(right);
    float y0 = -float(top);
    float y1 = -float(bottom);

    if (v)
    {
        v->x = x0;
        v->y = y1;
        v->z = 0.5f;
        v->color = color;
        ++v;

        v->x = x0;
        v->y = y0;
        v->z = 0.5f;
        v->color = color;
        ++v;

        v->x = x1;
        v->y = y1;
        v->z = 0.5f;
        v->color = color;
        ++v;

        v->x = x0;
        v->y = y0;
        v->z = 0.5f;
        v->color = color;
        ++v;

        v->x = x1;
        v->y = y0;
        v->z = 0.5f;
        v->color = color;
        ++v;

        v->x = x1;
        v->y = y1;
        v->z = 0.5f;
        v->color = color;
        ++v;
    }
}

//------------------------------------------------------------------------------

void DbgDraw::Line2D(int x1, int y1, int x2, int y2, uint32 color)
{
    DbgDraw* dd = Instance();
    DVASSERT(dd->_inited);
    Vertex_PC* v = dd->_line2d_buf.alloc_vertices(2);

    if (v)
    {
        v->x = float(x1);
        v->y = -float(y1);
        v->z = 0;
        v->color = color;
        ++v;

        v->x = float(x2);
        v->y = -float(y2);
        v->z = 0;
        v->color = color;
    }
}

//------------------------------------------------------------------------------

void DbgDraw::Rect2D(int left, int top, int right, int bottom, uint32 color)
{
    DbgDraw* dd = Instance();
    DVASSERT(dd->_inited);
    unsigned v_cnt = 4 * 2;
    Vertex_PC* v = dd->_line2d_buf.alloc_vertices(v_cnt);
    float x0 = float(left);
    float x1 = float(right);
    float y0 = -float(top);
    float y1 = -float(bottom);

    if (v)
    {
        v->x = x0;
        v->y = y0;
        v->z = 0;
        v->color = color;
        ++v;
        v->x = x1;
        v->y = y0;
        v->z = 0;
        v->color = color;
        ++v;

        v->x = x1;
        v->y = y0;
        v->z = 0;
        v->color = color;
        ++v;
        v->x = x1;
        v->y = y1;
        v->z = 0;
        v->color = color;
        ++v;

        v->x = x1;
        v->y = y1;
        v->z = 0;
        v->color = color;
        ++v;
        v->x = x0;
        v->y = y1;
        v->z = 0;
        v->color = color;
        ++v;

        v->x = x0;
        v->y = y1;
        v->z = 0;
        v->color = color;
        ++v;
        v->x = x0;
        v->y = y0;
        v->z = 0;
        v->color = color;
        ++v;
    }
}

//------------------------------------------------------------------------------

void DbgDraw::Triangle2D(int x0, int y0, int x1, int y1, int x2, int y2, uint32 color)
{
    DbgDraw* dd = Instance();
    DVASSERT(dd->_inited);
    unsigned v_cnt = 3 * 2;
    Vertex_PC* v = dd->_line2d_buf.alloc_vertices(v_cnt);

    if (v)
    {
        v->x = float(x0);
        v->y = -float(y0);
        v->z = 0;
        v->color = color;
        ++v;
        v->x = float(x1);
        v->y = -float(y1);
        v->z = 0;
        v->color = color;
        ++v;

        v->x = float(x1);
        v->y = -float(y1);
        v->z = 0;
        v->color = color;
        ++v;
        v->x = float(x2);
        v->y = -float(y2);
        v->z = 0;
        v->color = color;
        ++v;

        v->x = float(x2);
        v->y = -float(y2);
        v->z = 0;
        v->color = color;
        ++v;
        v->x = float(x0);
        v->y = -float(y0);
        v->z = 0;
        v->color = color;
        ++v;
    }
}

//------------------------------------------------------------------------------

void DbgDraw::FilledTriangle2D(int x0, int y0, int x1, int y1, int x2, int y2, uint32 color)
{
    DbgDraw* dd = Instance();
    DVASSERT(dd->_inited);
    unsigned v_cnt = 3;
    Vertex_PC* v = dd->_tri2d_buf.alloc_vertices(v_cnt);

    if (v)
    {
        v->x = float(x0);
        v->y = -float(y0);
        v->z = 0.5f;
        v->color = color;
        ++v;

        v->x = float(x1);
        v->y = -float(y1);
        v->z = 0.5f;
        v->color = color;
        ++v;

        v->x = float(x2);
        v->y = -float(y2);
        v->z = 0.5f;
        v->color = color;
        ++v;
    }
}

//------------------------------------------------------------------------------

void DbgDraw::_init()
{
    // init PTC

    ///    ShaderPreprocessScope preprocessScope;

    rhi::ShaderSource vp_ptc;
    rhi::ShaderSource fp_ptc;

    if (vp_ptc.Construct(rhi::PROG_VERTEX, vp__dbg_ptc) && fp_ptc.Construct(rhi::PROG_FRAGMENT, fp__dbg_ptc))
    {
        rhi::PipelineState::Descriptor ps_desc;
        rhi::DepthStencilState::Descriptor ds_desc;
        rhi::SamplerState::Descriptor s_desc;

        ps_desc.vertexLayout = vp_ptc.ShaderVertexLayout();
        ps_desc.vprogUid = FastName("vp.ptc");
        ps_desc.fprogUid = FastName("fp.ptc");
        ps_desc.blending = fp_ptc.Blending();

        ds_desc.depthTestEnabled = false;
        ds_desc.depthWriteEnabled = false;

        s_desc.fragmentSamplerCount = 1;
        s_desc.fragmentSampler[0].minFilter = rhi::TEXFILTER_NEAREST;
        s_desc.fragmentSampler[0].magFilter = rhi::TEXFILTER_NEAREST;
        s_desc.fragmentSampler[0].mipFilter = rhi::TEXMIPFILTER_NONE;

        const std::string& vp_bin = vp_ptc.GetSourceCode(rhi::HostApi());
        const std::string& fp_bin = fp_ptc.GetSourceCode(rhi::HostApi());

        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_VERTEX, ps_desc.vprogUid, vp_bin.c_str(), unsigned(vp_bin.length()));
        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_FRAGMENT, ps_desc.fprogUid, fp_bin.c_str(), unsigned(fp_bin.length()));

        _ptc_pipeline_state = rhi::AcquireRenderPipelineState(ps_desc);
        rhi::CreateVertexConstBuffers(_ptc_pipeline_state, 1, &_ptc_const);

        _ptc_depth_state = rhi::AcquireDepthStencilState(ds_desc);
        _ptc_sampler_state = rhi::AcquireSamplerState(s_desc);
    }

    // init PC

    rhi::ShaderSource vp_pc;
    rhi::ShaderSource fp_pc;

    if (vp_pc.Construct(rhi::PROG_VERTEX, vp__dbg_pc) && fp_pc.Construct(rhi::PROG_FRAGMENT, fp__dbg_pc))
    {
        rhi::PipelineState::Descriptor desc;

        desc.vertexLayout = vp_pc.ShaderVertexLayout();
        desc.vprogUid = FastName("vp.pc");
        desc.fprogUid = FastName("fp.pc");
        desc.blending = fp_pc.Blending();

        const std::string& vp_bin = vp_pc.GetSourceCode(rhi::HostApi());
        const std::string& fp_bin = fp_pc.GetSourceCode(rhi::HostApi());

        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_VERTEX, desc.vprogUid, vp_bin.c_str(), unsigned(vp_bin.length()));
        rhi::ShaderCache::UpdateProgBinary(rhi::HostApi(), rhi::PROG_FRAGMENT, desc.fprogUid, fp_bin.c_str(), unsigned(fp_bin.length()));

        _pc_pipeline_state = rhi::AcquireRenderPipelineState(desc);
        rhi::CreateVertexConstBuffers(_pc_pipeline_state, 1, &_pc_const);
    }

    // init small-font texture
    {
        rhi::Texture::Descriptor descr = rhi::Texture::Descriptor(FontTextureSize, FontTextureSize, rhi::TEXTURE_FORMAT_R8G8B8A8);
        _tex_small_font = rhi::CreateTexture(descr);

        if (_tex_small_font)
        {
            void* mip0 = rhi::MapTexture(_tex_small_font, 0);

            memcpy(mip0, Bin__dbg_FontSmall, Bin__dbg_FontSmall__Size);
            rhi::UnmapTexture(_tex_small_font);

            rhi::TextureSetDescriptor ts;
            ts.fragmentTextureCount = 1;
            ts.fragmentTexture[0] = _tex_small_font;

            _texset_small_font = rhi::AcquireTextureSet(ts);
        }
    }

    // init normal-font texture
    {
        rhi::Texture::Descriptor descr = rhi::Texture::Descriptor(FontTextureSize, FontTextureSize, rhi::TEXTURE_FORMAT_R8G8B8A8);
        _tex_normal_font = rhi::CreateTexture(descr);

        if (_tex_normal_font)
        {
            void* mip0 = rhi::MapTexture(_tex_normal_font, 0);

            memcpy(mip0, Bin__dbg_FontNormal, Bin__dbg_FontNormal__Size);
            rhi::UnmapTexture(_tex_normal_font);

            rhi::TextureSetDescriptor ts;
            ts.fragmentTextureCount = 1;
            ts.fragmentTexture[0] = _tex_normal_font;

            _texset_normal_font = rhi::AcquireTextureSet(ts);
        }
    }

    _normal_text2d_buf.construct(4 * 1024);
    _normal_text2d_buf.set_normal_text_size();
    _small_text2d_buf.construct(4 * 1024);
    _small_text2d_buf.set_small_text_size();
    _tri2d_buf.construct(1 * 1024);
    _line2d_buf.construct(1 * 1024);

    _permanent_text_small = true;

    Renderer::GetSignals().needRestoreResources.Connect(this, &DbgDraw::_restore);
}

//------------------------------------------------------------------------------

void DbgDraw::_uninit()
{
    if (_inited)
    {
        _line_buf1.destroy();
        _line_buf2.destroy();
        _tri3d_buf.destroy();
        _normal_text2d_buf.destroy();
        _small_text2d_buf.destroy();
        _tri2d_buf.destroy();
        _line2d_buf.destroy();

        Renderer::GetSignals().needRestoreResources.Disconnect(this);

        _inited = false;
    }
}

void DbgDraw::_restore()
{
    DbgDraw* dd = Instance();

    if (rhi::NeedRestoreTexture(dd->_tex_small_font))
    {
        rhi::UpdateTexture(dd->_tex_small_font, Bin__dbg_FontSmall, 0);
    }

    if (rhi::NeedRestoreTexture(dd->_tex_normal_font))
    {
        rhi::UpdateTexture(dd->_tex_normal_font, Bin__dbg_FontNormal, 0);
    }
}

//------------------------------------------------------------------------------

DbgDraw::Vertex_PC*
DbgDraw::_alloc_pc_vertices(unsigned count)
{
    return (_no_depth_test) ? _line_buf2.alloc_vertices(count) : _line_buf1.alloc_vertices(count);
}

//------------------------------------------------------------------------------

void DbgDraw::Uninitialize()
{
    Instance()->_uninit();
}
}
//==============================================================================
