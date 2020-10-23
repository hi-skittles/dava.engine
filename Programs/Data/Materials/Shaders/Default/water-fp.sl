#include "common.slh"

#if FRESNEL_TO_ALPHA
    blending { src=src_alpha dst=inv_src_alpha }
#endif

#define SHADING_PERVERTEX 0
#define SHADING_PERPIXEL  1

//convert old defines to new style
#if PIXEL_LIT
#define SHADING SHADING_PERPIXEL
#endif
#if VERTEX_LIT
#define SHADING SHADING_PERVERTEX
#endif

fragment_in
{
    float2 uv  : TEXCOORD0;
    float2 uv1 : TEXCOORD1;

    #if SHADING == SHADING_PERPIXEL
        half3   cameraToPointInTangentSpace : TEXCOORD2;

        #if REAL_REFLECTION
            float3  eyeDist : TEXCOORD3;
            float4  normalizedFragPos : TEXCOORD4;
            #if SPECULAR
                float3  varLightVec : TEXCOORD5;
            #endif
        #else
            half3 tbnToWorld0 : TEXCOORD3;
            half3 tbnToWorld1 : TEXCOORD4;
            half3 tbnToWorld2 : TEXCOORD5;
        #endif

    #endif

    #if SHADING == SHADING_PERVERTEX 
        float2 varTexCoordDecal : TEXCOORD2;
        float3 reflectionDirectionInWorldSpace : TEXCOORD3;
    #endif
    
    #if VERTEX_FOG
        float4 varFog : TEXCOORD6;
    #endif

};

fragment_out
{
    float4  color : SV_TARGET0;
};

#if SHADING == SHADING_PERPIXEL
    #if REAL_REFLECTION
        uniform sampler2D dynamicReflection;
        uniform sampler2D dynamicRefraction;
    #endif

    #if !DEBUG_UNITY_Z_NORMAL
        uniform sampler2D normalmap;
        #if (SEPARATE_NORMALMAPS)
            uniform sampler2D normalmap1;
        #endif
    #endif
#elif SHADING == SHADING_PERVERTEX
    uniform sampler2D albedo;
    uniform sampler2D decal;
#endif

#if !REAL_REFLECTION
    uniform samplerCUBE cubemap;
#endif


//properties
#if SHADING == SHADING_PERPIXEL
    #if REAL_REFLECTION
        [material][instance] property float  distortionFallSquareDist = 1.0;
        [material][instance] property float  reflectionDistortion     = 0;
        [material][instance] property float  refractionDistortion     = 0;
        [material][instance] property float3 refractionTintColor      = float3(1,1,1);
        #if SPECULAR
            [material][instance] property float  inGlossiness         = 0.5;
            [material][instance] property float  inSpecularity        = 0.5;
            [auto][instance] property float3 lightColor0;            
        #endif
    #endif

    [material][instance] property float3 reflectionTintColor          = float3(1,1,1);
    [material][instance] property float  fresnelBias                  = 0;
    [material][instance] property float  fresnelPow                   = 0.0;
    
    #if DEBUG_Z_NORMAL_SCALE && !DEBUG_UNITY_Z_NORMAL
        [material][instance] property float  normal0_z_scale = 1;
        [material][instance] property float  normal1_z_scale = 1;
    #endif
#elif SHADING == SHADING_PERVERTEX
    [material][instance] property float3 decalTintColor     = float3(0,0,0);
    [material][instance] property float3 reflectanceColor   = float3(0,0,0);
#endif


inline float 
FresnelShlick(float _NdotL, float _fresnelBias, float _fresnelPow)
{
    return _fresnelBias + (1.0 - _fresnelBias) * pow(1.0 - _NdotL, _fresnelPow);
}


fragment_out fp_main( fragment_in input )
{
    fragment_out output;


    float2 varTexCoord0 = input.uv;
    float2 varTexCoord1 = input.uv1;

#if SHADING == SHADING_PERPIXEL

//compute normal
    float3 normal;
    #if !DEBUG_UNITY_Z_NORMAL
        float3 normal0 = tex2D( normalmap, varTexCoord0 ).rgb;
        #if SEPARATE_NORMALMAPS
            float3 normal1 = tex2D( normalmap1, varTexCoord1).rgb;    
        #else
            float3 normal1 = tex2D( normalmap, varTexCoord1).rgb;
        #endif
        
        #if DEBUG_Z_NORMAL_SCALE
            normal0 = normal0 * 2.0 - 1.0;
            normal1 = normal1 * 2.0 - 1.0;
            normal0.xy *= normal0_z_scale;
            normal1.xy *= normal1_z_scale;
            normal = normalize (normal0 + normal1);
        #else    
            normal = normalize (normal0 + normal1 - 1.0); //same as * 2 -2
        #endif
    
    #else
        normal = float3(0.0,0.0,1.0);
    #endif    
    
    
//compute fresnel    
    float3 cameraToPointInTangentSpaceNorm = float3(normalize(input.cameraToPointInTangentSpace));    
    float lambertFactor = max (dot (-cameraToPointInTangentSpaceNorm, normal), 0.0);
    float fresnel = FresnelShlick(lambertFactor, fresnelBias, fresnelPow);
    
    #if REAL_REFLECTION        
        float3 eyeDist = input.eyeDist;    
        float2 waveOffset = normal.xy*max(0.1, 1.0-dot(eyeDist, eyeDist)*distortionFallSquareDist);
        float4 fragPos = input.normalizedFragPos;
        float2 texturePos =  fragPos.xy/fragPos.w*0.5 + 0.5;        
        float3 reflectionColor = tex2D(dynamicReflection, texturePos+waveOffset*reflectionDistortion).rgb;
        texturePos.y=1.0-texturePos.y;
        float3 refractionColor = tex2D(dynamicRefraction, texturePos+waveOffset*refractionDistortion).rgb * refractionTintColor;
        float3 resColor = lerp(refractionColor, reflectionColor*reflectionTintColor, fresnel);
        
        #if SPECULAR
            float3 halfVec = normalize(normalize(input.varLightVec)-cameraToPointInTangentSpaceNorm);       
            float glossPower = pow(5000.0, inGlossiness); //textureColor0.a;
            float specularNorm = (glossPower + 2.0) / 8.0;
            float specularNormalized = specularNorm * pow(max (dot (halfVec, normal), 0.0), glossPower) * inSpecularity;
            float specular = specularNormalized  * fresnel;
            float3 resSpecularColor = lightColor0*specular;
            resColor+=resSpecularColor*reflectionColor;        
        #endif
        
        output.color = float4(resColor, 1.0);
    #else
        float3x3 tbnToWorldMatrix = float3x3(float3(input.tbnToWorld0), float3(input.tbnToWorld1), float3(input.tbnToWorld2));
        float3 reflectionVectorInTangentSpace = reflect(cameraToPointInTangentSpaceNorm, normal);
        reflectionVectorInTangentSpace.z = abs(reflectionVectorInTangentSpace.z); //prevent reflection through surface
        float3 reflectionVectorInWorldSpace = mul (reflectionVectorInTangentSpace, tbnToWorldMatrix);    
        float3 reflectionColor = texCUBE(cubemap, reflectionVectorInWorldSpace).rgb * reflectionTintColor;                    
        output.color = float4(reflectionColor, fresnel);
    #endif    

#elif SHADING == SHADING_PERVERTEX

    float3 reflectionColor = texCUBE(cubemap, input.reflectionDirectionInWorldSpace).rgb;
    float3 textureColorDecal = tex2D(decal, input.varTexCoordDecal).rgb;
    float3 textureColor0 = tex2D(albedo, varTexCoord0).rgb;
    float3 textureColor1 = tex2D(albedo, varTexCoord1).rgb;    
    output.color = float4(((textureColor0 * textureColor1) * 3.0 * decalTintColor * textureColorDecal + reflectionColor * reflectanceColor) , 1.0);

#endif    

#if VERTEX_FOG
    
    float   varFogAmoung = input.varFog.a;
    float3  varFogColor  = input.varFog.rgb;       
    output.color.rgb = lerp( output.color.rgb, varFogColor, varFogAmoung );
    
#endif

    return output;    
}

