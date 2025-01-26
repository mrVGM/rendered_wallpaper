struct vertex_info
{
    float2 position : POSITION;
    float2 uv : UV;
};

struct vertex_to_pixel
{
    float4 position : SV_POSITION;
    float2 uv : UV;
};

void main(in vertex_info IN, out vertex_to_pixel OUT)
{
    OUT.position = float4(IN.position, 0.0, 1.0);
    OUT.uv = IN.uv;
};
