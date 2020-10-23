vertex_in
{
    float3  position    : POSITION;
    float2  uv          : TEXCOORD0;
    float4  color       : COLOR0;
};

vertex_out
{
    float4  position    : SV_POSITION;
    float2  uv          : TEXCOORD0;
    float4  color       : COLOR0;
};

[auto][instance] property float4x4 worldViewProjMatrix;

vertex_out vp_main( vertex_in input )
{
    vertex_out  output;

    output.position = mul( float4(input.position.xyz,1.0), worldViewProjMatrix );
    output.uv       = input.uv;
    output.color    = input.color;
    
    return output;
}
