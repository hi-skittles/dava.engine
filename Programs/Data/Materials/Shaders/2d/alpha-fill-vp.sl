vertex_in
{
    float4  pos : POSITION;
};

vertex_out
{
    float4  pos : SV_POSITION;
};

[auto][instance] property float4x4 worldViewProjMatrix;

vertex_out vp_main( vertex_in input )
{
    vertex_out  output;
    float3      in_pos = input.pos.xyz;
    
    output.pos = mul( float4(in_pos.x,in_pos.y,in_pos.z,1.0), worldViewProjMatrix );
    
    return output;
}
