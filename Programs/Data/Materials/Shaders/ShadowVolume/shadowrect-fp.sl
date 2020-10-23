blending { src=dst_color dst=zero }

fragment_in {};

fragment_out
{
    float4  color : SV_TARGET0;
};

[auto][instance] property float4 shadowColor = float4(1.0,0,0,1.0);

fragment_out fp_main( fragment_in input )
{
    fragment_out    output;

    output.color = float4(shadowColor.rgb, 1.0);
    
    return output;
}
