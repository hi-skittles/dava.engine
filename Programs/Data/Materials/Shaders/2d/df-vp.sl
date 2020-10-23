vertex_in
{
    float4  pos     : POSITION;
    float2  uv      : TEXCOORD;
    float4  color   : COLOR0;
};

vertex_out
{
    float4  pos     : SV_POSITION;
    float2  uv      : TEXCOORD0;
    [lowp] half4   color   : COLOR0;
};

[auto][instance] property float4x4 worldViewProjMatrix;

vertex_out vp_main( vertex_in input )
{
    vertex_out  output;
    float3      in_pos      = input.pos.xyz;
    float2      in_texcoord = input.uv;
    float4      in_color    = input.color;

    output.pos   = mul( float4(in_pos.x,in_pos.y,in_pos.z,1.0), worldViewProjMatrix );
    output.uv    = in_texcoord;
    output.color = in_color;
    
    return output;
}
