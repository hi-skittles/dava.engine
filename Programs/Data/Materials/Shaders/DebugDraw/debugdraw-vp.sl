#include "common.slh"

vertex_in
{
    float3  position : POSITION;
};

vertex_out
{
    float4  position : SV_POSITION;
};

[auto][a] property float4x4 worldViewProjMatrix;


vertex_out vp_main( vertex_in input )
{
    vertex_out  output;
    
    output.position = mul( float4(input.position.xyz,1.0), worldViewProjMatrix );

    return output;
}
