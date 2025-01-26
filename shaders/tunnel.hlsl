// https://www.shadertoy.com/view/MfVfz3
cbuffer Settings
{
    float3 iResolution;
    float iTimeO;
}

struct vertex_to_pixel
{
    float4 position : SV_POSITION;
    float2 uv : UV;
};


#define hash(x) frac(sin(x) * 43758.5453123)
float3 pal(float t)
{
    return .5 + .5 * cos(6.28 * (1. * t + float3(.0, .1, .1)));
}
float stepNoise(float x, float n)
{ // From Kamoshika shader
    const float factor = 0.3;
    float i = floor(x);
    float f = x - i;
    float u = smoothstep(0.5 - factor, 0.5 + factor, f);
    float res = lerp(floor(hash(i) * n), floor(hash(i + 1.) * n), u);
    res /= (n - 1.) * 0.5;
    return res - 1.;
}
float3 path(float3 p)
{
    float3 o = float3(0, 0, 0);
    o.x += stepNoise(p.z * .05, 5.) * 5.;
    o.y += stepNoise(p.z * .07, 3.975) * 5.;
    return o;
}
float diam2(float2 p, float s)
{
    p = abs(p);
    return (p.x + p.y - s) / sqrt(3.);
}
float3 erot(float3 p, float3 ax, float t)
{
    return lerp(dot(ax, p) * ax, p, cos(t)) + cross(ax, p) * sin(t);
}

float mod(float a, float b)
{
    return a - b * floor(a / b);
}

float4 main(in vertex_to_pixel IN) : SV_TARGET //out vec4 fragColor, in vec2 fragCoord)
{
    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = IN.uv - 0.5;

    float3 col = float3(0, 0, 0);

    float iTime = iTimeO / 15.0;

    float3 ro = float3(0., 0., -1.), rt = float3(0, 0, 0);
    ro.z += iTime * 5.;
    rt.z += iTime * 5.;
    ro += path(ro);
    rt += path(rt);
    float3 z = normalize(rt - ro);
    float3 x = float3(z.z, 0., -z.x);
    float i = 0., e = 0., g = 0.;

    float3 cs = cross(z, x);
    float3x3 tmpMat = float3x3(
        x.x, cs.x, z.x,
        x.y, cs.y, z.y,
        x.z, cs.z, z.z
    );
    float3 rd = mul(transpose(float3x3(x, cs, z)), erot(normalize(float3(uv, 1.)), float3(0., 0., 1.), stepNoise(iTime + hash(uv.x*uv.y*iTime) * .05, 6.)));
    for (; i++ < 99.;)
    {
        float3 p = ro + rd * g;

        p -= path(p);
        float r = 0.;;
        float3 pp = p;
        float sc = 1.;
        for (float j = 0.; j++ < 4.;)
        {
            r = clamp(r + abs(dot(sin(pp * 3.), cos(pp.yzx * 2.)) * .3 - .1) / sc, -.5, .5);
            pp = erot(pp, normalize(float3(.1, .2, .3)), .785 + j);
            pp += pp.yzx + j * 50.;
            sc *= 1.5;
            pp *= 1.5;
        }
      
        float h = abs(diam2(p.xy, 7.)) - 3. - r;
   
        p = erot(p, float3(0., 0., 1.), path(p).x * .5 + p.z * .2);
        float t = length(abs(p.xy) - .5) - .1;
        h = min(t, h);
        g += e = max(.001, t == h ? abs(h) : (h));
        col += (t == h ? float3(.3, .2, .1) * (100. * exp(-20. * frac(p.z * .25 + iTime))) * mod(floor(p.z * 4.) + mod(floor(p.y * 4.), 2.), 2.) : float3(.1, 0, 0)) * .0325 / exp(i * i * e);;
    }
    col = lerp(col, float3(.9, .9, 1.1), 1. - exp(-.01 * g * g * g));
    // Output to screen
    return float4(col, 1.0);
}

