#include "blending.slh"

fragment_in
{
};

fragment_out
{
    float4  color : SV_TARGET0;
};

[material][a] property float4  color ;


fragment_out fp_main( fragment_in input )
{
    fragment_out output;

    output.color = color;

    return output;
}
