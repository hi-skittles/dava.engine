#include "common.slh"
#include "blending.slh"

vertex_in
{
    float3 position : POSITION;
#if defined(DEBUG_2D)
    float2 texCoord : TEXCOORD0;
#endif
};

vertex_out
{
    float4 position : SV_POSITION;
    
#if defined(ENCODE_DISTANCE)

    float3 directionFromPoint  : TEXCOORD0;

#elif defined(DECODE_DISTANCE)

    float3 directionFromPoint  : TEXCOORD0;

#elif defined(REPROJECTION)

    float4 reprojectedCoords   : TEXCOORD0;
    float  distanceToOrigin    : TEXCOORD1;
    float4 viewportCoords      : TEXCOORD2;

#elif defined(DEBUG_2D)

    float2 texCoord            : TEXCOORD0;

#endif
};

[auto][a] property float4x4 worldViewProjMatrix;
[auto][a] property float4x4 worldMatrix;
[auto][a] property float4x4 viewProjMatrix;
[auto][a] property float3 cameraPosition;

#if defined(DECODE_DISTANCE)

[material][a] property float3 origin;

#elif defined(REPROJECTION)

[material][a] property float4x4 fixedFrameMatrix;
[material][a] property float3 origin;

#endif

vertex_out vp_main( vertex_in input )
{
    vertex_out output;

    float4 pos = float4(input.position.xyz, 1.0);
    float4 worldPosition = mul(pos, worldMatrix);

#if defined(ENCODE_DISTANCE)

    output.directionFromPoint = worldPosition.xyz - cameraPosition;

#elif defined(DECODE_DISTANCE)

    output.directionFromPoint = worldPosition.xyz - origin;

#elif defined(REPROJECTION)

    output.reprojectedCoords = mul(worldPosition, fixedFrameMatrix);
    output.distanceToOrigin = length(worldPosition.xyz - origin);

#elif defined(DEBUG_2D)

    output.texCoord = input.texCoord;

#endif

    output.position = mul(pos, worldViewProjMatrix);

#if defined(REPROJECTION)

    output.viewportCoords = output.position;

#endif

    return output;
}
