#include "blending.slh"
#include "common.slh"

#define COLOR_MUL 0
#define COLOR_ADD 1
#define ALPHA_MUL 2
#define ALPHA_ADD 3

#ifndef COLOR_OP
#define COLOR_OP COLOR_MUL
#endif

fragment_in
{
#if TEXTURED
    float2  uv      : TEXCOORD0;
#endif //TEXTURED
    [lowp] half4   color   : COLOR0;
};
    
fragment_out
{
    float4  color : SV_TARGET0;
};

#if TEXTURED
uniform sampler2D tex;
#endif //TEXTURED


fragment_out fp_main( fragment_in input )
{
    fragment_out    output;

    half4 in_color = input.color;

#if TEXTURED

    float2 in_uv = input.uv;

#if ALPHA8
    half4 resColor = half4( 1.0, 1.0, 1.0, FP_A8(tex2D( tex, in_uv )) );
#else
    half4 resColor = half4(tex2D( tex, in_uv ));
#endif

#if (COLOR_OP == COLOR_MUL)
    resColor = resColor * in_color;
#elif (COLOR_OP == COLOR_ADD)
    resColor = resColor + in_color;
#elif (COLOR_OP == ALPHA_MUL)
    resColor.a = resColor.a * in_color.a;
#elif (COLOR_OP == ALPHA_ADD)
    resColor.a = resColor.a + in_color.a;
#endif

#else //TEXTURED

    half4 resColor = in_color;

#endif //TEXTURED

#if GRAYSCALE
    half gray = dot(resColor.rgb, half3(0.3, 0.59, 0.11));
    resColor.rgb = half3(gray,gray,gray);
#endif

#if ALPHATEST
    half alpha = resColor.a;
    if( alpha < 0.5 ) discard;
#endif

    output.color = float4(resColor);
    
    return output;
}
