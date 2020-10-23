#include "common.slh"

fragment_in
{
    float2  texCoord        : TEXCOORD0;
    [lowp] half3   vegetationColor : COLOR0;

    #if VERTEX_FOG
    float4  varFog          : TEXCOORD5;
    #endif
};

fragment_out
{
    float4  color : SV_TARGET0;
};

uniform sampler2D albedo;

#if LOD_COLOR
[material][instance] property float3 lodColor = float3(1,1,1);
#endif


fragment_out fp_main( fragment_in input )
{
    fragment_out    output;

    float4 textureColor0 = tex2D( albedo, input.texCoord );
    float3 color = textureColor0.rgb * float3(input.vegetationColor) * 2.0;

#if LOD_COLOR
    color += lodColor;
#endif
        
    output.color = float4(color, 1.0);

    #if VERTEX_FOG
        
        float   varFogAmoung = input.varFog.a;
        float3  varFogColor  = input.varFog.rgb;
        
        #if !FRAMEBUFFER_FETCH
            //VI: fog equation is inside of color equation for framebuffer fetch
            output.color.rgb = lerp( output.color.rgb, varFogColor, varFogAmoung );
        #endif
    #endif

    return output;
}
