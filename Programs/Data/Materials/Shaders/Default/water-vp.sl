#include "common.slh"

#define SHADING_PERVERTEX 0
#define SHADING_PERPIXEL  1

//convert old defines to new style
#if PIXEL_LIT
#define SHADING SHADING_PERPIXEL
#endif
#if VERTEX_LIT
#define SHADING SHADING_PERVERTEX
#endif

vertex_in
{
    float3  position    : POSITION;    
    float3  normal      : NORMAL;
    float3  tangent     : TANGENT;
    
    float2  uv0         : TEXCOORD0;

    #if SHADING == SHADING_PERVERTEX 
    float2  uv1         : TEXCOORD1; // decal
    #endif    
};

vertex_out
{
    float4  position : SV_POSITION;
    
    #if (!DEBUG_UNITY_Z_NORMAL)||(SHADING == SHADING_PERVERTEX )
        float2  uv  : TEXCOORD0;
        float2  uv1 : TEXCOORD1;
    #endif
    #if SHADING == SHADING_PERPIXEL
        half3  cameraToPointInTangentSpace : TEXCOORD2;
        #if REAL_REFLECTION
            float3  eyeDist           : TEXCOORD3;
            float4  normalizedFragPos : TEXCOORD4;
            #if SPECULAR
                float3 varLightVec : TEXCOORD5;
            #endif
        #else
            half3   tbnToWorld0 : TEXCOORD3;
            half3   tbnToWorld1 : TEXCOORD4;
            half3   tbnToWorld2 : TEXCOORD5;
        #endif
    #endif

    #if SHADING == SHADING_PERVERTEX 
        float2 varTexCoordDecal : TEXCOORD2;
        float3 reflectionDirectionInWorldSpace : TEXCOORD3;
    #endif
    
    #if VERTEX_FOG
        float4  varFog : TEXCOORD6;
    #endif
    
};


#if VERTEX_FOG
    #if SHADING == SHADING_PERVERTEX
        [auto][instance] property float4x4 worldViewMatrix;
    #endif
    #if SHADING == SHADING_PERPIXEL
        [auto][instance] property float4x4 worldMatrix;
        [auto][instance] property float3   cameraPosition;
    #endif
    #if FOG_ATMOSPHERE
        #if SHADING == SHADING_PERVERTEX
            [auto][instance] property float4x4 worldViewInvTransposeMatrix;
        #endif
        #if (SHADING == SHADING_PERVERTEX) || !(REAL_REFLECTION && SPECULAR)
            [auto][instance] property float4   lightPosition0;
        #endif
    #endif
#endif


[auto][instance] property float4x4 worldViewProjMatrix;

#if SHADING == SHADING_PERPIXEL
[auto][instance] property float4x4 worldViewInvTransposeMatrix;
[auto][instance] property float4x4 worldViewMatrix;
    #if REAL_REFLECTION && SPECULAR
        [auto][instance] property float4 lightPosition0;
    #endif
#endif

[auto][instance] property float globalTime;

#if REAL_REFLECTION
[auto][instance] property float projectionFlip;
#endif

#if SHADING == SHADING_PERVERTEX 
[auto][instance] property float3 cameraPosition;
[auto][instance] property float4x4 worldMatrix;
[auto][instance] property float4x4 worldInvTransposeMatrix;
#endif

#if (!DEBUG_UNITY_Z_NORMAL)||(SHADING == SHADING_PERVERTEX )
    [material][instance] property float2 normal0ShiftPerSecond  = float2(0,0); 
    [material][instance] property float2 normal1ShiftPerSecond  = float2(0,0);
    [material][instance] property float normal0Scale            = 0;
    [material][instance] property float normal1Scale            = 0;
    #if DEBUG_NORMAL_ROTATION
        [material][instance] property float normalRotation = 0;
    #endif
#endif 

#include "vp-fog-props.slh"



vertex_out vp_main( vertex_in input )
{
    vertex_out  output;

//position
    float3 in_pos      = input.position.xyz;
    float4 inPosition  = float4(in_pos, 1.0);    
    float4 resPosition = mul( inPosition, worldViewProjMatrix );                
    
    output.position = resPosition;    
    
//texcoords
    #if (!DEBUG_UNITY_Z_NORMAL)||(SHADING == SHADING_PERVERTEX ) //prevent unused variables cut
        float2 inTexCoord0 = input.uv0;
        output.uv = inTexCoord0 * normal0Scale + frac(normal0ShiftPerSecond * globalTime);
        #if (SHADING == SHADING_PERPIXEL) && SEPARATE_NORMALMAPS
            #if DEBUG_NORMAL_ROTATION
                float rota = normalRotation/180.0*3.1415;        
                float cr = cos(rota);
                float sr = sin(rota);
                float2 rotatedTC = float2(inTexCoord0.x * cr + inTexCoord0.y * sr, inTexCoord0.x * sr - inTexCoord0.y * cr);
                output.uv1 = rotatedTC * normal1Scale + frac(normal1ShiftPerSecond * globalTime);
            #else            
                output.uv1 = inTexCoord0 * normal1Scale + frac(normal1ShiftPerSecond * globalTime);
            #endif
        #else
            output.uv1 = float2(inTexCoord0.x+inTexCoord0.y, inTexCoord0.y-inTexCoord0.x) * normal1Scale + frac(normal1ShiftPerSecond * globalTime);
        #endif
        
        #if (SHADING == SHADING_PERVERTEX)
            output.varTexCoordDecal = input.uv1;
        #endif
    #endif

    
//shading requirements
    float3  inNormal    = input.normal;
    
    #if (SHADING == SHADING_PERVERTEX)
        float3 world_position = mul( float4(input.position.xyz,1.0), worldMatrix ).xyz;
        float3 viewDirectionInWorldSpace = world_position - cameraPosition;
        float3 normalDirectionInWorldSpace = normalize(mul(float4(inNormal, 0), worldInvTransposeMatrix).xyz);
        output.reflectionDirectionInWorldSpace = reflect(viewDirectionInWorldSpace, normalDirectionInWorldSpace);
    #endif
    
    #if (SHADING == SHADING_PERPIXEL)
        float3 eyeCoordsPosition = mul (inPosition, worldViewMatrix ).xyz;       
        
        float3  inTangent   = input.tangent;
        
        float3 n = normalize( mul( float4(inNormal,1.0), worldViewInvTransposeMatrix ).xyz );
        float3 t = normalize( mul( float4(inTangent,1.0), worldViewInvTransposeMatrix ).xyz );
        float3 b = cross (n, t);

        float3 v;
        v.x = dot (eyeCoordsPosition, t);
        v.y = dot (eyeCoordsPosition, b);
        v.z = dot (eyeCoordsPosition, n);
        output.cameraToPointInTangentSpace = half3(v);                

        #if REAL_REFLECTION
            output.eyeDist = eyeCoordsPosition;
            output.normalizedFragPos = resPosition;                
            output.normalizedFragPos.y *= projectionFlip; 
            #if SPECULAR
                float3 toLightDir = lightPosition0.xyz - eyeCoordsPosition * lightPosition0.w;
                v.x = dot (toLightDir, t);
                v.y = dot (toLightDir, b);
                v.z = dot (toLightDir, n);
                output.varLightVec = v;       
            #endif
        #else
            output.tbnToWorld0 = half3(inTangent);
            output.tbnToWorld1 = half3(cross(inNormal, inTangent));
            output.tbnToWorld2 = half3(inNormal);            
        #endif       
    #endif

    #if VERTEX_FOG
    
        #if (SHADING == SHADING_PERVERTEX)
            float3 eyeCoordsPosition = mul( float4(input.position.xyz,1.0), worldViewMatrix ).xyz;
        #endif
    
        #define FOG_view_position eyeCoordsPosition
        
    #if FOG_ATMOSPHERE
        float3 tolight_dir = lightPosition0.xyz - eyeCoordsPosition * lightPosition0.w;
        #define FOG_to_light_dir tolight_dir
    #endif
        
    #if FOG_HALFSPACE || FOG_ATMOSPHERE_MAP
        #if (SHADING == SHADING_PERPIXEL)
            float3 world_position = mul( float4(input.position.xyz,1.0), worldMatrix ).xyz;
        #endif
        #define FOG_world_position world_position
    #endif

        #define FOG_eye_position cameraPosition
        #define FOG_in_position input.position

        #include "vp-fog-math.slh" // in{ float3 FOG_view_position, float3 FOG_eye_position, float3 FOG_to_light_dir, float3 FOG_world_position }; out{ float4 FOG_result };
        
        output.varFog = FOG_result;
        
    #endif

    return output; 
}
