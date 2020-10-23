#include "common.slh"

#if LANDSCAPE_USE_INSTANCING

    vertex_in
    {
        [vertex]    float4 data0 : TEXCOORD0; // position + edgeShiftDirection
        [vertex]    float4 data1 : TEXCOORD1; // edge mask
        [vertex]    float2 data2 : TEXCOORD2; // edgeVertexIndex + edgeMaskNull

        [instance]  float3 data3 : TEXCOORD3; // patch position + scale
        [instance]  float4 data4 : TEXCOORD4; // neighbour patch lodOffset
        #if LANDSCAPE_LOD_MORPHING
        [instance]  float4 data5 : TEXCOORD5; // neighbour patch morph
        [instance]  float3 data6 : TEXCOORD6; // patch lod + morph + pixelMappingOffset
        #endif
    };
    
#else

    vertex_in
    {    
        float4  pos     : POSITION;
        float2  uv      : TEXCOORD0;
        #if LANDSCAPE_SPECULAR
        float3  normal  : NORMAL;
        float3  tangent : TANGENT;
        #endif
    };
    
#endif

vertex_out
{
    float4  pos             : SV_POSITION;    
    float2  texCoord        : TEXCOORD0;

    #if TILEMASK
    float2  texCoordTiled   : TEXCOORD1;
    #endif

    #if LANDSCAPE_SPECULAR
    float3  varHalfVec      : TEXCOORD2;
    float3  varToLightVec   : TEXCOORD3;
    float3  varToCameraVec  : TEXCOORD4;
    #endif
    
    #if VERTEX_FOG
    float4  varFog          : TEXCOORD5;
    #endif

    #if LANDSCAPE_MORPHING_COLOR
    float4  morphColor      : COLOR0;
    #endif    
};


#if LANDSCAPE_USE_INSTANCING
    uniform sampler2D heightmap;
#endif

#if TILEMASK
    [material][instance] property float2 textureTiling = float2(50,50);
#endif

[auto][a] property float4x4 worldViewProjMatrix;

#if LANDSCAPE_USE_INSTANCING
    [auto][a] property float3 boundingBoxSize;
    [auto][a] property float heightmapTextureSize;
    
#if LANDSCAPE_SPECULAR
    uniform sampler2D tangentSpace; 
#endif

#endif

#if VERTEX_FOG
[auto][a] property float4x4 worldMatrix;
[auto][a] property float3   cameraPosition;
#endif
#if VERTEX_FOG || LANDSCAPE_SPECULAR
[auto][a] property float4x4 worldViewMatrix;
#endif
#if (VERTEX_FOG && FOG_ATMOSPHERE) || LANDSCAPE_SPECULAR
[auto][a] property float4   lightPosition0;
#endif

#if LANDSCAPE_SPECULAR
    [auto][a] property float4x4 worldViewInvTransposeMatrix;
    
    [material][instance] property float inSpecularity             = 1.0;
    [material][instance] property float inGlossiness              = 0.5;
    [material][instance] property float3 metalFresnelReflectance  = float3(0.5,0.5,0.5);
#endif

#include "vp-fog-props.slh"

vertex_out vp_main( vertex_in input )
{
    vertex_out  output;

#if LANDSCAPE_USE_INSTANCING

    float2 in_pos = input.data0.xy;
    float2 edgeShiftDirection = input.data0.zw;
    float4 edgeMask = input.data1;
    float edgeVertexIndex = input.data2.x;
    
    float3 patchOffsetScale = input.data3.xyz;
    float4 neighbourPatchLodOffset = input.data4;
    
    //Calculate vertecies offset for fusing neighboring patches
    float lodOffset = dot(edgeMask, neighbourPatchLodOffset);
    float edgeShiftAmount = pow(2.0, lodOffset);
    in_pos += edgeShiftDirection * fmod(edgeVertexIndex, edgeShiftAmount);
    
    float2 relativePosition = patchOffsetScale.xy + in_pos.xy * patchOffsetScale.z; //[0.0, 1.0]
    
#if LANDSCAPE_LOD_MORPHING
    
    float edgeMaskNull = input.data2.y; //if all components of edgeMask is zero - this value is 0.0, othewise - 1.0. Used for a little optimization.
    float4 neighbourPatchMorph = input.data5;
    
    float baseLod = input.data6.x;
    float patchMorph = input.data6.y;
    float basePixelOffset = input.data6.z;
    
    //Calculate 'zero-multiplier' that provide fetch zero-mip for vertecies at the edges with climbs beyound height-texture. 
    float2 zeroLod = step(1.0, relativePosition);
    float zeroLodMul = 1.0 - min(1.0, zeroLod.x + zeroLod.y);

    //Calculate fetch parameters
    float sampleLod = (baseLod + lodOffset) * zeroLodMul;
    float samplePixelOffset = basePixelOffset * edgeShiftAmount * zeroLodMul; //mul by 'edgeShiftAmount' give 0.5 / ( 2 ^ ( baseLod + lodOffset ) )
    float4 heightmapSample = tex2Dlod(heightmap, float2(relativePosition + samplePixelOffset), sampleLod);
    
    //Calculate morphed height. 
    float morphAmount = dot(edgeMask, neighbourPatchMorph) + patchMorph * edgeMaskNull;
    // float h0 = dot(heightmapSample.xy, float2(0.0038910506, 0.99610895)); // 'accurate' height
    // float h1 = dot(heightmapSample.zw, float2(0.0038910506, 0.99610895)); // 'averaged' height
    // float height = lerp(h1, h0, morphAmount);
    
    // This code make the same thing as the code above, but potentially using fewer multiplications
    float2 hmSampleMorphed = lerp(heightmapSample.zw, heightmapSample.xy, morphAmount);
    float height = dot(hmSampleMorphed, float2(0.0038910506, 0.99610895));
    
    #if LANDSCAPE_MORPHING_COLOR
        output.morphColor = float4(1.0 - morphAmount, morphAmount, 1.0, 1.0);
    #endif

#else
    
    #if HEIGHTMAP_FLOAT_TEXTURE
        float height = tex2Dlod(heightmap, float2(relativePosition + 0.5 / heightmapTextureSize), 0.0).r;
    #else	
        float4 heightmapSample = tex2Dlod(heightmap, float2(relativePosition + 0.5 / heightmapTextureSize), 0.0);
        float height = dot(heightmapSample, float4(0.00022888532845, 0.00366216525521, 0.05859464408331, 0.93751430533303));
    #endif
    
#endif

    float3 vx_position = float3( relativePosition - 0.5, height ) * boundingBoxSize;
    
    output.pos = mul( float4(vx_position.x, vx_position.y, vx_position.z, 1.0), worldViewProjMatrix );
    output.texCoord = float2(relativePosition.x, 1.0 - relativePosition.y);
     
#if LANDSCAPE_SPECULAR
    float4 tangentBasisSample = tex2Dlod(tangentSpace, float2(relativePosition + 0.5 / heightmapTextureSize), 0.0) * 2.0 - 1.0;
    float3 inNormal;
    float3 inTangent;
    inNormal.xy = tangentBasisSample.rg;
    inNormal.z = sqrt(1.0 - inNormal.x * inNormal.x - inNormal.y * inNormal.y);
    inTangent.yz = tangentBasisSample.ba;
    inTangent.x = sqrt(1.0 - inTangent.y * inTangent.y - inTangent.z * inTangent.z);
    
#endif
    
#else
    
    float3 vx_position = input.pos.xyz;
    
    output.pos = mul( float4(vx_position.x, vx_position.y, vx_position.z, 1.0), worldViewProjMatrix );
    output.texCoord = input.uv;
    
#if LANDSCAPE_SPECULAR
    float3 inNormal = input.normal;
    float3 inTangent = input.tangent;
#endif
    
#endif
    
#if LANDSCAPE_SPECULAR

    float3 n = mul(float4(inNormal, 0.0), worldViewInvTransposeMatrix).xyz;
    float3 t = mul(float4(inTangent, 0.0), worldViewInvTransposeMatrix).xyz;
    float3 b = cross(n, t);
    
    float3 eyeCoordsPosition = mul( float4(vx_position.x, vx_position.y, vx_position.z, 1.0), worldViewMatrix ).xyz;
    float3 toLightDir = lightPosition0.xyz - eyeCoordsPosition * lightPosition0.w;
    toLightDir = normalize(toLightDir);

    float3 v;
    v.x = dot (toLightDir, t);
    v.y = dot (toLightDir, b);
    v.z = dot (toLightDir, n);
    
    output.varToLightVec = normalize(v);
    
    float3 toCameraDir = -eyeCoordsPosition;
    v.x = dot (toCameraDir, t);
    v.y = dot (toCameraDir, b);
    v.z = dot (toCameraDir, n);
    
    output.varToCameraVec = normalize(v);
    
    float3 halfVector = normalize(normalize(toCameraDir) + normalize(toLightDir));
    v.x = dot (halfVector, t);
    v.y = dot (halfVector, b);
    v.z = dot (halfVector, n);
        
    // No need to normalize, t,b,n and halfVector are normal vectors.
    output.varHalfVec = v;

#endif
    
#if TILEMASK
    output.texCoordTiled = output.texCoord * textureTiling.xy;
#endif
    
#if VERTEX_FOG
    
    float3 view_position = mul( float4(vx_position.xyz,1.0), worldViewMatrix ).xyz;
    #define FOG_view_position view_position
    
#if FOG_ATMOSPHERE
    float3 tolight_dir = lightPosition0.xyz - view_position * lightPosition0.w;
    #define FOG_to_light_dir tolight_dir
#endif
    
#if FOG_HALFSPACE || FOG_ATMOSPHERE_MAP
    float3 world_position = mul( float4(vx_position.xyz,1.0), worldMatrix ).xyz;
    #define FOG_world_position world_position
#endif

    #define FOG_eye_position cameraPosition
    #define FOG_in_position input.pos

    #include "vp-fog-math.slh" // in{ float3 FOG_view_position, float3 FOG_eye_position, float3 FOG_to_light_dir, float3 FOG_world_position }; out{ float4 FOG_result };
    
    output.varFog = FOG_result;
    
#endif

    return output;    
}
