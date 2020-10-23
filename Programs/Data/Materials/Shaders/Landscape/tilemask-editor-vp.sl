#include "common.slh"

vertex_in
{
    float3  pos      : POSITION;
    float2  uv       : TEXCOORD0;
};

vertex_out
{
    float4  position : SV_POSITION;
    float2  uv       : TEXCOORD0;
};

vertex_out vp_main( vertex_in input )
{
    vertex_out  output;

    output.position = float4(input.pos.x,input.pos.y,input.pos.z,1.0);
    output.uv       = input.uv;

    return output;
}
