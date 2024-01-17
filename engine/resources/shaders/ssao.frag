#version 300 es
precision mediump float;    



uniform mat4 view;
uniform mat4 projection;

uniform sampler2D mapPosition;
uniform sampler2D mapNormal;

uniform sampler2D ssaoNoiseTexture;

uniform vec3 ssaoKernel[64];

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 64;
float radius = 0.5;
float bias = 0.025;

// tile noise texture over screen based on screen dimensions divided by noise size
const vec2 noiseScale = vec2(800.0/4.0, 600.0/4.0); 


in vec4 pPosition;

layout(location = 0) out vec4 gColor;

float rand(float n){return fract(sin(n) * 43758.5453123);}

float noise(float p){
	float fl = floor(p);
  float fc = fract(p);
	return mix(rand(fl), rand(fl + 1.0), fc);
}

void main()
{
vec4 color = vec4(1.0);

//	***********ssaoGen**********
	vec2 sc = vec2(pPosition.x, pPosition.y) / pPosition.w * 0.5 + vec2(0.5, 0.5);
	vec3 normal = normalize(texture(mapNormal, sc).xyz);
	//normal = vec3(1,0,0);

	vec3 fragPos = texture(mapPosition, sc).xyz;

    vec3 randomVec = normalize(texture(ssaoNoiseTexture, sc * noiseScale).xyz);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 smp = TBN * ssaoKernel[i]; // from tangent to view-space
        smp = fragPos + smp * radius; 
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(smp, 1.0);
        offset = projection * offset; // from view to clip-space
        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        
        // get sample depth
        float sampleDepth = texture(mapPosition, offset.xy).z; // get depth value of kernel sample
        
        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= smp.z + bias ? 1.0 : 0.0) * rangeCheck;           
    }
    occlusion = 1.0 - (occlusion / float(kernelSize));
    
//	***********output**********
	color *= occlusion;
	//color.xyz = randomVec;
	color.a = 1.0;
	gColor = color;
}