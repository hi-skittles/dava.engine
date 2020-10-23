#include "common.slh"

vertex_in
{
    float3  pos     : POSITION;
    float3  normal  : NORMAL;

    #if HARD_SKINNING
    float  index   : BLENDINDICES;
    #endif
};

vertex_out
{
    float4  pos     : SV_POSITION;        
};

[auto][instance] property float4x4 worldViewMatrix;
[auto][instance] property float4x4 worldViewInvTransposeMatrix;
[auto][instance] property float4 lightPosition0;

[auto][global] property float4x4 projMatrix;

#if HARD_SKINNING
[auto][jpos] property float4 jointPositions[MAX_JOINTS] : "bigarray"; // (x, y, z, scale)
[auto][jrot] property float4 jointQuaternions[MAX_JOINTS] : "bigarray";
#endif

#if FORCED_SHADOW_DIRECTION
[auto][global] property float4x4 viewMatrix;
[material][global] property float3 forcedShadowDirection = float3(0.0, 0.0, -1.0);
#endif

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

    float3 in_pos      = input.pos.xyz;
    float3 in_normal   = input.normal;

    float3x3 normalMatrix = float3x3(worldViewInvTransposeMatrix[0].xyz, 
                                     worldViewInvTransposeMatrix[1].xyz, 
                                     worldViewInvTransposeMatrix[2].xyz);

    float4 position;
    float3 normal;
    
#if HARD_SKINNING
    {
        int jIndex = int(input.index);
        
        float4 jP = jointPositions[jIndex];
        float4 jQ = jointQuaternions[jIndex];
    
        float3 tmp = 2.0 * cross(jQ.xyz, in_pos.xyz);
        position = float4(jP.xyz + (in_pos.xyz + jQ.w * tmp + cross(jQ.xyz, tmp)) * jP.w, 1.0);
        
        normal = normalize( mul( JointTransformTangent(in_normal, jQ), normalMatrix ) );
    }
#else
    position = float4(in_pos.x, in_pos.y, in_pos.z, 1.0);
    normal = mul( in_normal, normalMatrix );
#endif

    float4 posView = mul( position, worldViewMatrix );

#if FORCED_SHADOW_DIRECTION
    float3 lightVecView = normalize(mul(float4(forcedShadowDirection, 0.0), viewMatrix).xyz);        
#else
    float3 lightVecView = normalize(posView.xyz * lightPosition0.w - lightPosition0.xyz);        
#endif
    

    float4 posProj = mul(posView, projMatrix);
    
    if (dot(normal, lightVecView) > 0.0)    
    {
    
        float g = 1.45; //guardband extrusion limit
        float depthExtrusion = 200.0; //depth extrusion limit - for now just const, but it can be possibly computed on cpu as distance to landscape
                
        //compute projected space vector
        float4 lightProj = mul(float4(lightVecView, 0.0), projMatrix);    
        
        //compute extrusion value to hit guardband 
        float2 sl = sign(lightProj.xy);
        float2 evRes = (posProj.w*g*sl - posProj.xy)/(lightProj.xy - lightProj.w*g*sl);
        
        //negative component means that ray will not hit selected plane (as frustum is not a cube!)
        //equal to per-compound if (evRes < 0.0) evRes = depthExtrusion;            
        evRes = lerp(evRes, float2(depthExtrusion, depthExtrusion), step(evRes, float2(0.0, 0.0)));
        
        //select nearest hit between planes
        float ev = min(evRes.x, evRes.y);                        
            
        //extrude to corresponding value
        output.pos = posProj+lightProj*ev;        
    }       
    else
    {                
        output.pos = posProj;                
    }    
    
    return output; 
};
