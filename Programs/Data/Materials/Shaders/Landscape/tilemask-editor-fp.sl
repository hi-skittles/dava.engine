#include "common.slh"

#define DRAW_TYPE_R_CHANNEL  0
#define DRAW_TYPE_G_CHANNEL  1
#define DRAW_TYPE_B_CHANNEL  2
#define DRAW_TYPE_A_CHANNEL  3
#define DRAW_TYPE_COPY_PASTE 4
#ensuredefined DRAW_TYPE 0

fragment_in
{
    float2  uv      : TEXCOORD0;
};

fragment_out
{
    float4  color   : SV_TARGET0;
};


uniform sampler2D sourceTexture;
uniform sampler2D toolTexture;

[material][instance] property float intensity = 1.0;

#if DRAW_TYPE == DRAW_TYPE_COPY_PASTE
[material][instance] property float2 copypasteOffset = float2(0,0);
#endif

fragment_out fp_main( fragment_in input )
{
    fragment_out    output;

    float4 colorMaskOld = tex2D( sourceTexture, input.uv );
    float toolValue = tex2D( toolTexture, input.uv ).r;
    float4 outColor = float4(1.0, 1.0, 1.0, 1.0);

#if DRAW_TYPE == DRAW_TYPE_R_CHANNEL

    outColor.r = colorMaskOld.r+toolValue*intensity;
    outColor.r = min(outColor.r, 1.0);
    float freeColors = 1.0-outColor.r;
    float usedColors = colorMaskOld.g+colorMaskOld.b+colorMaskOld.a;
    float div = usedColors/freeColors; // /0?
    outColor.gba = colorMaskOld.gba/div;

#elif DRAW_TYPE == DRAW_TYPE_G_CHANNEL

    outColor.g = colorMaskOld.g+toolValue*intensity;
    outColor.g = min(outColor.g, 1.0);
    float freeColors = 1.0-outColor.g;
    float usedColors = colorMaskOld.r+colorMaskOld.b+colorMaskOld.a;
    float div = usedColors/freeColors; // /0?
    outColor.rba = colorMaskOld.rba/div;

#elif DRAW_TYPE == DRAW_TYPE_B_CHANNEL

    outColor.b = colorMaskOld.b+toolValue*intensity;
    outColor.b = min(outColor.b, 1.0);
    float freeColors = 1.0-outColor.b;
    float usedColors = colorMaskOld.r+colorMaskOld.g+colorMaskOld.a;
    float div = usedColors/freeColors; // /0?
    outColor.rga = colorMaskOld.rga/div;

#elif DRAW_TYPE == DRAW_TYPE_A_CHANNEL

    outColor.a = colorMaskOld.a+toolValue*intensity;
    outColor.a = min(outColor.a, 1.0);
    float freeColors = 1.0-outColor.a;
    float usedColors = colorMaskOld.r+colorMaskOld.g+colorMaskOld.b;
    float div = usedColors/freeColors; // /0?
    outColor.rgb = colorMaskOld.rgb/div;

#elif DRAW_TYPE == DRAW_TYPE_COPY_PASTE

    float4 colorMaskNew = tex2D( sourceTexture, input.uv + copypasteOffset );
    outColor = lerp( colorMaskOld, colorMaskNew, toolValue);

#endif

    output.color = outColor;
    
    return output;
}

