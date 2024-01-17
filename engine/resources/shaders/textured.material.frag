#version 300 es
precision mediump float;    



uniform vec4 diffuseColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform sampler2D baseColorTexture;
uniform sampler2D mapPosition;
uniform sampler2D mapNormal;
uniform sampler2D mapSSAO;

in vec4 vertexColor;
in vec2 textureCoords;
in vec4 wPosition;
in vec4 pPosition;
layout(location = 0) out vec4 gColor;

void main()
{
vec4 color;

//	***********materialColor**********
	color = vertexColor * diffuseColor;
	
//	***********texturedMaterialColor**********
	vec2 sc = vec2(pPosition.x, pPosition.y) / pPosition.w * 0.5 + vec2(0.5, 0.5);
	//color = color * texture(baseColorTexture, textureCoords) * texture(mapNormal, sc);
	
//	***********ssao**********

    vec2 texelSize = 1.0 / vec2(textureSize(mapSSAO, 0));
    float result = 0.0;
    for (int x = -2; x <= 2; ++x) 
    {
        for (int y = -2; y <= 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(mapSSAO, sc + offset).r;
        }
    }
    color  *= result / (5.0 * 5.0);
	
//	***********output**********
	float light = dot(normalize(mat3(view) * vec3(1, -1, 1)), normalize(texture(mapNormal, sc).xyz));
	gColor = color * (light * 0.1 + 0.9);
	gColor.a = 1.0;
}