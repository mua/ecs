#version 300 es
precision mediump float;    

layout(location = 0) in vec3 position;
layout(location = 3) in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 wPosition;
out vec4 pPosition;
out vec3 vertexNormal;
out vec3 pVertexNormal;

void main()
{


//	***********transform**********
	wPosition = model * vec4(position, 1.0);
	pPosition = view * wPosition;
	gl_Position = projection * pPosition;
	
//	***********vertexNormal**********
	vertexNormal = normalize(normal);
	
//	***********clipVertexNormal**********
	pVertexNormal = vec3(view *  model * vec4(vertexNormal, 0.0));
	
}