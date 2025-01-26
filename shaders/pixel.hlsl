cbuffer Settings
{
    float3 iResolution;
    float iTime;
}

struct vertex_to_pixel
{
    float4 position : SV_POSITION;
    float2 uv : UV;
};

float4 main(in vertex_to_pixel IN) : SV_TARGET
{
    return float4(IN.uv, 0.0, 1.0);
}

