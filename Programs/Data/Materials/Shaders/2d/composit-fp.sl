blending { src=one dst=inv_src_alpha }

#define GRADIENT_MULTIPLY 0
#define GRADIENT_BLEND    1
#define GRADIENT_ADD      2
#define GRADIENT_SCREEN   3
#define GRADIENT_OVERLAY  4

#ifndef GRADIENT_MODE
#define GRADIENT_MODE GRADIENT_MULTIPLY
#endif

fragment_in
{
    float2  uvMask      : TEXCOORD0;
    float2  uvDetail    : TEXCOORD1;
    float2  uvGradient  : TEXCOORD2;
    float2  uvContour   : TEXCOORD3;
    [lowp] half4   color       : COLOR0;
};

fragment_out
{
    float4  color       : SV_TARGET0;
};        

sampler2D mask;
sampler2D detail;
sampler2D gradient;
sampler2D contour;


fragment_out fp_main( fragment_in input )
{
    fragment_out    output;

    half4 in_color = input.color;
    
    //fetch
    float maskColor = tex2D(mask, input.uvMask).a;
    float4 detailColor = tex2D(detail, input.uvDetail); 
    float4 gradientColor = tex2D(gradient, input.uvGradient);
    float4 contourColor = tex2D(contour, input.uvContour);
    
    #if GRADIENT_MODE == GRADIENT_MULTIPLY
        float4 detailImpact = float4(detailColor.rgb * gradientColor.rgb, maskColor);
    #elif GRADIENT_MODE == GRADIENT_BLEND
        float4 detailImpact = float4(lerp(detailColor.rgb, gradientColor.rgb, gradientColor.a), maskColor);
    #elif GRADIENT_MODE == GRADIENT_ADD
        float4 detailImpact = float4(detailColor.rgb + gradientColor.rgb, maskColor);
    #elif GRADIENT_MODE == GRADIENT_SCREEN
        float4 detailImpact = float4(float3(1.0, 1.0, 1.0) - (float3(1.0, 1.0, 1.0)-detailColor.rgb) * (float3(1.0, 1.0, 1.0) - gradientColor.rgb), maskColor);
    #elif GRADIENT_MODE == GRADIENT_OVERLAY
        float3 overlayLow = 2.0 * detailColor.rgb * gradientColor.rgb;
        float3 overlayHi = float3(1.0, 1.0, 1.0) - 2.0* (float3(1.0, 1.0, 1.0)-detailColor.rgb) * (float3(1.0, 1.0, 1.0) - gradientColor.rgb);
        float3 detailImpactColor = lerp(overlayLow, overlayHi, step(0.5, detailColor.rgb));
        float4 detailImpact = float4(detailImpactColor, maskColor);
    #endif

    float3 detailPremult = detailImpact.rgb*detailImpact.a;
    float4 resColor = float4((contourColor.rgb-detailPremult)*contourColor.a+detailPremult, detailImpact.a + contourColor.a - detailImpact.a * contourColor.a);

    in_color.rgb *= in_color.a; //as we are using premultipled alpha blending
    resColor = resColor * float4(in_color);

    output.color = resColor; 
    
    return output;
}
