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

static const float REPEAT = 5.0;

// 回転行列
float2x2 rot(float a) {
	float c = cos(a), s = sin(a);
	return float2x2(c,s,-s,c);
}

float sdBox( float3 p, float3 b )
{
	float3 q = abs(p) - b;
	return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float box(float3 pos, float scale) {
	pos *= scale;
	float base = sdBox(pos, float3(.4,.4,.1)) /1.5;
	pos.xy *= 5.;
	pos.y -= 3.5;
    pos.xy *= mul(pos.xy, rot(.75));
	float result = -base;
	return result;
}

float box_set(float3 pos, float iTime, float gTime) {
	float3 pos_origin = pos;
	pos = pos_origin;
	pos .y += sin(gTime * 0.4) * 2.5;
    pos.xy *= mul(pos.xy, rot(.8));
	float box1 = box(pos,2. - abs(sin(gTime * 0.4)) * 1.5);
	pos = pos_origin;
	pos .y -=sin(gTime * 0.4) * 2.5;
    pos.xy *= mul(pos.xy, rot(.8));
	float box2 = box(pos,2. - abs(sin(gTime * 0.4)) * 1.5);
	pos = pos_origin;
	pos .x +=sin(gTime * 0.4) * 2.5;
    pos.xy *= mul(pos.xy, rot(.8));
	float box3 = box(pos,2. - abs(sin(gTime * 0.4)) * 1.5);	
	pos = pos_origin;
	pos .x -=sin(gTime * 0.4) * 2.5;
    pos.xy *= mul(pos.xy, rot(.8));
	float box4 = box(pos,2. - abs(sin(gTime * 0.4)) * 1.5);	
	pos = pos_origin;
    pos.xy *= mul(pos.xy, rot(.8));
	float box5 = box(pos,.5) * 6.;	
	pos = pos_origin;
	float box6 = box(pos,.5) * 6.;	
	float result = max(max(max(max(max(box1,box2),box3),box4),box5),box6);
	return result;
}

float map(float3 pos, float iTime, float gTime) {
	float3 pos_origin = pos;
	float box_set1 = box_set(pos, iTime, gTime);

	return box_set1;
}

float mod(float a, float b)
{
    return a - b * floor(a / b);
}
float3 mod(float3 a, float3 b)
{
    return float3(mod(a.x, b.x), mod(a.y, b.y), mod(a.z, b.z));
}

float4 main(in vertex_to_pixel IN) : SV_TARGET
{
    float2 fragCoord = IN.uv * iResolution.xy;
	float2 p = (fragCoord.xy * 2. - iResolution.xy) / min(iResolution.x, iResolution.y);
    //p = IN.uv;
	float3 ro = float3(0., -0.2 ,iTime * 4.);
	float3 ray = normalize(float3(p, 1.5));
    ray.xy = mul(ray.xy, rot(sin(iTime * .03) * 5.));
    ray.yz = mul(ray.yz, rot(sin(iTime * .05) * .2));
	float t = 0.1;
	float3 col = float3(0, 0, 0);
	float ac = 0.0;


	for (int i = 0; i < 99; i++){
		float3 pos = ro + ray * t;
        pos = mod(pos - float3(2, 2, 2), float3(4, 4, 4)) - 2.;
		float gTime = iTime -float(i) * 0.01;
		
		float d = map(pos, iTime, gTime);

		d = max(abs(d), 0.01);
		ac += exp(-d*23.);

		t += d* 0.55;
	}

    col = ac * 0.02 * float3(1, 1, 1);

	col += float3(0.,0.2 * abs(sin(iTime)),0.5 + sin(iTime) * 0.2);


    return float4(col, 1.0);
}

/** SHADERDATA
{
	"title": "Octgrams",
	"description": "Lorem ipsum dolor",
	"model": "person"
}
*/
