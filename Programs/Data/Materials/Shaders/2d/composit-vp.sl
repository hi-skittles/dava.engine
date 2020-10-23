vertex_in
{
    float4  position    : POSITION;
    float2  uv0         : TEXCOORD0;
    float2  uv1         : TEXCOORD1;
    float2  uv2         : TEXCOORD2;
    float2  uv3         : TEXCOORD3;
    float4  color       : COLOR0;
};

vertex_out
{
    float4  position    : SV_POSITION;
    float2  uvMask      : TEXCOORD0;
    float2  uvDetail    : TEXCOORD1;
    float2  uvGradient  : TEXCOORD2;
    float2  uvContour   : TEXCOORD3;
    [lowp] half4   color       : COLOR0;
};

[auto][instance] property float4x4 worldViewProjMatrix;


vertex_out vp_main( vertex_in input )
{
    vertex_out  output;

    float3 in_pos      = input.position.xyz;
    output.position = mul( float4(in_pos.x,in_pos.y,in_pos.z,1.0), worldViewProjMatrix );

    output.uvMask   = input.uv0;
    output.uvDetail = input.uv1;
    output.uvGradient = input.uv2;
    output.uvContour = input.uv3;

    output.color =  half4(input.color);

    return output;
}
