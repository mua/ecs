#version 300 es
precision mediump float;    



uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec4 wPosition;
in vec4 pPosition;
in vec3 vertexNormal;
in vec3 pVertexNormal;
layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;

void main()
{


//	***********output**********
	gNormal = vec4(normalize(pVertexNormal), 1.0);
	gPosition = pPosition / pPosition.w;
}