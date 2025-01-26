// https://www.shadertoy.com/view/M3ycWd
// The MIT License
// Copyright © 2025 Inigo Quilez
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


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

// Snap unit vectors to 45 degree angles without using atan.


// spalmer+iq version (2.8x faster than trigonometric), fixed by FordPerfect
float2 snap45(in float2 v)
{
    v = round(v * 1.30656296); // sqrt(1.0+sqrt(0.5))
    return v * (abs(v.x) + abs(v.y) > 1.5 ? sqrt(0.5) : 1.0);
    
    // as fast (ADD, FMA, MUL) instead of (ADD, CMP, SEL, MUL)
    // return v*((2.0-sqrt(0.5))-(1.0-sqrt(0.5))*(abs(v.x)+abs(v.y)));
}

// spalmer version (2.1x faster than trigonometric), fixed by FordPerfect
float2 snap45_spalmer(in float2 v)
{
    return normalize(round(v * 1.30656296)); // sqrt(1.0+sqrt(0.5)) = 1/2sin(PI/8)
}

// iq version (1.8x faster than trigonometric)
float2 snap45_iq(in float2 v)
{
    float2 s = sign(v);
    float x = abs(v.x);
    return x > 0.92387953 ? float2(s.x, 0.0) : // cos(PI*1/8)
           x < 0.38268343 ? float2(0.0, s.y) : // cos(PI*3/8)
                        s * sqrt(0.5);
}

// iq initial experiment, not great
float2 snap45_experiment(in float2 v)
{
    float2 s = sign(v);
    float2 a = abs(v);
    return (a.y < a.x * (sqrt(2.0) - 1.0)) ? float2(s.x, 0.0) :
           (a.x < a.y * (sqrt(2.0) - 1.0)) ? float2(0.0, s.y) :
                                       s * sqrt(0.5);
}

// trigonometric reference, very slow (specially in CPUs)
float2 snap45_trig(in float2 v)
{
    const float r = 6.283185 / 8.0; // 45 degrees

    float a = atan2(v.y, v.x); // vector to angle
    float s = r * round(a / r); // snap angle
    return float2(cos(s), sin(s)); // angle to vector
}

//===============================================================

float line1(in float2 p, in float2 b, in float th, in float w)
{
    return smoothstep(w, -w, length(p - b * clamp(dot(p, b) / dot(b, b), 0.0, 1.0)) - th);
}

float4 main(vertex_to_pixel IN) : SV_TARGET
{
    float2 fragCoord = IN.uv * iResolution.xy;
    float2 p = (2.0 * fragCoord - iResolution.xy) / iResolution.y;
    float px = 2.0 / iResolution.y;
    
    // full background
    float3 col = float3(21, 32, 43) / 255.0;

    const float ra = 0.9;

    // draw circle
    {
        float r = length(p);
        if (r < ra)
        {
        // radial gradient
            col += 0.25 * r * r / (ra * ra);
        // 8 regions
            float2 q = abs(p);
            if (q.y < q.x * (sqrt(2.0) - 1.0) ||
            q.x < q.y * (sqrt(2.0) - 1.0))
                col += 0.03;
        }
        col += 0.4 * smoothstep(1.5 * px, 0.0, abs(r - ra) - 0.003);
    }
    
    // draw 8 octants
    {
        const float2 k = float2(-sqrt(2.0 + sqrt(2.0)) / 2.0,
                         sqrt(2.0 - sqrt(2.0)) / 2.0);
        float2 q = abs(p);
        q -= 2.0 * min(dot(float2(k.x, k.y), q), 0.0) * float2(k.x, k.y);
        q -= 2.0 * min(dot(float2(-k.x, k.y), q), 0.0) * float2(-k.x, k.y);
        col += 0.2 * line1(q, float2(0.0, ra), 0.003, px);
    }
    
    // draw vector, and snap vector
    {
        float2 v = normalize(
        cos(6.283185 * (iTime / 15.0 + float2(0.0, -0.25))));
        float2 s = snap45(v);
        col = lerp(col, float3(0.9, 0.90, 0.90), line1(p, v * ra, 0.003, px));
        col = lerp(col, float3(3.0, 0.95, 0.20), line1(p, s * ra, 0.004, px));
    }
    
    return float4(col, 1.0);
}