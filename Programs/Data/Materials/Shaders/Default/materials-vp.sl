#include "common.slh"



////////////////////////////////////////////////////////////////////////////////
// vprog-input

vertex_in
{    
    float3 position : POSITION;
    
    #if VERTEX_LIT || PIXEL_LIT
    float3 normal : NORMAL;
    #endif

    #if MATERIAL_TEXTURE
        #if MATERIAL_SKYBOX
        float3 texcoord0 : TEXCOORD0;
        #else
        float2 texcoord0 : TEXCOORD0;
        #endif
    #endif

    #if MATERIAL_DECAL || ( MATERIAL_LIGHTMAP  && VIEW_DIFFUSE ) || ALPHA_MASK
    float2 texcoord1 : TEXCOORD1;
    #endif

    #if FRAME_BLEND
    float3 texcoord1 : TEXCOORD1; // uv1.xy + time
    #endif

    #if PARTICLES_FLOWMAP
        float4 texcoord2 : TEXCOORD2; // Flow speed and flow offset.
    #endif

    #if PARTICLES_NOISE
        float3 texcoord3 : TEXCOORD3; // Noise uv and scale.
    #endif

    #if VERTEX_COLOR
        float4 color0 : COLOR0;
    #endif
    
    #if PIXEL_LIT
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
    #endif
    
    #if SOFT_SKINNING
    float4 index  : BLENDINDICES;
    float4 weight : BLENDWEIGHT;
    #elif HARD_SKINNING
    float  index  : BLENDINDICES;
    #endif


    #if SPEED_TREE_OBJECT
    float4 pivot       : TEXCOORD4;
    #if WIND_ANIMATION
    float2 angleSinCos : TEXCOORD6;
    #endif
    #endif
    
    #if PARTICLES_FRESNEL_TO_ALPHA || PARTICLES_ALPHA_REMAP || PARTICLES_PERSPECTIVE_MAPPING
        float3 texcoord5 : TEXCOORD5;  // x - fresnel. y - alpha remap. z - perspective mapping w.
    #endif

    #if WIND_ANIMATION
    float flexibility : TEXCOORD5;
    #endif

    #if GEO_DECAL
    float4 geoDecalCoord : TEXCOORD3;
    #endif
};

////////////////////////////////////////////////////////////////////////////////
// vprog-output

vertex_out
{
    float4  position : SV_POSITION;

    #if MATERIAL_SKYBOX
    float3 varTexCoord0 : TEXCOORD0;
    #elif MATERIAL_TEXTURE || TILED_DECAL_MASK
        #if PARTICLES_PERSPECTIVE_MAPPING
            float3 varTexCoord0 : TEXCOORD0;
        #else
    float2 varTexCoord0 : TEXCOORD0;
    #endif
    #endif

    #if MATERIAL_DECAL || ( MATERIAL_LIGHTMAP  && VIEW_DIFFUSE ) || FRAME_BLEND || ALPHA_MASK
    float2 varTexCoord1 : TEXCOORD1;
    #endif

    #if MATERIAL_DETAIL
    float2 varDetailTexCoord : TEXCOORD2;
    #endif

    #if TILED_DECAL_MASK
    float2 varDecalTileTexCoord : TEXCOORD2;
    #endif
    
    #if PARTICLES_FLOWMAP
        float2 varParticleFlowTexCoord : TEXCOORD2;
    #endif
    
    #if VERTEX_LIT
        [lowp] half varDiffuseColor : COLOR0;
        #if BLINN_PHONG
            [lowp] half varSpecularColor : TEXCOORD4;
        #elif NORMALIZED_BLINN_PHONG
            [lowp] half4 varSpecularColor : TEXCOORD4;
        #endif
    #endif

    #if PIXEL_LIT
        #if FAST_NORMALIZATION
        [lowp] half3 varHalfVec : COLOR0;
        #endif
        [lowp] half3 varToLightVec : COLOR1;
        float3 varToCameraVec : TEXCOORD7;
    #endif    
    
    #if VERTEX_COLOR || SPHERICAL_LIT
        [lowp] half4 varVertexColor : COLOR1;
    #endif
    
    #if VERTEX_FOG
        [lowp] half4 varFog : TEXCOORD5;
    #endif

    #if PARTICLES_NOISE
        #if PARTICLES_FRESNEL_TO_ALPHA
            float4 varTexcoord6 : TEXCOORD6; // Noise uv and scale. Fresnel a.
        #else
            float3 varTexcoord6 : TEXCOORD6; // Noise uv and scale.
        #endif
    #elif PARTICLES_FRESNEL_TO_ALPHA
        float varTexcoord6 : TEXCOORD6; // Fresnel a.
    #endif 

    #if FRAME_BLEND && PARTICLES_ALPHA_REMAP
        half2 varTexcoord3 : TEXCOORD3;
    #elif FRAME_BLEND || PARTICLES_ALPHA_REMAP
        half varTexcoord3 : TEXCOORD3;
    #endif

    #if FLOWMAP || PARTICLES_FLOWMAP
        [lowp] float3 varFlowData : TEXCOORD4;
    #endif

    #if GEO_DECAL
        float2 geoDecalCoord : TEXCOORD6;
    #endif
};

////////////////////////////////////////////////////////////////////////////////
// properties

[auto][a] property float4x4 worldViewProjMatrix;

#if VERTEX_LIT || PIXEL_LIT || VERTEX_FOG || SPEED_TREE_OBJECT || SPHERICAL_LIT
[auto][a] property float4x4 worldViewMatrix;
#endif

#if VERTEX_LIT || PIXEL_LIT /*|| (VERTEX_FOG && FOG_ATMOSPHERE)*/
[auto][a] property float4x4 worldViewInvTransposeMatrix;
#if DISTANCE_ATTENUATION
[material][a] property float lightIntensity0 = 1.0; 
#endif
#endif
#if VERTEX_LIT || PIXEL_LIT || (VERTEX_FOG && FOG_ATMOSPHERE)
[auto][a] property float4 lightPosition0;
#endif

#if VERTEX_LIT
[material][a] property float materialSpecularShininess = 0.5;
[material][a] property float inSpecularity = 1.0;
[material][a] property float inGlossiness = 0.5;
[material][a] property float3 metalFresnelReflectance = float3(0.5,0.5,0.5);
#endif

#if SOFT_SKINNING || HARD_SKINNING
[auto][jpos] property float4 jointPositions[MAX_JOINTS] : "bigarray" ; // (x, y, z, scale)
[auto][jrot] property float4 jointQuaternions[MAX_JOINTS] : "bigarray";
#endif

#include "vp-fog-props.slh"

#if ( MATERIAL_LIGHTMAP  && VIEW_DIFFUSE ) && !SETUP_LIGHTMAP
[material][a] property float2 uvOffset = float2(0,0);
[material][a] property float2 uvScale = float2(0,0);
#endif

#if WIND_ANIMATION
[auto][a] property float2 trunkOscillationParams;
#endif

#if SPEED_TREE_OBJECT
[auto][a] property float3 worldScale;
[auto][a] property float4x4 projMatrix;

    #if CUT_LEAF
        [material][a] property float cutDistance = 1.0;
    #endif

    #if !SPHERICAL_LIT  //legacy for old tree lighting
        [material][a] property float4 treeLeafColorMul = float4(0.5,0.5,0.5,0.5) ;
        [material][a] property float treeLeafOcclusionOffset = 0.0 ;
        [material][a] property float treeLeafOcclusionMul = 0.5 ;
    #endif
    
    #if WIND_ANIMATION
        [auto][a] property float2 leafOscillationParams; //x: A*sin(T); y: A*cos(T);
    #endif
    
    #if SPHERICAL_LIT
        [auto][a] property float speedTreeLightSmoothing;
    #endif
#endif

#if SPHERICAL_LIT
[auto][a] property float3 worldViewObjectCenter;
[auto][a] property float4x4 invViewMatrix;
[auto][a] property float3 boundingBoxSize;

    #if SPHERICAL_HARMONICS_9
        [auto][sh] property float4 sphericalHarmonics[7] : "bigarray";
    #elif SPHERICAL_HARMONICS_4
        [auto][sh] property float4 sphericalHarmonics[3] : "bigarray";
    #else
        [auto][sh] property float4 sphericalHarmonics;
    #endif
#endif

#if TILED_DECAL_MASK
[material][a] property float2 decalTileCoordOffset = float2(0,0);
[material][a] property float2 decalTileCoordScale = float2(0,0);
#endif

#if MATERIAL_DETAIL
[material][a] property float2 detailTileCoordScale = float2(1.0,1.0);
#endif

#if TEXTURE0_SHIFT_ENABLED
[material][a] property float2 texture0Shift = float2(0,0);
#endif 
#if TEXTURE0_ANIMATION_SHIFT
[material][a] property float2 tex0ShiftPerSecond = float2(0,0);
#endif

#if VERTEX_FOG 
[auto][a] property float3 cameraPosition;
[auto][a] property float4x4 worldMatrix;
#endif

#if WAVE_ANIMATION || TEXTURE0_ANIMATION_SHIFT || FLOWMAP || PARTICLES_FLOWMAP
[auto][a] property float globalTime;
#endif

#if FLOWMAP
[material][a] property float flowAnimSpeed = 0;
[material][a] property float flowAnimOffset = 0;
#endif

inline float FresnelShlick( float NdotL, float Cspec )
{
    float fresnel_exponent = 5.0;
    return (1.0 - Cspec) * pow(1.0 - NdotL, fresnel_exponent) + Cspec;
}

inline float3 FresnelShlickVec3( float NdotL, float3 Cspec )
{
    float fresnel_exponent = 5.0;
    return (1.0 - Cspec) * (pow(1.0 - NdotL, fresnel_exponent)) + Cspec;
}

#if SOFT_SKINNING

inline float3 JointTransformTangent( float3 tangent, float4 quaternion, float jWeight)
{
    float3 tmp = 2.0 * cross(quaternion.xyz, tangent);
    return tangent + (quaternion.w * tmp + cross(quaternion.xyz, tmp)) * jWeight;
}

#elif HARD_SKINNING

inline float3 JointTransformTangent( float3 tangent, float4 quaternion )
{
    float3 tmp = 2.0 * cross(quaternion.xyz, tangent);
    return tangent + quaternion.w * tmp + cross(quaternion.xyz, tmp);
}

#endif

inline float4 Wave( float time, float4 pos, float2 uv )
{
//  float time = globalTime;
//  vec4 pos = inPosition;
//  vec2 uv = inTexCoord0;
#if 1
    float4 off;
    float sinOff = pos.x + pos.y + pos.z;
    float t = -time * 3.0;
    float cos1 = cos(t * 1.45 + sinOff);
    float cos2 = cos(t * 3.12 + sinOff);
    float cos3 = cos(t * 2.2 + sinOff);
    float fx= uv.x;
    float fy= uv.x * uv.y;
    
    off.y = pos.y + cos2 * fx * 0.5 - fy * 0.9;
    off.x = pos.x + cos1 * fx * 0.5;
    off.z = pos.z + cos3 * fx * 0.5;
    off.w = pos.w;
#else
    float4 off;
    float t = -time;
    float sin2 = sin(4.0 * sqrt(uv.x + uv.x + uv.y * uv.y) + time);
    
    off.x = pos.x;// + cos1 * fx * 0.5;
    off.y = pos.y + sin2 * 0.5;// - fy * 0.9;
    off.z = pos.z;// + cos3 * fx * 0.5;
    off.w = pos.w;
#endif
    return off;
}

////////////////////////////////////////////////////////////////////////////////
// main

vertex_out vp_main( vertex_in input )
{
    vertex_out  output;

#if FLOWMAP || PARTICLES_FLOWMAP
#if FLOWMAP
        float flowSpeed = flowAnimSpeed;
        float flowOffset = flowAnimOffset;
    #else
        float flowSpeed = input.texcoord2.z;
        float flowOffset = input.texcoord2.w;
        output.varParticleFlowTexCoord = input.texcoord2.xy;
    #endif

    float scaledTime = globalTime * flowSpeed;
    float2 flowPhases = frac(float2(scaledTime, scaledTime+0.5))-float2(0.5, 0.5);
    float flowBlend = abs(flowPhases.x*2.0);
    output.varFlowData = float3(flowPhases * flowOffset, flowBlend);
#endif

#if PARTICLES_NOISE
    output.varTexcoord6.xyz = input.texcoord3.xyz;
#endif

#if MATERIAL_SKYBOX
    
    float4 vecPos = mul( input.position.xyz, worldViewProjMatrix );
    output.position = float4(vecPos.xy, vecPos.w - 0.0001, vecPos.w);

#elif SKYOBJECT
    
    float4x4 mwpWOtranslate = float4x4(worldViewProjMatrix[0], worldViewProjMatrix[1], worldViewProjMatrix[2], float4(0.0, 0.0, 0.0, 1.0));
    float4   vecPos         = mul( float4(input.position.xyz,1.0), mwpWOtranslate );
    output.position = float4(vecPos.x, vecPos.y, vecPos.w - 0.0001, vecPos.w);

#elif SPEED_TREE_OBJECT

    float3 position = lerp(input.position.xyz, input.pivot.xyz, input.pivot.w);
    float3 billboardOffset = input.position.xyz - position.xyz;
    
    #if CUT_LEAF
	    float pivotDistance = dot(position.xyz, float3(worldViewMatrix[0].z, worldViewMatrix[1].z, worldViewMatrix[2].z)) + worldViewMatrix[3].z;
        billboardOffset *= step(-cutDistance, pivotDistance);
    #endif
	
    #if WIND_ANIMATION
    
        //inAngleSinCos:          x: cos(T0);  y: sin(T0);
        //leafOscillationParams:  x: A*sin(T); y: A*cos(T);
        float3 windVectorFlex = float3(trunkOscillationParams * input.flexibility, 0.0);
        position += windVectorFlex;
        
        float2 SinCos = input.angleSinCos * leafOscillationParams; //vec2(A*sin(t)*cos(t0), A*cos(t)*sin(t0))
        float sinT = SinCos.x + SinCos.y;     //sin(t+t0)*A = sin*cos + cos*sin
        float cosT = 1.0 - 0.5 * sinT * sinT; //cos(t+t0)*A = 1 - 0.5*sin^2
        
        float4 SinCosT = float4(sinT, cosT, cosT, sinT); //temp vec for mul
        float4 offsetXY = float4(billboardOffset.x, billboardOffset.y, billboardOffset.x, billboardOffset.y); //temp vec for mul
        float4 rotatedOffsetXY = offsetXY * SinCosT; //vec4(x*sin, y*cos, x*cos, y*sin)
        
        billboardOffset.x = rotatedOffsetXY.z - rotatedOffsetXY.w; //x*cos - y*sin
        billboardOffset.y = rotatedOffsetXY.x + rotatedOffsetXY.y; //x*sin + y*cos

    #endif
    
    float4 eyeCoordsPosition4 = mul( float4(position, 1.0), worldViewMatrix ) + float4(worldScale * billboardOffset, 0.0);

    output.position = mul(eyeCoordsPosition4, projMatrix);

#else

    #if WIND_ANIMATION

        float3 windVectorFlex = float3(trunkOscillationParams * input.flexibility, 0.0);
        output.position = mul( float4(input.position.xyz + windVectorFlex, 1.0), worldViewProjMatrix );
        
    #else // WIND_ANIMATION

        #if WAVE_ANIMATION
            float4 waveValue = Wave(globalTime, float4(input.position.xyz, 1.0), input.texcoord0);
            output.position = mul( waveValue, worldViewProjMatrix );
        #else
            #if SOFT_SKINNING || HARD_SKINNING
            
                float4 skinnedPosition = float4(0.0, 0.0, 0.0, 0.0);
                
                #if SOFT_SKINNING
                {
                    float4 indices = input.index;
                    float4 weights = input.weight;
                    for(int i = 0; i < SOFT_SKINNING; ++i)
                    {
                        int jIndex = int(indices.x);
                        
                        float4 jP = jointPositions[jIndex];
                        float4 jQ = jointQuaternions[jIndex];
                    
                        float3 tmp = 2.0 * cross(jQ.xyz, input.position.xyz);
                        skinnedPosition += float4(jP.xyz + (input.position.xyz + jQ.w * tmp + cross(jQ.xyz, tmp)) * jP.w, 1.0) * weights.x;
                        
                        indices = indices.yzwx;
                        weights = weights.yzwx;
                    }
                }
                #else 
                {
                    int jIndex = int(input.index);
                    
                    float4 jP = jointPositions[jIndex];
                    float4 jQ = jointQuaternions[jIndex];

                    float3 tmp = 2.0 * cross(jQ.xyz, input.position.xyz);
                    skinnedPosition = float4(jP.xyz + (input.position.xyz + jQ.w * tmp + cross(jQ.xyz, tmp)) * jP.w, 1.0);
                }
                #endif
                    
                output.position = mul( skinnedPosition, worldViewProjMatrix );
                    
            #else
                output.position = mul( float4(input.position.xyz,1.0), worldViewProjMatrix );
            #endif
        #endif

    #endif // WIND_ANIMATION

#endif


#if SPEED_TREE_OBJECT
    float3 eyeCoordsPosition = eyeCoordsPosition4.xyz;
#elif VERTEX_LIT || PIXEL_LIT || VERTEX_FOG || SPHERICAL_LIT
    #if SOFT_SKINNING || HARD_SKINNING
        float3 eyeCoordsPosition = mul( skinnedPosition, worldViewMatrix ).xyz; // view direction in view space
    #else
        // view direction in view space
        float3 eyeCoordsPosition = mul( float4(input.position.xyz,1.0), worldViewMatrix ).xyz;
    #endif
#endif


#if VERTEX_LIT || PIXEL_LIT || (VERTEX_FOG && FOG_ATMOSPHERE)
    float3 toLightDir = lightPosition0.xyz - eyeCoordsPosition * lightPosition0.w;
#endif


#if VERTEX_LIT
    
//-    float3  inNormal = input.normal;
    
    float3 normal = normalize(mul(float4(input.normal, 0.0), worldViewInvTransposeMatrix).xyz); // normal in eye coordinates
   
    #if DISTANCE_ATTENUATION
        float attenuation = lightIntensity0;
        float distAttenuation = length(toLightDir);
        attenuation /= (distAttenuation * distAttenuation); // use inverse distance for distance attenuation
    #endif
    
    toLightDir = normalize(toLightDir);
    
    #if BLINN_PHONG
        
        output.varDiffuseColor = max(0.0, dot(normal, toLightDir));

        // Blinn-phong reflection
        float3 toCameraDir = normalize(-eyeCoordsPosition);
        float3 H = normalize(toLightDir + toCameraDir);
        float nDotHV = max(0.0, dot(normal, H));
        output.varSpecularColor = pow(nDotHV, materialSpecularShininess);
        
    #elif NORMALIZED_BLINN_PHONG
        
        float3 toCameraNormalized = normalize(-eyeCoordsPosition);
        float3 H = normalize(toLightDir + toCameraNormalized);

        float NdotL = max (dot (normal, toLightDir), 0.0);
        float NdotH = max (dot (normal, H), 0.0);
        float LdotH = max (dot (toLightDir, H), 0.0);
        float NdotV = max (dot (normal, toCameraNormalized), 0.0);

        //float3 fresnelIn = FresnelShlickVec3(NdotL, metalFresnelReflectance);
        float3 fresnelOut  = FresnelShlickVec3(NdotV, metalFresnelReflectance);
        float  specularity = inSpecularity;

        float Dbp = NdotL;
        float Geo = 1.0 / LdotH * LdotH;
        
        output.varDiffuseColor = NdotL / _PI;
        
        output.varSpecularColor.xyz = half3(Dbp * Geo * fresnelOut * specularity);
        output.varSpecularColor.w = half(NdotH);
    
    #endif
    
#endif // VERTEX_LIT

#if PARTICLES_FRESNEL_TO_ALPHA
    #if PARTICLES_NOISE
        output.varTexcoord6.w = input.texcoord5.x;
    #else
        output.varTexcoord6 = input.texcoord5.x;
    #endif 
#endif

#if PIXEL_LIT

    float3  inNormal    = input.normal;
    float3  inTangent   = input.tangent;
    float3  inBinormal  = input.binormal;
    
    #if SOFT_SKINNING

        float3 n = inNormal;
        float3 t = inTangent;
        float3 b = inBinormal;

        float4 indices = input.index;
        float4 weights = input.weight;
        for(int i = 0; i < SOFT_SKINNING; ++i)
        {
            float4 jointQuaternion = jointQuaternions[int(indices.x)];

            n = JointTransformTangent(n, jointQuaternion, weights.x);
            t = JointTransformTangent(t, jointQuaternion, weights.x);
            b = JointTransformTangent(b, jointQuaternion, weights.x);

            indices = indices.yzwx;
            weights = weights.yzwx;
        }

        n = normalize( mul( float4(n, 1.0), worldViewInvTransposeMatrix ).xyz );
        t = normalize( mul( float4(t, 1.0), worldViewInvTransposeMatrix ).xyz );
        b = normalize( mul( float4(b, 1.0), worldViewInvTransposeMatrix ).xyz );

    #elif HARD_SKINNING

        float4 jointQuaternion = jointQuaternions[int(input.index)];
        float3 n = normalize( mul( float4(JointTransformTangent(inNormal, jointQuaternion), 1.0), worldViewInvTransposeMatrix ).xyz );
        float3 t = normalize( mul( float4(JointTransformTangent(inTangent, jointQuaternion), 1.0), worldViewInvTransposeMatrix ).xyz );
        float3 b = normalize( mul( float4(JointTransformTangent(inBinormal, jointQuaternion), 1.0), worldViewInvTransposeMatrix ).xyz );

    #else

        float3 n = normalize( mul( float4(inNormal,1.0), worldViewInvTransposeMatrix ).xyz );
        float3 t = normalize( mul( float4(inTangent,1.0), worldViewInvTransposeMatrix ).xyz );
        float3 b = normalize( mul( float4(inBinormal,1.0), worldViewInvTransposeMatrix ).xyz );

    #endif       
    
    // transform light and half angle vectors by tangent basis
    float3 v;
    v.x = dot (toLightDir, t);
    v.y = dot (toLightDir, b);
    v.z = dot (toLightDir, n);
    
    #if !FAST_NORMALIZATION
        output.varToLightVec = half3(v);
    #else
        output.varToLightVec = half3(normalize(v));
    #endif

    float3 toCameraDir = -eyeCoordsPosition;

    v.x = dot (toCameraDir, t);
    v.y = dot (toCameraDir, b);
    v.z = dot (toCameraDir, n);
    
    #if !FAST_NORMALIZATION
        output.varToCameraVec = float3(v);
    #else
        output.varToCameraVec = float3(normalize(v));
    #endif
    
    /* Normalize the halfVector to pass it to the fragment shader */
    // No need to divide by two, the result is normalized anyway.
    // float3 halfVector = normalize((E + lightDir) / 2.0);
    #if FAST_NORMALIZATION
        float3 halfVector = normalize(normalize(toCameraDir) + normalize(toLightDir));
        v.x = dot (halfVector, t);
        v.y = dot (halfVector, b);
        v.z = dot (halfVector, n);
        
        // No need to normalize, t,b,n and halfVector are normal vectors.
        output.varHalfVec = half3(v);
    #endif

//    varLightPosition.x = dot (lightPosition0.xyz, t);
//    varLightPosition.y = dot (lightPosition0.xyz, b);
//    varLightPosition.z = dot (lightPosition0.xyz, n);
    
#endif // PIXEL_LIT

#if VERTEX_FOG
    
    #define FOG_eye_position cameraPosition
    #define FOG_view_position eyeCoordsPosition
    #define FOG_in_position input.position
        
#if FOG_ATMOSPHERE
    #define FOG_to_light_dir toLightDir
#endif
    
#if FOG_HALFSPACE || FOG_ATMOSPHERE_MAP
    float3 world_position = mul( float4(input.position.xyz,1.0), worldMatrix ).xyz;
    #define FOG_world_position world_position
#endif
    
    #include "vp-fog-math.slh" // in{float3 FOG_view_position; float3 FOG_eye_position; float3 FOG_to_light_dir; float3 FOG_world_position; } ; out{ float4 FOG_result }
    
    output.varFog = half4(FOG_result);

#endif

#if VERTEX_COLOR
    output.varVertexColor = half4(input.color0);
#endif


#if SPHERICAL_LIT

//    #define A0      (0.282094)
//    #define A1      (0.325734)

//    #define Y2_2(n) (0.273136 * (n.y * n.x))                                // (1.0 / 2.0) * sqrt(15.0 / PI) * ((n.y * n.x)) * 0.785398 / PI
//    #define Y2_1(n) (0.273136 * (n.y * n.z))                                // (1.0 / 2.0) * sqrt(15.0 / PI) * ((n.y * n.z)) * 0.785398 / PI
//    #define Y20(n)  (0.078847 * (3.0 * n.z * n.z - 1.0))                    // (1.0 / 4.0) * sqrt(5.0 / PI) * ((3.0 * n.z * n.z - 1.0)) * 0.785398 / PI
//    #define Y21(n)  (0.273136 * (n.z * n.x))                                // (1.0 / 2.0) * sqrt(15.0 / PI) * ((n.z * n.x)) * 0.785398 / PI
//    #define Y22(n)  (0.136568 * (n.x * n.x - n.y * n.y))                    // (1.0 / 4.0) * sqrt(15.0 / PI) * ((n.x * n.x - n.y * n.y)) * 0.785398 / PI

    #if SPHERICAL_HARMONICS_4 || SPHERICAL_HARMONICS_9
        float3 sphericalLightFactor = 0.282094 * sphericalHarmonics[0].xyz;
    #else
        float3 sphericalLightFactor = 0.282094 * sphericalHarmonics.xyz;
    #endif
    
    #if SPEED_TREE_OBJECT
        float3 localSphericalLightFactor = sphericalLightFactor;
    #endif
    
    #if !CUT_LEAF

        #if SPHERICAL_HARMONICS_4 || SPHERICAL_HARMONICS_9

            float3x3 invViewMatrix3 = float3x3(float3(invViewMatrix[0].xyz), float3(invViewMatrix[1].xyz), float3(invViewMatrix[2].xyz));
            float3 normal = mul((eyeCoordsPosition - worldViewObjectCenter), invViewMatrix3);
            normal /= boundingBoxSize;
            float3 n = normalize(normal);

            float3x3 shMatrix = float3x3(float3(sphericalHarmonics[0].w,  sphericalHarmonics[1].xy),
                                         float3(sphericalHarmonics[1].zw, sphericalHarmonics[2].x),
                                         float3(sphericalHarmonics[2].yzw));
            sphericalLightFactor += 0.325734 * mul(float3(n.y, n.z, n.x), shMatrix);
        
            #if SPEED_TREE_OBJECT
                float3 localNormal = mul( (worldScale * billboardOffset), invViewMatrix3 );
                localNormal.z += 1.0 - input.pivot.w; //in case regular geometry (not billboard) we have zero 'localNoraml', so add something to correct 'normalize'
                float3 ln = normalize(localNormal);
                localSphericalLightFactor += (0.325734 * mul(float3(ln.y, ln.z, ln.x), shMatrix)) * input.pivot.w;
            #endif

            #if SPHERICAL_HARMONICS_9
                sphericalLightFactor += (0.273136 * (n.y * n.x)) * float3(sphericalHarmonics[3].xyz);                
                sphericalLightFactor += (0.273136 * (n.y * n.z)) * float3(sphericalHarmonics[3].w,  sphericalHarmonics[4].xy);                
                sphericalLightFactor += (0.078847 * (3.0 * n.z * n.z - 1.0)) * float3(sphericalHarmonics[4].zw, sphericalHarmonics[5].x);
                sphericalLightFactor += (0.273136 * (n.z * n.x))  * float3(sphericalHarmonics[5].yzw);
                sphericalLightFactor += (0.136568 * (n.x * n.x - n.y * n.y)) * float3(sphericalHarmonics[6].xyz);
            #endif

            #if SPEED_TREE_OBJECT
                sphericalLightFactor = lerp(sphericalLightFactor, localSphericalLightFactor, speedTreeLightSmoothing);
            #endif
        
        #endif // SPHERICAL_HARMONICS_4 || SPHERICAL_HARMONICS_9

    #endif // !CUT_LEAF

    #if VERTEX_COLOR
        output.varVertexColor.xyz = half3(input.color0.xyz) * half3(sphericalLightFactor * 2.0);
    #else
        output.varVertexColor.xyz = half3(sphericalLightFactor * 2.0);
    #endif

    output.varVertexColor.w = half(1.0);    

#elif SPEED_TREE_OBJECT //legacy for old tree lighting
    
    output.varVertexColor.xyz = half3(input.color0.xyz * treeLeafColorMul.xyz * treeLeafOcclusionMul + float3(treeLeafOcclusionOffset,treeLeafOcclusionOffset,treeLeafOcclusionOffset));

#endif

#if MATERIAL_SKYBOX || MATERIAL_TEXTURE || TILED_DECAL_MASK
    output.varTexCoord0.xy = input.texcoord0;
    #if PARTICLES_PERSPECTIVE_MAPPING
        output.varTexCoord0.z = input.texcoord5.z;
    #endif
#endif    

#if MATERIAL_TEXTURE
    #if TEXTURE0_SHIFT_ENABLED
        output.varTexCoord0.xy += texture0Shift;
    #endif

    #if TEXTURE0_ANIMATION_SHIFT
        output.varTexCoord0.xy += frac(tex0ShiftPerSecond * globalTime);
    #endif
#endif

#if TILED_DECAL_MASK
    float2 resDecalTexCoord = output.varTexCoord0.xy * decalTileCoordScale + decalTileCoordOffset;    
    #if TILE_DECAL_ROTATION
        resDecalTexCoord = float2(resDecalTexCoord.x+resDecalTexCoord.y, resDecalTexCoord.y-resDecalTexCoord.x);
    #endif
    output.varDecalTileTexCoord = resDecalTexCoord;
#endif
    
#if MATERIAL_DETAIL
    output.varDetailTexCoord = output.varTexCoord0.xy * detailTileCoordScale;
#endif

#if MATERIAL_DECAL || ( MATERIAL_LIGHTMAP  && VIEW_DIFFUSE ) || FRAME_BLEND || ALPHA_MASK
    #if ( MATERIAL_LIGHTMAP && VIEW_DIFFUSE && !SETUP_LIGHTMAP )
        output.varTexCoord1 = uvScale*input.texcoord1.xy + uvOffset;
    #else
        output.varTexCoord1 = input.texcoord1.xy;
    #endif
#endif

#if FRAME_BLEND
    #if PARTICLES_ALPHA_REMAP
        output.varTexcoord3.x = input.texcoord1.z;
        output.varTexcoord3.y = input.texcoord5.y;
    #else
        output.varTexcoord3 = input.texcoord1.z;
    #endif
#elif PARTICLES_ALPHA_REMAP
    output.varTexcoord3 = input.texcoord5.y;
#endif

#if FORCE_2D_MODE
    output.position.z=0.0;
#endif

#if (GEO_DECAL)
    // apply constant bias to prevent z-fighting on decals
    // possible improvement : calculate offset based on near/far plane
    // todo : check on various GPUs
    output.position.z -= output.position.w / 65535.0;
    output.geoDecalCoord = input.geoDecalCoord.xy;
#endif

    return output;
}
