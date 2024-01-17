#version 300 es
precision mediump float;    

layout(location = 0) in vec3 position;

uniform mat4 view;
uniform mat4 projection;
uniform sampler2D mapPosition;
uniform sampler2D mapNormal;

out vec4 pPosition;

void main()
{


//	***********ndcTransform**********
	pPosition = vec4(position, 1.0);
	gl_Position = pPosition;
	
}