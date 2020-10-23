#include "common.slh"

fragment_in
{    
    float2 texCoord : TEXCOORD0;
    
    #if TILEMASK
    float2 texCoordTiled : TEXCOORD1;
    #endif
    
    #if LANDSCAPE_SPECULAR
        float3 varHalfVec       : TEXCOORD2;
        float3 varToLightVec    : TEXCOORD3;
        float3 varToCameraVec   : TEXCOORD4;
    #endif
    
    #if VERTEX_FOG
        float4 varFog : TEXCOORD5;
    #endif

    #if LANDSCAPE_MORPHING_COLOR
        float4 morphColor : COLOR0; //patch color
    #endif
    
};

fragment_out
{
    float4 color : SV_TARGET0;
};

#if TILEMASK
    uniform sampler2D tileTexture0;
    uniform sampler2D tileMask;
    uniform sampler2D colorTexture;

    [material][instance] property float3 tileColor0 = float3(1,1,1);
    [material][instance] property float3 tileColor1 = float3(1,1,1);
    [material][instance] property float3 tileColor2 = float3(1,1,1);
    [material][instance] property float3 tileColor3 = float3(1,1,1);
#else
    uniform sampler2D fullTiledTexture;
#endif

#if CURSOR
uniform sampler2D cursorTexture;
[material][instance] property float4 cursorCoordSize = float4(0,0,1,1);
#endif

#if LANDSCAPE_SPECULAR
    [material][a] property float inGlossiness             = 0.5;
    [material][a] property float inSpecularity            = 1.0;
    [material][a] property float3 metalFresnelReflectance = float3(0.5,0.5,0.5);
    [material][a] property float4 lightPosition0;
    [material][a] property float3 lightColor0             = float3(1,1,1);
    
    inline float3 FresnelShlickVec3( float NdotL, float3 Cspec )
    {
        float fresnel_exponent = 5.0;
        return (1.0 - Cspec) * (pow(1.0 - NdotL, fresnel_exponent)) + Cspec;
    }
#endif


fragment_out fp_main( fragment_in input )
{
    fragment_out    output;

#ifdef TILEMASK
    float4 tileColor = tex2D( tileTexture0, input.texCoordTiled );
    float4 mask = tex2D( tileMask, input.texCoord );
    float4 color = tex2D( colorTexture, input.texCoord );
    
    float3 color3 = (tileColor.r * mask.r * tileColor0.rgb + 
                     tileColor.g * mask.g * tileColor1.rgb + 
                     tileColor.b * mask.b * tileColor2.rgb + 
                     tileColor.a * mask.a * tileColor3.rgb ) * color.rgb * 2.0;

#else
    float3 color3 = tex2D(fullTiledTexture, input.texCoord).rgb;
#endif                     
         
    float4 outColor = float4(color3, 1.0);
    
#if LANDSCAPE_LOD_MORPHING && LANDSCAPE_MORPHING_COLOR
    outColor = outColor * 0.25 + input.morphColor * 0.75;
#endif
    
#if CURSOR
    float2 cursorCoord = (input.texCoord + cursorCoordSize.xy) / cursorCoordSize.zw + float2(0.5, 0.5);
    float4 cursorColor = tex2D( cursorTexture, cursorCoord );
    outColor.rgb *= 1.0 - cursorColor.a;
    outColor.rgb += cursorColor.rgb * cursorColor.a;
#endif
    
#if LANDSCAPE_SPECULAR

    float3 normal = float3(0.0, 0.0, 1.0); //fetch from normalmap here

    float3 normalizedHalf = normalize(input.varHalfVec.xyz);
            
    float NdotL = max (dot (normal, input.varToLightVec.xyz), 0.0);
    float NdotH = max (dot (normal, normalizedHalf), 0.0);
    float LdotH = max (dot (input.varToLightVec.xyz, normalizedHalf), 0.0);
    float NdotV = max (dot (normal, input.varToCameraVec), 0.0);

    float3 fresnelOut = FresnelShlickVec3( NdotV, metalFresnelReflectance );
    
    float glossiness = inGlossiness * color.a;
    float glossPower = pow(5000.0, glossiness);
    float specularNorm = (glossiness + 2.0) / 8.0;
    float specularNormalized = specularNorm * pow(NdotH, glossPower) * NdotL * inSpecularity;
    float3 specular = specularNormalized * fresnelOut;
    outColor.rgb += specular * lightColor0;
    
#endif

    output.color = outColor;    
    
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
