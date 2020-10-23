#include "common.slh"

vertex_in
{
    float4  position : POSITION;
    float3  normal   : NORMAL;
    
    #if HARD_SKINNING
    float   index    : BLENDINDICES;
    #endif
};

vertex_out
{
    float4  position : SV_POSITION;
};

[auto][a] property float4x4 worldViewProjMatrix;
[auto][a] property float4x4 worldViewMatrix;
[auto][a] property float4x4 projMatrix;
[auto][a] property float4x4 worldViewInvTransposeMatrix;

#if HARD_SKINNING
[auto][jpos] property float4 jointPositions[MAX_JOINTS] : "bigarray" ; // (x, y, z, scale)
[auto][jrot] property float4 jointQuaternions[MAX_JOINTS] : "bigarray" ;
#endif

[material][a] property float silhouetteScale = 1.0;
[material][a] property float silhouetteExponent = 0;

#if HARD_SKINNING

inline float3 JointTransformTangent( float3 tangent, float4 quaternion )
{
    float3 tmp = 2.0 * cross(quaternion.xyz, tangent);
    return tangent + quaternion.w * tmp + cross(quaternion.xyz, tmp);
}

#endif

vertex_out vp_main( vertex_in input )
{
    vertex_out  output;

    float4 position;
    float3 normal;
    
#if HARD_SKINNING
    {
        int jIndex = int(input.index);
        
        float4 jP = jointPositions[jIndex];
        float4 jQ = jointQuaternions[jIndex];
    
        float3 tmp = 2.0 * cross(jQ.xyz, input.position.xyz);
        position = float4(jP.xyz + (input.position.xyz + jQ.w * tmp + cross(jQ.xyz, tmp)) * jP.w, 1.0);

        normal = JointTransformTangent(input.normal, jQ);
    }
#else
    position = float4(input.position.xyz, 1.0);
    normal = input.normal;
#endif

    normal = normalize(mul(float4(normal, 0.0), worldViewInvTransposeMatrix).xyz);
    float4 PosView = mul(position, worldViewMatrix);

    float distanceScale = length(PosView.xyz) / 100.0;

    PosView.xyz += normal * pow(silhouetteScale * distanceScale, silhouetteExponent);
    output.position = mul(PosView, projMatrix);
    
    return output;
}
