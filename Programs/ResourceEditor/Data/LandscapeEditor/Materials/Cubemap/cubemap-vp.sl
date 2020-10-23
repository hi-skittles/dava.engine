#include "common.slh"

vertex_in
{
    float3  position : POSITION;
    float3  normal   : NORMAL;
};

vertex_out
{
    float4  position : SV_POSITION;
    float3  normal   : TEXCOORD0;
};

[auto][a] property float4x4 worldViewProjMatrix;
[auto][a] property float4x4 worldMatrix;

vertex_out vp_main( vertex_in input )
{
    vertex_out  output;

    output.normal   = normalize( mul(float4(input.normal,0.0), worldMatrix).xyz );
    output.position = mul( float4(input.position.xyz,1.0), worldViewProjMatrix );

    return output;
}