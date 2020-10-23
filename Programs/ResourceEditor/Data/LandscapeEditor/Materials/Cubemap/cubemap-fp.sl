fragment_in
{
    float3  normal : TEXCOORD0;
};

fragment_out
{
    float4  color   : SV_TARGET0;
};

uniform samplerCUBE cubemap;


fragment_out fp_main( fragment_in input )
{
    fragment_out    output;

    output.color = texCUBE( cubemap, input.normal );

    return output;
}
