#ensuredefined TEXTURED 0

vertex_in
{
    float4  pos : POSITION;
#if TEXTURED
    float2  uv  : TEXCOORD0;
#endif //TEXTURED
    float4  color : COLOR0;
};

vertex_out
{
    float4  pos     : SV_POSITION;
#if TEXTURED
    float2  uv      : TEXCOORD0;
#endif //TEXTURED
    [lowp] half4   color   : COLOR0;
};


[auto][instance] property float4x4 worldViewProjMatrix;

vertex_out vp_main( vertex_in input )
{
    vertex_out  output;

    float3 in_pos      = input.pos.xyz;
#if TEXTURED
    float2 in_texcoord = input.uv;
#endif //TEXTURED
    float4 in_color    = input.color;

    output.pos = mul( float4(in_pos.x,in_pos.y,in_pos.z,1.0), worldViewProjMatrix );
#if TEXTURED
    output.uv = in_texcoord;
#endif //TEXTURED
    output.color = half4(in_color);

    return output;
}
