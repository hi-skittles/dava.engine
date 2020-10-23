fragment_in
{
    float2  uv      : TEXCOORD0;
    float4  color   : COLOR0;
};

fragment_out
{
    float4  color   : SV_TARGET0;
};

uniform sampler2D tex;

fragment_out fp_main( fragment_in input )
{
    fragment_out    output;


    float4 resColor = tex2D( tex, input.uv );
    resColor = resColor * input.color;

    float alpha = resColor.a;
    if( alpha < 0.5 )
        discard;
    else
        resColor.a = 1.0;

    output.color = resColor;
    
    return output;
}
