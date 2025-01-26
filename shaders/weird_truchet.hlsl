// https://www.shadertoy.com/view/4ccfRn
/** 

    License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
    
    Corne Keyboard Test Shader
    11/26/2024  @byt3_m3chanic
    
    Got this tiny Corne knock-off type keyboard from Amazon - 36 key
    So this is me trying to code a shader, and memorize the key 
    combos for the special/math chars.
    
    see keyboard here:
    https://bsky.app/profile/byt3m3chanic.bsky.social/post/3lbsqbatwjc2q
    
*/

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

#define R           iResolution
#define T           iTime

#define PI         3.14159265359
#define PI2        6.28318530718

float2x2 rot(float a) {return float2x2(cos(a),sin(a),-sin(a),cos(a));}
float3 hue(float t, float f) { return f+f*cos(PI2*t*(float3(1,.75,.75)+float3(.96,.57,.12)));}
float hash21(float2 a) {return frac(sin(dot(a,float2(27.69,32.58)))*43758.53);}
float box(float2 p, float2 b) {float2 d = abs(p)-b; return length(max(d,0.)) + min(max(d.x,d.y),0.);}

float2 pattern(float2 p, float sc, float2x2 r90) {
    float2 uv = p;
    float2 id = floor(p*sc);
          p = frac(p*sc)-.5;

    float rnd = hash21(id);
    
    // turn tiles
    if (rnd > .5)
        p = mul(p, r90);
    rnd=frac(rnd*32.54);
    if(rnd>.4)
        p = mul(p, r90);
    if(rnd>.8)
        p = mul(p, r90);
    
    // randomize hash for type
    rnd=frac(rnd*47.13);

    float tk = .075;
    // kind of messy and long winded
    float d = box(p-float2(.6,.7),float2(.25,.75))-.15;
    float l = box(p-float2(.7,.5),float2(.75,.15))-.15;
    float b = box(p+float2(0,.7),float2(.05,.25))-.15;
    float r = box(p+float2(.6,0),float2(.15,.05))-.15;
    d = abs(d)-tk; 
    
    if(rnd>.92) {
        d = box(p-float2(-.6,.5),float2(.25,.15))-.15;
        l = box(p-float2(.6,.6),float2(.25,0.25))-.15;
        b = box(p+float2(.6,.6),float2(.25,.25))-.15;
        r = box(p-float2(.6,-.6),float2(.25,.25))-.15;
        d = abs(d)-tk; 
        
    } else if(rnd>.6) {
        d = length(p.x-.2)-tk;
        l = box(p-float2(-.6,.5),float2(.25,.15))-.15;
        b = box(p+float2(.6,.6),float2(.25,.25))-.15;
        r = box(p-float2(.3,0),float2(.25,.05))-.15;
    }
    
    l = abs(l)-tk; b = abs(b)-tk; r = abs(r)-tk;

    float e = min(d,min(l,min(b,r)));
    
    if(rnd>.6) {
        r = max(r,-box(p-float2(.2,.2),tk*1.3*float2(1,1)));
        d = max(d,-box(p+float2(-.2,.2),tk*1.3*float2(1,1)));
    } else {
        l = max(l,-box(p-float2(.2,.2),tk*1.3*float2(1, 1)));
    }
    
    d = min(d,min(l,min(b,r)));

    return float2(d,e);
}
float4 main(vertex_to_pixel IN) : SV_TARGET
{
    float2 F = IN.uv * R.xy;
    float3 C = float3(.0,0,0);
    float2 uv = (2.*F-R.xy)/max(R.x,R.y);
    float2x2 r90 = rot(1.5707);
    
    uv = mul(uv, rot(T * .095));
    //@Shane
    uv = float2(log(length(uv)), atan2(uv.y, uv.x)*6./PI2);
    // Original.
    //uv = vec2(log(length(uv)), atan(uv.y, uv.x))*8./6.2831853;

    float scale = 8.;
    for(float i=0.;i<4.;i++){  
        float ff=(i*.05)+.2;

        uv.x+=T*ff;

        float px = fwidth(uv.x*scale);
        float2 d = pattern(uv,scale, r90);
        float3 clr = hue(sin(uv.x+(i*8.))*.2+.4,(.5+i)*.15);
        C = lerp(C,.001*float3(1,1,1),smoothstep(px,-px,d.y-.04));
        C = lerp(C,clr,smoothstep(px,-px,d.x));
        scale *=.5;
    }

    // Output to screen
    C = pow(C,.4545*float3(1,1,1));
    return float4(C,1.0);
}
