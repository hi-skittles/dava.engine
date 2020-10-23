    #include "../rhi_ShaderCache.h"
    #include "../rhi_ShaderSource.h"

namespace rhi
{
static ShaderBuilder _ShaderBuilder = nullptr;

struct ProgInfo
{
    DAVA::FastName uid;
    std::vector<uint8> bin;
};

static std::vector<ProgInfo> _ProgInfo;

namespace ShaderCache
{
//------------------------------------------------------------------------------

bool Initialize(ShaderBuilder builder)
{
    _ShaderBuilder = builder;
    return true;
}

//------------------------------------------------------------------------------

void Unitialize()
{
}

//------------------------------------------------------------------------------

void Clear()
{
}

//------------------------------------------------------------------------------

void Load(const char* binFileName)
{
}

//------------------------------------------------------------------------------

const std::vector<uint8>& GetProg(const DAVA::FastName& uid)
{
    static const std::vector<uint8> empty(0);

    for (unsigned i = 0; i != _ProgInfo.size(); ++i)
    {
        if (_ProgInfo[i].uid == uid)
        {
            return _ProgInfo[i].bin;
        }
    }

    return empty;
}

//------------------------------------------------------------------------------
/*
static const char* _ShaderHeader_Metal =
"#include <metal_stdlib>\n"
"#include <metal_graphics>\n"
"#include <metal_matrix>\n"
"#include <metal_geometric>\n"
"#include <metal_math>\n"
"#include <metal_texture>\n"
"using namespace metal;\n\n"

"#define min10float  half\n"
"#define min10float1 half\n"
"#define min10float2 half2\n"
"#define min10float3 half3\n"
"#define min10float4 half4\n"

"#define half1 half\n"
"#define half2 half2\n"
"#define half3 half3\n"
"#define half4 half4\n"

"#define float1 float\n"
"#define float2 vector_float2\n"
"#define float3 vector_float3\n"
"#define float4 vector_float4\n"

"float4 mul( float4 v, float4x4 m );\n"
"float4 mul( float4 v, float4x4 m ) { return m*v; }\n"
"float4 mul( float4x4 m, float4 v );\n"
"float4 mul( float4x4 m, float4 v ) { return v*m; }\n"
"float3 mul( float3 v, float3x3 m );\n"
"float3 mul( float3 v, float3x3 m ) { return m*v; }\n"
"#define lerp(a,b,t) mix( (a), (b), (t) )\n"

"#define  frac(a) fract(a)\n"

"#define FP_DISCARD_FRAGMENT discard_fragment()\n"
"#define FP_A8(t) t.a\n"

"#define STEP(edge,x) step((edge), (x))\n";

static const char* _ShaderDefine_Metal =
"#define VPROG_IN_BEGIN          struct VP_Input {\n"
"#define VPROG_IN_POSITION       float3 position [[ attribute(VATTR_POSITION) ]]; \n"
"#define VPROG_IN_NORMAL         float3 normal [[ attribute(VATTR_NORMAL) ]] ; \n"
"#define VPROG_IN_TEXCOORD       float2 texcoord0 [[ attribute(VATTR_TEXCOORD_0) ]] ; \n"
"#define VPROG_IN_TEXCOORD0(sz)  float##sz texcoord0 [[ attribute(VATTR_TEXCOORD_0) ]] ; \n"
"#define VPROG_IN_TEXCOORD1(sz)  float##sz texcoord1 [[ attribute(VATTR_TEXCOORD_1) ]] ; \n"
"#define VPROG_IN_TEXCOORD2(sz)  float##sz texcoord2 [[ attribute(VATTR_TEXCOORD_2) ]] ; \n"
"#define VPROG_IN_TEXCOORD3(sz)  float##sz texcoord3 [[ attribute(VATTR_TEXCOORD_3) ]] ; \n"
"#define VPROG_IN_TEXCOORD4(sz)  float##sz texcoord4 [[ attribute(VATTR_TEXCOORD_4) ]] ; \n"
"#define VPROG_IN_TEXCOORD5(sz)  float##sz texcoord5 [[ attribute(VATTR_TEXCOORD_5) ]] ; \n"
"#define VPROG_IN_TEXCOORD6(sz)  float##sz texcoord6 [[ attribute(VATTR_TEXCOORD_6) ]] ; \n"
"#define VPROG_IN_TEXCOORD7(sz)  float##sz texcoord7 [[ attribute(VATTR_TEXCOORD_7) ]] ; \n"
"#define VPROG_IN_COLOR          float4 color0 [[ attribute(VATTR_COLOR_0) ]] ; \n"
"#define VPROG_IN_COLOR0         float4 color0 [[ attribute(VATTR_COLOR_0) ]] ; \n"
"#define VPROG_IN_COLOR1         float4 color1 [[ attribute(VATTR_COLOR_1) ]] ; \n"
"#define VPROG_IN_TANGENT        float3 tangent [[ attribute(VATTR_TANGENT) ]] ; \n"
"#define VPROG_IN_BINORMAL       float3 binormal [[ attribute(VATTR_BINORMAL) ]] ; \n"
"#define VPROG_IN_BLENDWEIGHT    float3 blendweight [[ attribute(VATTR_BLENDWEIGHT) ]] ; \n"
"#define VPROG_IN_BLENDINDEX(sz) float##sz blendindex [[ attribute(VATTR_BLENDINDEX) ]] ; \n"
"#define VPROG_IN_END            };\n"

"#define VPROG_OUT_BEGIN                       struct VP_Output {\n"
"#define VPROG_OUT_POSITION                    float4 position [[ position ]]; \n"
"#define VPROG_OUT_TEXCOORD0(name,size)        float##size name [[ user(texcoord0) ]];\n"
"#define VPROG_OUT_TEXCOORD1(name,size)        float##size name [[ user(texcoord1) ]];\n"
"#define VPROG_OUT_TEXCOORD2(name,size)        float##size name [[ user(texcoord2) ]];\n"
"#define VPROG_OUT_TEXCOORD3(name,size)        float##size name [[ user(texcoord3) ]];\n"
"#define VPROG_OUT_TEXCOORD4(name,size)        float##size name [[ user(texcoord4) ]];\n"
"#define VPROG_OUT_TEXCOORD5(name,size)        float##size name [[ user(texcoord5) ]];\n"
"#define VPROG_OUT_TEXCOORD6(name,size)        float##size name [[ user(texcoord6) ]];\n"
"#define VPROG_OUT_TEXCOORD7(name,size)        float##size name [[ user(texcoord7) ]];\n"
"#define VPROG_OUT_TEXCOORD0_HALF(name,size)   half##size name [[ user(texcoord0) ]];\n"
"#define VPROG_OUT_TEXCOORD1_HALF(name,size)   half##size name [[ user(texcoord1) ]];\n"
"#define VPROG_OUT_TEXCOORD2_HALF(name,size)   half##size name [[ user(texcoord2) ]];\n"
"#define VPROG_OUT_TEXCOORD3_HALF(name,size)   half##size name [[ user(texcoord3) ]];\n"
"#define VPROG_OUT_TEXCOORD4_HALF(name,size)   half##size name [[ user(texcoord4) ]];\n"
"#define VPROG_OUT_TEXCOORD5_HALF(name,size)   half##size name [[ user(texcoord5) ]];\n"
"#define VPROG_OUT_TEXCOORD6_HALF(name,size)   half##size name [[ user(texcoord6) ]];\n"
"#define VPROG_OUT_TEXCOORD7_HALF(name,size)   half##size name [[ user(texcoord7) ]];\n"
"#define VPROG_OUT_TEXCOORD0_LOW(name,size)    half##size name [[ user(texcoord0) ]];\n"
"#define VPROG_OUT_TEXCOORD1_LOW(name,size)    half##size name [[ user(texcoord1) ]];\n"
"#define VPROG_OUT_TEXCOORD2_LOW(name,size)    half##size name [[ user(texcoord2) ]];\n"
"#define VPROG_OUT_TEXCOORD3_LOW(name,size)    half##size name [[ user(texcoord3) ]];\n"
"#define VPROG_OUT_TEXCOORD4_LOW(name,size)    half##size name [[ user(texcoord4) ]];\n"
"#define VPROG_OUT_TEXCOORD5_LOW(name,size)    half##size name [[ user(texcoord5) ]];\n"
"#define VPROG_OUT_TEXCOORD6_LOW(name,size)    half##size name [[ user(texcoord6) ]];\n"
"#define VPROG_OUT_TEXCOORD7_LOW(name,size)    half##size name [[ user(texcoord7) ]];\n"
"#define VPROG_OUT_COLOR0(name,size)           float##size name [[ user(color0) ]];\n"
"#define VPROG_OUT_COLOR1(name,size)           float##size name [[ user(color1) ]];\n"
"#define VPROG_OUT_COLOR0_HALF(name,size)      half##size name [[ user(color0) ]];\n"
"#define VPROG_OUT_COLOR1_HALF(name,size)      half##size name [[ user(color1) ]];\n"
"#define VPROG_OUT_COLOR0_LOW(name,size)       half##size name [[ user(color0) ]];\n"
"#define VPROG_OUT_COLOR1_LOW(name,size)       half##size name [[ user(color1) ]];\n"
"#define VPROG_OUT_END           };\n"

"#define DECL_VPROG_BUFFER(idx,sz) struct __VP_Buffer##idx { packed_float4 data[sz]; };\n"
"#define VP_BUF_FLOAT3X3(buf,reg)  float3x3( (float3)(float4(VP_Buffer##buf[reg+0])), (float3)(float4(VP_Buffer##buf[reg+1])), (float3)(float4(VP_Buffer##buf[reg+2])) );\n"

"#define VPROG_BEGIN             vertex VP_Output vp_main"
"("
"    VP_Input  in    [[ stage_in ]]"
"    VPROG_IN_BUFFER_0 "
"    VPROG_IN_BUFFER_1 "
"    VPROG_IN_BUFFER_2 "
"    VPROG_IN_BUFFER_3 "
"    VPROG_IN_BUFFER_4 "
"    VPROG_IN_BUFFER_5 "
"    VPROG_IN_BUFFER_6 "
"    VPROG_IN_BUFFER_7 "
"    VPROG_IN_TEXTURE_0 "
"    VPROG_IN_TEXTURE_1 "
"    VPROG_IN_TEXTURE_2 "
"    VPROG_IN_TEXTURE_3 "
"    VPROG_IN_TEXTURE_4 "
"    VPROG_IN_TEXTURE_5 "
"    VPROG_IN_TEXTURE_6 "
"    VPROG_IN_TEXTURE_7 "
"    VPROG_SAMPLER_0 "
"    VPROG_SAMPLER_1 "
"    VPROG_SAMPLER_2 "
"    VPROG_SAMPLER_3 "
"    VPROG_SAMPLER_4 "
"    VPROG_SAMPLER_5 "
"    VPROG_SAMPLER_6 "
"    VPROG_SAMPLER_7 "
")"
"{"
"    VPROG_BUFFER_0 "
"    VPROG_BUFFER_1 "
"    VPROG_BUFFER_2 "
"    VPROG_BUFFER_3 "
"    VPROG_BUFFER_4 "
"    VPROG_BUFFER_5 "
"    VPROG_BUFFER_6 "
"    VPROG_BUFFER_7 "
"    VP_Output   OUT;"
"    VP_Input    IN  = in;\n"

"#define VPROG_END               return OUT;"
"}\n"

"#define VP_IN_POSITION          (float3(IN.position))\n"
"#define VP_IN_NORMAL            (float3(IN.normal))\n"
"#define VP_IN_TEXCOORD          (float2(IN.texcoord0))\n"
"#define VP_IN_TEXCOORD0         IN.texcoord0\n"
"#define VP_IN_TEXCOORD1         IN.texcoord1\n"
"#define VP_IN_TEXCOORD2         IN.texcoord2\n"
"#define VP_IN_TEXCOORD3         IN.texcoord3\n"
"#define VP_IN_TEXCOORD4         IN.texcoord4\n"
"#define VP_IN_TEXCOORD5         IN.texcoord5\n"
"#define VP_IN_TEXCOORD6         IN.texcoord6\n"
"#define VP_IN_TEXCOORD7         IN.texcoord7\n"
"#define VP_IN_COLOR             (float4(IN.color0[0],IN.color0[1],IN.color0[2],IN.color0[3]))\n"
"#define VP_IN_COLOR0            (float4(IN.color0[0],IN.color0[1],IN.color0[2],IN.color0[3]))\n"
"#define VP_IN_COLOR1            (float4(IN.color1[0],IN.color1[1],IN.color1[2],IN.color1[3]))\n"
"#define VP_IN_TANGENT           (float3(IN.tangent))\n"
"#define VP_IN_BINORMAL          (float3(IN.binormal))\n"
"#define VP_IN_BLENDWEIGHT       (float3(IN.blendweight))\n"
"#define VP_IN_BLENDINDEX        (IN.blendindex)\n"

"#define VP_OUT_POSITION         OUT.position\n"
"#define VP_OUT(name)            OUT.name\n"

"#define FPROG_IN_BEGIN                        struct FP_Input { float4 position [[position]]; \n"
"#define FPROG_IN_TEXCOORD0(name,size)         float##size name [[ user(texcoord0) ]];\n"
"#define FPROG_IN_TEXCOORD1(name,size)         float##size name [[ user(texcoord1) ]];\n"
"#define FPROG_IN_TEXCOORD2(name,size)         float##size name [[ user(texcoord2) ]];\n"
"#define FPROG_IN_TEXCOORD3(name,size)         float##size name [[ user(texcoord3) ]];\n"
"#define FPROG_IN_TEXCOORD4(name,size)         float##size name [[ user(texcoord4) ]];\n"
"#define FPROG_IN_TEXCOORD5(name,size)         float##size name [[ user(texcoord5) ]];\n"
"#define FPROG_IN_TEXCOORD6(name,size)         float##size name [[ user(texcoord6) ]];\n"
"#define FPROG_IN_TEXCOORD7(name,size)         float##size name [[ user(texcoord7) ]];\n"
"#define FPROG_IN_TEXCOORD0_HALF(name,size)    half##size name [[ user(texcoord0) ]];\n"
"#define FPROG_IN_TEXCOORD1_HALF(name,size)    half##size name [[ user(texcoord1) ]];\n"
"#define FPROG_IN_TEXCOORD2_HALF(name,size)    half##size name [[ user(texcoord2) ]];\n"
"#define FPROG_IN_TEXCOORD3_HALF(name,size)    half##size name [[ user(texcoord3) ]];\n"
"#define FPROG_IN_TEXCOORD4_HALF(name,size)    half##size name [[ user(texcoord4) ]];\n"
"#define FPROG_IN_TEXCOORD5_HALF(name,size)    half##size name [[ user(texcoord5) ]];\n"
"#define FPROG_IN_TEXCOORD6_HALF(name,size)    half##size name [[ user(texcoord6) ]];\n"
"#define FPROG_IN_TEXCOORD7_HALF(name,size)    half##size name [[ user(texcoord7) ]];\n"
"#define FPROG_IN_TEXCOORD0_LOW(name,size)     half##size name [[ user(texcoord0) ]];\n"
"#define FPROG_IN_TEXCOORD1_LOW(name,size)     half##size name [[ user(texcoord1) ]];\n"
"#define FPROG_IN_TEXCOORD2_LOW(name,size)     half##size name [[ user(texcoord2) ]];\n"
"#define FPROG_IN_TEXCOORD3_LOW(name,size)     half##size name [[ user(texcoord3) ]];\n"
"#define FPROG_IN_TEXCOORD4_LOW(name,size)     half##size name [[ user(texcoord4) ]];\n"
"#define FPROG_IN_TEXCOORD5_LOW(name,size)     half##size name [[ user(texcoord5) ]];\n"
"#define FPROG_IN_TEXCOORD6_LOW(name,size)     half##size name [[ user(texcoord6) ]];\n"
"#define FPROG_IN_TEXCOORD7_LOW(name,size)     half##size name [[ user(texcoord7) ]];\n"
"#define FPROG_IN_COLOR0(name,size)            float##size name [[ user(color0) ]];\n"
"#define FPROG_IN_COLOR1(name,size)            float##size name [[ user(color1) ]];\n"
"#define FPROG_IN_COLOR0_HALF(name,size)       half##size name [[ user(color0) ]];\n"
"#define FPROG_IN_COLOR1_HALF(name,size)       half##size name [[ user(color1) ]];\n"
"#define FPROG_IN_COLOR0_LOW(name,size)        half##size name [[ user(color0) ]];\n"
"#define FPROG_IN_COLOR1_LOW(name,size)        half##size name [[ user(color1) ]];\n"
"#define FPROG_IN_END                          };\n"

"#define FPROG_OUT_BEGIN         struct FP_Output {\n"
"#define FPROG_OUT_COLOR         float4 color [[color(0)]];\n"
"#define FPROG_OUT_END           };\n"

"#define DECL_FP_SAMPLER2D(unit)    \n"
"#define DECL_FP_SAMPLERCUBE(unit)  \n"

"#define DECL_VP_SAMPLER2D(unit)    \n"

"#define FP_TEXTURE2D(unit,uv)   fp_tex##unit.sample( fp_tex##unit##_sampler, uv )\n"
"#define FP_TEXTURECUBE(unit,uv) fp_tex##unit.sample( fp_tex##unit##_sampler, uv )\n"
"#define FP_IN(name)             IN.##name\n"

"#define VP_TEXTURE2D(unit,uv,lod)   vp_tex##unit.sample( vp_tex##unit##_sampler, uv, level(lod) )\n"

"#define FP_OUT_COLOR            OUT.color\n"

"#define DECL_FPROG_BUFFER(idx,sz) struct __FP_Buffer##idx { packed_float4 data[sz]; };\n"

"#define FPROG_BEGIN             fragment FP_Output fp_main"
"("
"    FP_Input IN                 [[ stage_in ]]"
"    FPROG_IN_BUFFER_0 "
"    FPROG_IN_BUFFER_1 "
"    FPROG_IN_BUFFER_2 "
"    FPROG_IN_BUFFER_3 "
"    FPROG_IN_BUFFER_4 "
"    FPROG_IN_BUFFER_5 "
"    FPROG_IN_BUFFER_6 "
"    FPROG_IN_BUFFER_7 "
"    FPROG_IN_TEXTURE_0 "
"    FPROG_IN_TEXTURE_1 "
"    FPROG_IN_TEXTURE_2 "
"    FPROG_IN_TEXTURE_3 "
"    FPROG_IN_TEXTURE_4 "
"    FPROG_IN_TEXTURE_5 "
"    FPROG_IN_TEXTURE_6 "
"    FPROG_IN_TEXTURE_7 "
"    FPROG_SAMPLER_0 "
"    FPROG_SAMPLER_1 "
"    FPROG_SAMPLER_2 "
"    FPROG_SAMPLER_3 "
"    FPROG_SAMPLER_4 "
"    FPROG_SAMPLER_5 "
"    FPROG_SAMPLER_6 "
"    FPROG_SAMPLER_7 "
")"
"{"
"    FPROG_BUFFER_0 "
"    FPROG_BUFFER_1 "
"    FPROG_BUFFER_2 "
"    FPROG_BUFFER_3 "
"    FPROG_BUFFER_4 "
"    FPROG_BUFFER_5 "
"    FPROG_BUFFER_6 "
"    FPROG_BUFFER_7 "
//"    const packed_float4* FP_Buffer0 = buf0->data;"
"    FP_Output   OUT;\n"
"#define FPROG_END               return OUT; }\n"

;

static const char* _ShaderHeader_GLES2 =
"#define float1                 float\n"
"#define float2                 vec2\n"
"#define float3                 vec3\n"
"#define float4                 vec4\n"
"#define float4x4               mat4\n"
"#define float3x3               mat3\n"
"#define vec1                   float\n"
#if 0 //defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
"#define half                   mediump float\n"
"#define half1                  mediump float\n"
"#define half2                  mediump vec2\n"
"#define half3                  mediump vec3\n"
"#define half4                  mediump vec4\n"
"#define min10float             lowp float\n"
"#define min10float1            lowp float\n"
"#define min10float2            lowp vec2\n"
"#define min10float3            lowp vec3\n"
"#define min10float4            lowp vec4\n"
#else
"#define half                   float\n"
"#define half1                  float\n"
"#define half2                  vec2\n"
"#define half3                  vec3\n"
"#define half4                  vec4\n"
"#define min10float             float\n"
"#define min10float1            float\n"
"#define min10float2            vec2\n"
"#define min10float3            vec3\n"
"#define min10float4            vec4\n"
#endif
#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
"#define i_half                 mediump float\n"
"#define i_half1                mediump float\n"
"#define i_half2                mediump vec2\n"
"#define i_half3                mediump vec3\n"
"#define i_half4                mediump vec4\n"
"#define i_min10float           lowp float\n"
"#define i_min10float1          lowp float\n"
"#define i_min10float2          lowp vec2\n"
"#define i_min10float3          lowp vec3\n"
"#define i_min10float4          lowp vec4\n"
#else
"#define i_half                 float\n"
"#define i_half1                float\n"
"#define i_half2                vec2\n"
"#define i_half3                vec3\n"
"#define i_half4                vec4\n"
"#define i_min10float           float\n"
"#define i_min10float1          float\n"
"#define i_min10float2          vec2\n"
"#define i_min10float3          vec3\n"
"#define i_min10float4          vec4\n"
#endif
//"vec4 mul( vec4 v, mat4 m ) { return m*v; }\n"
//"vec4 mul( mat4 m, vec4 v ) { return v*m; }\n"
//"vec3 mul( vec3 v, mat3 m ) { return m*v; }\n"
"#define mul( v, m ) ((m)*(v))\n"

#if defined(__DAVAENGINE_MACOS__)
"#define lerp(a,b,t) ( ( (b) - (a) ) * (t) + (a) )\n"
#else
"#define lerp(a,b,t) mix( (a), (b), (t) )\n"
#endif

"#define  frac(a) fract(a)\n"

"#define fmod(x, y) mod( (x), (y) )\n"

"#define FP_DISCARD_FRAGMENT discard\n"
"#define FP_A8(t) t.a\n"

"#define STEP(edge,x) step( (edge), (x) )\n";

static const char* _ShaderDefine_GLES2 =
"#define VPROG_IN_BEGIN          \n"
"#define VPROG_IN_POSITION       attribute vec4 attr_position;\n"
"#define VPROG_IN_NORMAL         attribute vec3 attr_normal;\n"
"#define VPROG_IN_TEXCOORD       attribute vec2 attr_texcoord0;\n"
"#define VPROG_IN_TEXCOORD0(sz)  attribute vec##sz attr_texcoord0;\n"
"#define VPROG_IN_TEXCOORD1(sz)  attribute vec##sz attr_texcoord1;\n"
"#define VPROG_IN_TEXCOORD2(sz)  attribute vec##sz attr_texcoord2;\n"
"#define VPROG_IN_TEXCOORD3(sz)  attribute vec##sz attr_texcoord3;\n"
"#define VPROG_IN_TEXCOORD4(sz)  attribute vec##sz attr_texcoord4;\n"
"#define VPROG_IN_TEXCOORD5(sz)  attribute vec##sz attr_texcoord5;\n"
"#define VPROG_IN_TEXCOORD6(sz)  attribute vec##sz attr_texcoord6;\n"
"#define VPROG_IN_TEXCOORD7(sz)  attribute vec##sz attr_texcoord7;\n"
"#define VPROG_IN_COLOR          attribute vec4 attr_color0;\n"
"#define VPROG_IN_COLOR0         attribute vec4 attr_color0;\n"
"#define VPROG_IN_COLOR1         attribute vec4 attr_color1;\n"
"#define VPROG_IN_TANGENT        attribute vec3 attr_tangent;\n"
"#define VPROG_IN_BINORMAL       attribute vec3 attr_binormal;\n"
"#define VPROG_IN_BLENDWEIGHT    attribute vec3 attr_blendweight;\n"
"#define VPROG_IN_BLENDINDEX(sz) attribute vec##sz attr_blendindex;\n"
"#define VPROG_IN_END            \n"

"#define VPROG_OUT_BEGIN         \n"
"#define VPROG_OUT_POSITION      \n"
"#define VPROG_OUT_TEXCOORD0(name,size)         varying vec##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD1(name,size)         varying vec##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD2(name,size)         varying vec##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD3(name,size)         varying vec##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD4(name,size)         varying vec##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD5(name,size)         varying vec##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD6(name,size)         varying vec##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD7(name,size)         varying vec##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD0_HALF(name,size)    varying i_half##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD1_HALF(name,size)    varying i_half##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD2_HALF(name,size)    varying i_half##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD3_HALF(name,size)    varying i_half##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD4_HALF(name,size)    varying i_half##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD5_HALF(name,size)    varying i_half##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD6_HALF(name,size)    varying i_half##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD7_HALF(name,size)    varying i_half##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD0_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD1_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD2_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD3_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD4_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD5_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD6_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define VPROG_OUT_TEXCOORD7_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define VPROG_OUT_COLOR0(name,size)            varying vec##size var_##name;\n"
"#define VPROG_OUT_COLOR1(name,size)            varying vec##size var_##name;\n"
"#define VPROG_OUT_COLOR0_HALF(name,size)       varying i_half##size var_##name;\n"
"#define VPROG_OUT_COLOR1_HALF(name,size)       varying i_half##size var_##name;\n"
"#define VPROG_OUT_COLOR0_LOW(name,size)        varying i_min10float##size var_##name;\n"
"#define VPROG_OUT_COLOR1_LOW(name,size)        varying i_min10float##size var_##name;\n"
"#define VPROG_OUT_END           \n"

"#define DECL_VPROG_BUFFER(idx,sz) uniform vec4 VP_Buffer##idx[sz];\n"
"#define VP_BUF_FLOAT3X3(buf,reg)  float3x3( float3(VP_Buffer##buf[reg+0]), float3(VP_Buffer##buf[reg+1]), float3(VP_Buffer##buf[reg+2]) );\n"

"#define VPROG_BEGIN             void main() {\n"
"#define VPROG_END               }\n"

"#define VP_IN_POSITION          attr_position\n"
"#define VP_IN_NORMAL            attr_normal\n"
"#define VP_IN_TEXCOORD          attr_texcoord0\n"
"#define VP_IN_TEXCOORD0         attr_texcoord0\n"
"#define VP_IN_TEXCOORD1         attr_texcoord1\n"
"#define VP_IN_TEXCOORD2         attr_texcoord2\n"
"#define VP_IN_TEXCOORD3         attr_texcoord3\n"
"#define VP_IN_TEXCOORD4         attr_texcoord4\n"
"#define VP_IN_TEXCOORD5         attr_texcoord5\n"
"#define VP_IN_TEXCOORD6         attr_texcoord6\n"
"#define VP_IN_TEXCOORD7         attr_texcoord7\n"
"#define VP_IN_COLOR             attr_color0\n"
"#define VP_IN_COLOR0            attr_color0\n"
"#define VP_IN_COLOR1            attr_color1\n"
"#define VP_IN_TANGENT           attr_tangent\n"
"#define VP_IN_BINORMAL          attr_binormal\n"
"#define VP_IN_BLENDWEIGHT       attr_blendweigh\n"
"#define VP_IN_BLENDINDEX        attr_blendindex\n"

"#define VP_OUT_POSITION         gl_Position\n"
"#define VP_OUT(name)            var_##name\n"

"#define VP_TEXTURE2D(unit,uv,lod)   texture2DLod( VertexTexture##unit, uv, lod)\n"

"#define FPROG_IN_BEGIN          \n"
"#define FPROG_IN_TEXCOORD0(name,size)         varying vec##size var_##name;\n"
"#define FPROG_IN_TEXCOORD1(name,size)         varying vec##size var_##name;\n"
"#define FPROG_IN_TEXCOORD2(name,size)         varying vec##size var_##name;\n"
"#define FPROG_IN_TEXCOORD3(name,size)         varying vec##size var_##name;\n"
"#define FPROG_IN_TEXCOORD4(name,size)         varying vec##size var_##name;\n"
"#define FPROG_IN_TEXCOORD5(name,size)         varying vec##size var_##name;\n"
"#define FPROG_IN_TEXCOORD6(name,size)         varying vec##size var_##name;\n"
"#define FPROG_IN_TEXCOORD7(name,size)         varying vec##size var_##name;\n"
"#define FPROG_IN_TEXCOORD0_HALF(name,size)    varying i_half##size var_##name;\n"
"#define FPROG_IN_TEXCOORD1_HALF(name,size)    varying i_half##size var_##name;\n"
"#define FPROG_IN_TEXCOORD2_HALF(name,size)    varying i_half##size var_##name;\n"
"#define FPROG_IN_TEXCOORD3_HALF(name,size)    varying i_half##size var_##name;\n"
"#define FPROG_IN_TEXCOORD4_HALF(name,size)    varying i_half##size var_##name;\n"
"#define FPROG_IN_TEXCOORD5_HALF(name,size)    varying i_half##size var_##name;\n"
"#define FPROG_IN_TEXCOORD6_HALF(name,size)    varying i_half##size var_##name;\n"
"#define FPROG_IN_TEXCOORD7_HALF(name,size)    varying i_half##size var_##name;\n"
"#define FPROG_IN_TEXCOORD0_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define FPROG_IN_TEXCOORD1_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define FPROG_IN_TEXCOORD2_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define FPROG_IN_TEXCOORD3_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define FPROG_IN_TEXCOORD4_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define FPROG_IN_TEXCOORD5_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define FPROG_IN_TEXCOORD6_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define FPROG_IN_TEXCOORD7_LOW(name,size)     varying i_min10float##size var_##name;\n"
"#define FPROG_IN_COLOR0(name,size)            varying float##size var_##name;\n"
"#define FPROG_IN_COLOR1(name,size)            varying float##size var_##name;\n"
"#define FPROG_IN_COLOR0_HALF(name,size)       varying i_half##size var_##name;\n"
"#define FPROG_IN_COLOR1_HALF(name,size)       varying i_half##size var_##name;\n"
"#define FPROG_IN_COLOR0_LOW(name,size)        varying i_min10float##size var_##name;\n"
"#define FPROG_IN_COLOR1_LOW(name,size)        varying i_min10float##size var_##name;\n"
"#define FPROG_IN_END            \n"

"#define FPROG_OUT_BEGIN         \n"
"#define FPROG_OUT_COLOR         \n"
"#define FPROG_OUT_END           \n"

#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
"#define DECL_FP_SAMPLER2D(unit)    uniform lowp sampler2D FragmentTexture##unit;\n"
"#define DECL_FP_SAMPLERCUBE(unit)  uniform lowp samplerCube FragmentTexture##unit;\n"
"#define DECL_VP_SAMPLER2D(unit)    uniform lowp sampler2D VertexTexture##unit;\n"
#else
"#define DECL_FP_SAMPLER2D(unit)    uniform sampler2D FragmentTexture##unit;\n"
"#define DECL_FP_SAMPLERCUBE(unit)  uniform samplerCube FragmentTexture##unit;\n"
"#define DECL_VP_SAMPLER2D(unit)    uniform sampler2D VertexTexture##unit;\n"
#endif

"#define FP_TEXTURE2D(unit,uv)   texture2D( FragmentTexture##unit, uv )\n"
"#define FP_TEXTURECUBE(unit,uv) textureCube( FragmentTexture##unit, uv )\n"
"#define FP_IN(name)             var_##name\n"

"#define FP_OUT_COLOR            gl_FragColor\n"

"#define DECL_FPROG_BUFFER(idx,sz) uniform vec4 FP_Buffer##idx[sz];\n"

"#define FPROG_BEGIN             void main() {\n"
"#define FPROG_END               }\n"

;

static const char* _ShaderHeader_DX9 =
"#define float1                 float\n"
"#define half1                  half\n"
"#define min10float             half\n"
"#define min10float1            half\n"
"#define min10float2            half2\n"
"#define min10float3            half3\n"
"#define min10float4            half4\n"

"#define FP_DISCARD_FRAGMENT discard\n"
"#define FP_A8(t) t.a\n"

"#define STEP(edge,x) step( (edge), (x) )\n";

static const char* _ShaderDefine_DX9 =
"#define VPROG_IN_BEGIN                 struct VP_Input {\n"
"#define VPROG_IN_POSITION              float3 position : POSITION0;\n"
"#define VPROG_IN_NORMAL                float3 normal : NORMAL0;\n"
"#define VPROG_IN_TEXCOORD              float2 texcoord0 : TEXCOORD0;\n"
"#define VPROG_IN_TEXCOORD0(sz)         float##sz texcoord0 : TEXCOORD0;\n"
"#define VPROG_IN_TEXCOORD1(sz)         float##sz texcoord1 : TEXCOORD1;\n"
"#define VPROG_IN_TEXCOORD2(sz)         float##sz texcoord2 : TEXCOORD2;\n"
"#define VPROG_IN_TEXCOORD3(sz)         float##sz texcoord3 : TEXCOORD3;\n"
"#define VPROG_IN_TEXCOORD4(sz)         float##sz texcoord4 : TEXCOORD4;\n"
"#define VPROG_IN_TEXCOORD5(sz)         float##sz texcoord5 : TEXCOORD5;\n"
"#define VPROG_IN_TEXCOORD6(sz)         float##sz texcoord6 : TEXCOORD6;\n"
"#define VPROG_IN_TEXCOORD7(sz)         float##sz texcoord7 : TEXCOORD7;\n"
"#define VPROG_IN_COLOR                 float4 color0 : COLOR0;\n"
"#define VPROG_IN_COLOR0                float4 color0 : COLOR0;\n"
"#define VPROG_IN_COLOR1                float4 color1 : COLOR1;\n"
"#define VPROG_IN_TANGENT               float3 tangent : TANGENT;\n"
"#define VPROG_IN_BINORMAL              float3 binormal : BINORMAL;\n"
"#define VPROG_IN_BLENDWEIGHT           float3 blendweight : BLENDWEIGHT;\n"
"#define VPROG_IN_BLENDINDEX(sz)        float##sz blendindex : BLENDINDICES;\n"
"#define VPROG_IN_END                   };\n"

"#define VPROG_OUT_BEGIN                        struct VP_Output {\n"
"#define VPROG_OUT_POSITION                     float4 position : POSITION0;\n"
"#define VPROG_OUT_TEXCOORD0(name,size)         float##size name : TEXCOORD0;\n"
"#define VPROG_OUT_TEXCOORD1(name,size)         float##size name : TEXCOORD1;\n"
"#define VPROG_OUT_TEXCOORD2(name,size)         float##size name : TEXCOORD2;\n"
"#define VPROG_OUT_TEXCOORD3(name,size)         float##size name : TEXCOORD3;\n"
"#define VPROG_OUT_TEXCOORD4(name,size)         float##size name : TEXCOORD4;\n"
"#define VPROG_OUT_TEXCOORD5(name,size)         float##size name : TEXCOORD5;\n"
"#define VPROG_OUT_TEXCOORD6(name,size)         float##size name : TEXCOORD6;\n"
"#define VPROG_OUT_TEXCOORD7(name,size)         float##size name : TEXCOORD7;\n"
"#define VPROG_OUT_TEXCOORD0_HALF(name,size)    half##size name : TEXCOORD0;\n"
"#define VPROG_OUT_TEXCOORD1_HALF(name,size)    half##size name : TEXCOORD1;\n"
"#define VPROG_OUT_TEXCOORD2_HALF(name,size)    half##size name : TEXCOORD2;\n"
"#define VPROG_OUT_TEXCOORD3_HALF(name,size)    half##size name : TEXCOORD3;\n"
"#define VPROG_OUT_TEXCOORD4_HALF(name,size)    half##size name : TEXCOORD4;\n"
"#define VPROG_OUT_TEXCOORD5_HALF(name,size)    half##size name : TEXCOORD5;\n"
"#define VPROG_OUT_TEXCOORD6_HALF(name,size)    half##size name : TEXCOORD6;\n"
"#define VPROG_OUT_TEXCOORD7_HALF(name,size)    half##size name : TEXCOORD7;\n"
"#define VPROG_OUT_TEXCOORD0_LOW(name,size)     half##size name : TEXCOORD0;\n"
"#define VPROG_OUT_TEXCOORD1_LOW(name,size)     half##size name : TEXCOORD1;\n"
"#define VPROG_OUT_TEXCOORD2_LOW(name,size)     half##size name : TEXCOORD2;\n"
"#define VPROG_OUT_TEXCOORD3_LOW(name,size)     half##size name : TEXCOORD3;\n"
"#define VPROG_OUT_TEXCOORD4_LOW(name,size)     half##size name : TEXCOORD4;\n"
"#define VPROG_OUT_TEXCOORD5_LOW(name,size)     half##size name : TEXCOORD5;\n"
"#define VPROG_OUT_TEXCOORD6_LOW(name,size)     half##size name : TEXCOORD6;\n"
"#define VPROG_OUT_TEXCOORD7_LOW(name,size)     half##size name : TEXCOORD7;\n"
"#define VPROG_OUT_COLOR0(name,size)            float##size name : COLOR0;\n"
"#define VPROG_OUT_COLOR1(name,size)            float##size name : COLOR1;\n"
"#define VPROG_OUT_COLOR0_HALF(name,size)       half##size name : COLOR0;\n"
"#define VPROG_OUT_COLOR1_HALF(name,size)       half##size name : COLOR1;\n"
"#define VPROG_OUT_COLOR0_LOW(name,size)        half##size name : COLOR0;\n"
"#define VPROG_OUT_COLOR1_LOW(name,size)        half##size name : COLOR1;\n"
"#define VPROG_OUT_END                          };\n"

"#define DECL_VPROG_BUFFER(idx,sz) uniform float4 VP_Buffer##idx[sz];\n"
"#define VP_BUF_FLOAT3X3(buf,reg)  float3x3( (float3)(float4(VP_Buffer##buf[reg+0])), (float3)(float4(VP_Buffer##buf[reg+1])), (float3)(float4(VP_Buffer##buf[reg+2])) );\n"

"#define VPROG_BEGIN             VP_Output vp_main( VP_Input IN ) { VP_Output OUT;\n"
"#define VPROG_END               return OUT; }\n"

"#define VP_IN_POSITION          IN.position\n"
"#define VP_IN_NORMAL            IN.normal\n"
"#define VP_IN_TEXCOORD          IN.texcoord0\n"
"#define VP_IN_TEXCOORD0         IN.texcoord0\n"
"#define VP_IN_TEXCOORD1         IN.texcoord1\n"
"#define VP_IN_TEXCOORD2         IN.texcoord2\n"
"#define VP_IN_TEXCOORD3         IN.texcoord3\n"
"#define VP_IN_TEXCOORD4         IN.texcoord4\n"
"#define VP_IN_TEXCOORD5         IN.texcoord5\n"
"#define VP_IN_TEXCOORD6         IN.texcoord6\n"
"#define VP_IN_TEXCOORD7         IN.texcoord7\n"
"#define VP_IN_COLOR             IN.color0\n"
"#define VP_IN_COLOR0            IN.color0\n"
"#define VP_IN_COLOR1            IN.color1\n"
"#define VP_IN_TANGENT           IN.tangent\n"
"#define VP_IN_BINORMAL          IN.binormal\n"
"#define VP_IN_BLENDWEIGHT       IN.blendweight\n"
"#define VP_IN_BLENDINDEX        IN.blendindex\n"

"#define VP_OUT_POSITION         OUT.position\n"
"#define VP_OUT(name)            OUT.##name\n"

"#define VP_TEXTURE2D(unit,uv,lod)   tex2Dlod( VertexTexture##unit, float4(uv.x,uv.y,0,lod) )\n"

"#define FPROG_IN_BEGIN                        struct FP_Input {\n"
"#define FPROG_IN_TEXCOORD0(name,size)         float##size name : TEXCOORD0;\n"
"#define FPROG_IN_TEXCOORD1(name,size)         float##size name : TEXCOORD1;\n"
"#define FPROG_IN_TEXCOORD2(name,size)         float##size name : TEXCOORD2;\n"
"#define FPROG_IN_TEXCOORD3(name,size)         float##size name : TEXCOORD3;\n"
"#define FPROG_IN_TEXCOORD4(name,size)         float##size name : TEXCOORD4;\n"
"#define FPROG_IN_TEXCOORD5(name,size)         float##size name : TEXCOORD5;\n"
"#define FPROG_IN_TEXCOORD6(name,size)         float##size name : TEXCOORD6;\n"
"#define FPROG_IN_TEXCOORD7(name,size)         float##size name : TEXCOORD7;\n"
"#define FPROG_IN_TEXCOORD0_HALF(name,size)    half##size name : TEXCOORD0;\n"
"#define FPROG_IN_TEXCOORD1_HALF(name,size)    half##size name : TEXCOORD1;\n"
"#define FPROG_IN_TEXCOORD2_HALF(name,size)    half##size name : TEXCOORD2;\n"
"#define FPROG_IN_TEXCOORD3_HALF(name,size)    half##size name : TEXCOORD3;\n"
"#define FPROG_IN_TEXCOORD4_HALF(name,size)    half##size name : TEXCOORD4;\n"
"#define FPROG_IN_TEXCOORD5_HALF(name,size)    half##size name : TEXCOORD5;\n"
"#define FPROG_IN_TEXCOORD6_HALF(name,size)    half##size name : TEXCOORD6;\n"
"#define FPROG_IN_TEXCOORD7_HALF(name,size)    half##size name : TEXCOORD7;\n"
"#define FPROG_IN_TEXCOORD0_LOW(name,size)     half##size name : TEXCOORD0;\n"
"#define FPROG_IN_TEXCOORD1_LOW(name,size)     half##size name : TEXCOORD1;\n"
"#define FPROG_IN_TEXCOORD2_LOW(name,size)     half##size name : TEXCOORD2;\n"
"#define FPROG_IN_TEXCOORD3_LOW(name,size)     half##size name : TEXCOORD3;\n"
"#define FPROG_IN_TEXCOORD4_LOW(name,size)     half##size name : TEXCOORD4;\n"
"#define FPROG_IN_TEXCOORD5_LOW(name,size)     half##size name : TEXCOORD5;\n"
"#define FPROG_IN_TEXCOORD6_LOW(name,size)     half##size name : TEXCOORD6;\n"
"#define FPROG_IN_TEXCOORD7_LOW(name,size)     half##size name : TEXCOORD7;\n"
"#define FPROG_IN_COLOR0(name,size)            float##size name : COLOR0;\n"
"#define FPROG_IN_COLOR1(name,size)            float##size name : COLOR1;\n"
"#define FPROG_IN_COLOR0_HALF(name,size)       half##size name : COLOR0;\n"
"#define FPROG_IN_COLOR1_HALF(name,size)       half##size name : COLOR1;\n"
"#define FPROG_IN_COLOR0_LOW(name,size)        half##size name : COLOR0;\n"
"#define FPROG_IN_COLOR1_LOW(name,size)        half##size name : COLOR1;\n"
"#define FPROG_IN_END                          };\n"

"#define FPROG_OUT_BEGIN         struct FP_Output {\n"
"#define FPROG_OUT_COLOR         float4 color : COLOR0;\n"
"#define FPROG_OUT_END           };\n"

#if defined(__DAVAENGINE_IPHONE__) || defined(__DAVAENGINE_ANDROID__)
"#define DECL_FP_SAMPLER2D(unit)    uniform lowp sampler2D FragmentTexture##unit : TEXUNIT##unit;\n"
"#define DECL_FP_SAMPLERCUBE(unit)  uniform lowp samplerCUBE FragmentTexture##unit : TEXUNIT##unit;\n"
"#define DECL_VP_SAMPLER2D(unit)    uniform lowp sampler2D VertexTexture##unit : TEXUNIT##unit;\n"
#else
"#define DECL_FP_SAMPLER2D(unit)    uniform sampler2D FragmentTexture##unit : TEXUNIT##unit;\n"
"#define DECL_FP_SAMPLERCUBE(unit)  uniform samplerCUBE FragmentTexture##unit : TEXUNIT##unit;\n"
"#define DECL_VP_SAMPLER2D(unit)    uniform sampler2D VertexTexture##unit : TEXUNIT##unit;\n"
#endif

"#define FP_TEXTURE2D(unit,uv)   tex2D( FragmentTexture##unit, uv )\n"
"#define FP_TEXTURECUBE(unit,uv) texCUBE( FragmentTexture##unit, uv )\n"
"#define FP_IN(name)             IN.##name\n"

"#define FP_OUT_COLOR            OUT.color\n"

"#define DECL_FPROG_BUFFER(idx,sz) uniform float4 FP_Buffer##idx[sz];\n"

"#define FPROG_BEGIN             FP_Output fp_main( FP_Input IN ) { FP_Output OUT;\n"
"#define FPROG_END               return OUT; }\n"

;

static const char* _ShaderHeader_DX11 =
"#define float1                 float\n"
"#define half1                  half\n"

"#define FP_DISCARD_FRAGMENT discard\n"
"#define FP_A8(t) t.r\n"

"#define STEP(edge,x) step( (edge), (x) )\n";

static const char* _ShaderDefine_DX11 =
#if !defined(__DAVAENGINE_WIN_UAP__)
"#define float1                 float\n"
"#define half1                  half\n"
"#define min10float             half\n"
"#define min10float1            half\n"
"#define min10float2            half2\n"
"#define min10float3            half3\n"
"#define min10float4            half4\n"
#endif
"#define VPROG_IN_BEGIN                 struct VP_Input {\n"
"#define VPROG_IN_POSITION              float3 position : POSITION;\n"
"#define VPROG_IN_NORMAL                float3 normal : NORMAL;\n"
"#define VPROG_IN_TEXCOORD              float2 texcoord0 : TEXCOORD0;\n"
"#define VPROG_IN_TEXCOORD0(sz)         float##sz texcoord0 : TEXCOORD0;\n"
"#define VPROG_IN_TEXCOORD1(sz)         float##sz texcoord1 : TEXCOORD1;\n"
"#define VPROG_IN_TEXCOORD2(sz)         float##sz texcoord2 : TEXCOORD2;\n"
"#define VPROG_IN_TEXCOORD3(sz)         float##sz texcoord3 : TEXCOORD3;\n"
"#define VPROG_IN_TEXCOORD4(sz)         float##sz texcoord4 : TEXCOORD4;\n"
"#define VPROG_IN_TEXCOORD5(sz)         float##sz texcoord5 : TEXCOORD5;\n"
"#define VPROG_IN_TEXCOORD6(sz)         float##sz texcoord6 : TEXCOORD6;\n"
"#define VPROG_IN_TEXCOORD7(sz)         float##sz texcoord7 : TEXCOORD7;\n"
"#define VPROG_IN_COLOR                 float4 color0 : COLOR0;\n"
"#define VPROG_IN_COLOR0                float4 color0 : COLOR0;\n"
"#define VPROG_IN_COLOR1                float4 color1 : COLOR1;\n"
"#define VPROG_IN_TANGENT               float3 tangent : TANGENT;\n"
"#define VPROG_IN_BINORMAL              float3 binormal : BINORMAL;\n"
"#define VPROG_IN_BLENDWEIGHT           float3 blendweight : BLENDWEIGHT;\n"
"#define VPROG_IN_BLENDINDEX(sz)        float##sz blendindex : BLENDINDICES;\n"
"#define VPROG_IN_END                   };\n"

"#define VPROG_OUT_BEGIN                        struct VP_Output {\n"
"#define VPROG_OUT_POSITION                     float4 position : SV_POSITION;\n"
"#define VPROG_OUT_TEXCOORD0(name,size)         float##size name : TEXCOORD0;\n"
"#define VPROG_OUT_TEXCOORD1(name,size)         float##size name : TEXCOORD1;\n"
"#define VPROG_OUT_TEXCOORD2(name,size)         float##size name : TEXCOORD2;\n"
"#define VPROG_OUT_TEXCOORD3(name,size)         float##size name : TEXCOORD3;\n"
"#define VPROG_OUT_TEXCOORD4(name,size)         float##size name : TEXCOORD4;\n"
"#define VPROG_OUT_TEXCOORD5(name,size)         float##size name : TEXCOORD5;\n"
"#define VPROG_OUT_TEXCOORD6(name,size)         float##size name : TEXCOORD6;\n"
"#define VPROG_OUT_TEXCOORD7(name,size)         float##size name : TEXCOORD7;\n"
"#define VPROG_OUT_TEXCOORD0_HALF(name,size)    half##size name : TEXCOORD0;\n"
"#define VPROG_OUT_TEXCOORD1_HALF(name,size)    half##size name : TEXCOORD1;\n"
"#define VPROG_OUT_TEXCOORD2_HALF(name,size)    half##size name : TEXCOORD2;\n"
"#define VPROG_OUT_TEXCOORD3_HALF(name,size)    half##size name : TEXCOORD3;\n"
"#define VPROG_OUT_TEXCOORD4_HALF(name,size)    half##size name : TEXCOORD4;\n"
"#define VPROG_OUT_TEXCOORD5_HALF(name,size)    half##size name : TEXCOORD5;\n"
"#define VPROG_OUT_TEXCOORD6_HALF(name,size)    half##size name : TEXCOORD6;\n"
"#define VPROG_OUT_TEXCOORD7_HALF(name,size)    half##size name : TEXCOORD7;\n"
"#define VPROG_OUT_TEXCOORD0_LOW(name,size)     half##size name : TEXCOORD0;\n"
"#define VPROG_OUT_TEXCOORD1_LOW(name,size)     half##size name : TEXCOORD1;\n"
"#define VPROG_OUT_TEXCOORD2_LOW(name,size)     half##size name : TEXCOORD2;\n"
"#define VPROG_OUT_TEXCOORD3_LOW(name,size)     half##size name : TEXCOORD3;\n"
"#define VPROG_OUT_TEXCOORD4_LOW(name,size)     half##size name : TEXCOORD4;\n"
"#define VPROG_OUT_TEXCOORD5_LOW(name,size)     half##size name : TEXCOORD5;\n"
"#define VPROG_OUT_TEXCOORD6_LOW(name,size)     half##size name : TEXCOORD6;\n"
"#define VPROG_OUT_TEXCOORD7_LOW(name,size)     half##size name : TEXCOORD7;\n"
"#define VPROG_OUT_COLOR0(name,size)            float##size name : COLOR0;\n"
"#define VPROG_OUT_COLOR1(name,size)            float##size name : COLOR1;\n"
"#define VPROG_OUT_COLOR0_HALF(name,size)       half##size name : COLOR0;\n"
"#define VPROG_OUT_COLOR1_HALF(name,size)       half##size name : COLOR1;\n"
"#define VPROG_OUT_COLOR0_LOW(name,size)        half##size name : COLOR0;\n"
"#define VPROG_OUT_COLOR1_LOW(name,size)        half##size name : COLOR1;\n"
"#define VPROG_OUT_END                          };\n"

//"#define DECL_VPROG_BUFFER(idx,sz) uniform float4 VP_Buffer##idx[sz];\n"
"#define DECL_VPROG_BUFFER(idx,sz) cbuffer VP_Buffer##idx##_t : register(b##idx) { float4 VP_Buffer##idx[sz]; };\n"
"#define VP_BUF_FLOAT3X3(buf,reg)  float3x3( (float3)(float4(VP_Buffer##buf[reg+0])), (float3)(float4(VP_Buffer##buf[reg+1])), (float3)(float4(VP_Buffer##buf[reg+2])) );\n"

"#define VPROG_BEGIN             VP_Output vp_main( VP_Input IN ) { VP_Output OUT;\n"
"#define VPROG_END               return OUT; }\n"

"#define VP_IN_POSITION          IN.position\n"
"#define VP_IN_NORMAL            IN.normal\n"
"#define VP_IN_TEXCOORD          IN.texcoord0\n"
"#define VP_IN_TEXCOORD0         IN.texcoord0\n"
"#define VP_IN_TEXCOORD1         IN.texcoord1\n"
"#define VP_IN_TEXCOORD2         IN.texcoord2\n"
"#define VP_IN_TEXCOORD3         IN.texcoord3\n"
"#define VP_IN_TEXCOORD4         IN.texcoord4\n"
"#define VP_IN_TEXCOORD5         IN.texcoord5\n"
"#define VP_IN_TEXCOORD6         IN.texcoord6\n"
"#define VP_IN_TEXCOORD7         IN.texcoord7\n"
"#define VP_IN_COLOR             IN.color0\n"
"#define VP_IN_COLOR0            IN.color0\n"
"#define VP_IN_COLOR1            IN.color1\n"
"#define VP_IN_TANGENT           IN.tangent\n"
"#define VP_IN_BINORMAL          IN.binormal\n"
"#define VP_IN_BLENDWEIGHT       IN.blendweight\n"
"#define VP_IN_BLENDINDEX        IN.blendindex\n"

"#define VP_OUT_POSITION         OUT.position\n"
"#define VP_OUT(name)            OUT.##name\n"

"#define VP_TEXTURE2D(unit,uv,lod)   VertexTexture##unit.SampleLevel( VertexTexture##unit##_Sampler, uv, lod )\n"

"#define FPROG_IN_BEGIN                        struct FP_Input { float4 pos : SV_POSITION; \n"
"#define FPROG_IN_TEXCOORD0(name,size)         float##size name : TEXCOORD0;\n"
"#define FPROG_IN_TEXCOORD1(name,size)         float##size name : TEXCOORD1;\n"
"#define FPROG_IN_TEXCOORD2(name,size)         float##size name : TEXCOORD2;\n"
"#define FPROG_IN_TEXCOORD3(name,size)         float##size name : TEXCOORD3;\n"
"#define FPROG_IN_TEXCOORD4(name,size)         float##size name : TEXCOORD4;\n"
"#define FPROG_IN_TEXCOORD5(name,size)         float##size name : TEXCOORD5;\n"
"#define FPROG_IN_TEXCOORD6(name,size)         float##size name : TEXCOORD6;\n"
"#define FPROG_IN_TEXCOORD7(name,size)         float##size name : TEXCOORD7;\n"
"#define FPROG_IN_TEXCOORD0_HALF(name,size)    half##size name : TEXCOORD0;\n"
"#define FPROG_IN_TEXCOORD1_HALF(name,size)    half##size name : TEXCOORD1;\n"
"#define FPROG_IN_TEXCOORD2_HALF(name,size)    half##size name : TEXCOORD2;\n"
"#define FPROG_IN_TEXCOORD3_HALF(name,size)    half##size name : TEXCOORD3;\n"
"#define FPROG_IN_TEXCOORD4_HALF(name,size)    half##size name : TEXCOORD4;\n"
"#define FPROG_IN_TEXCOORD5_HALF(name,size)    half##size name : TEXCOORD5;\n"
"#define FPROG_IN_TEXCOORD6_HALF(name,size)    half##size name : TEXCOORD6;\n"
"#define FPROG_IN_TEXCOORD7_HALF(name,size)    half##size name : TEXCOORD7;\n"
"#define FPROG_IN_TEXCOORD0_LOW(name,size)     half##size name : TEXCOORD0;\n"
"#define FPROG_IN_TEXCOORD1_LOW(name,size)     half##size name : TEXCOORD1;\n"
"#define FPROG_IN_TEXCOORD2_LOW(name,size)     half##size name : TEXCOORD2;\n"
"#define FPROG_IN_TEXCOORD3_LOW(name,size)     half##size name : TEXCOORD3;\n"
"#define FPROG_IN_TEXCOORD4_LOW(name,size)     half##size name : TEXCOORD4;\n"
"#define FPROG_IN_TEXCOORD5_LOW(name,size)     half##size name : TEXCOORD5;\n"
"#define FPROG_IN_TEXCOORD6_LOW(name,size)     half##size name : TEXCOORD6;\n"
"#define FPROG_IN_TEXCOORD7_LOW(name,size)     half##size name : TEXCOORD7;\n"
"#define FPROG_IN_COLOR0(name,size)            float##size name : COLOR0;\n"
"#define FPROG_IN_COLOR1(name,size)            float##size name : COLOR1;\n"
"#define FPROG_IN_COLOR0_HALF(name,size)       half##size name : COLOR0;\n"
"#define FPROG_IN_COLOR1_HALF(name,size)       half##size name : COLOR1;\n"
"#define FPROG_IN_COLOR0_LOW(name,size)        half##size name : COLOR0;\n"
"#define FPROG_IN_COLOR1_LOW(name,size)        half##size name : COLOR1;\n"
"#define FPROG_IN_END                          };\n"

"#define FPROG_OUT_BEGIN         struct FP_Output {\n"
"#define FPROG_OUT_COLOR         float4 color : SV_TARGET0;\n"
"#define FPROG_OUT_END           };\n"

"#define DECL_FP_SAMPLER2D(unit)    Texture2D FragmentTexture##unit : register(t##unit); SamplerState FragmentTexture##unit##_Sampler : register(s##unit);\n"
"#define DECL_FP_SAMPLERCUBE(unit)    TextureCube FragmentTexture##unit : register(t##unit); SamplerState FragmentTexture##unit##_Sampler : register(s##unit);\n"
"#define DECL_VP_SAMPLER2D(unit)    Texture2D VertexTexture##unit : register(t##unit); SamplerState VertexTexture##unit##_Sampler : register(s##unit);\n"

"#define FP_TEXTURE2D(unit,uv)   FragmentTexture##unit.Sample( FragmentTexture##unit##_Sampler, uv )\n"
"#define FP_TEXTURECUBE(unit,uv) FragmentTexture##unit.Sample( FragmentTexture##unit##_Sampler, uv )\n"
"#define FP_IN(name)             IN.##name\n"

"#define FP_OUT_COLOR            OUT.color\n"

"#define DECL_FPROG_BUFFER(idx,sz) cbuffer FP_Buffer##idx##_t : register(b##idx) { float4 FP_Buffer##idx[sz]; };\n"

"#define FPROG_BEGIN             FP_Output fp_main( FP_Input IN ) { FP_Output OUT;\n"
"#define FPROG_END               return OUT; }\n"

;
*/
//------------------------------------------------------------------------------

void UpdateProg(Api targetApi, ProgType progType, const DAVA::FastName& uid, const char* srcText)
{
    ShaderSource src;

    if (src.Construct(progType, srcText))
    {
        const std::string& code = src.GetSourceCode(targetApi);

        UpdateProgBinary(targetApi, progType, uid, code.c_str(), unsigned(code.length()));
        //DAVA::Logger::Info("\n\n--shader  \"%s\"", uid.c_str());
        //DAVA::Logger::Info(code.c_str());
    }
}

//------------------------------------------------------------------------------

void UpdateProgBinary(Api targetApi, ProgType progType, const DAVA::FastName& uid, const void* bin, unsigned binSize)
{
    std::vector<uint8>* pbin = nullptr;

    for (unsigned i = 0; i != _ProgInfo.size(); ++i)
    {
        if (_ProgInfo[i].uid == uid)
        {
            pbin = &(_ProgInfo[i].bin);
            break;
        }
    }

    if (!pbin)
    {
        _ProgInfo.push_back(ProgInfo());

        _ProgInfo.back().uid = uid;
        pbin = &(_ProgInfo.back().bin);
    }

    //- DAVA::Logger::Info("\n\n--shader  \"%s\"", uid.c_str());
    //- DAVA::Logger::Info((const char*)bin);
    pbin->clear();
    pbin->insert(pbin->begin(), reinterpret_cast<const uint8*>(bin), reinterpret_cast<const uint8*>(bin) + binSize);
    pbin->push_back(0);
}

} // namespace ShaderCache
} // namespace rhi
