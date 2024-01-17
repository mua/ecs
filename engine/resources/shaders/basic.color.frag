#version 300 es
precision mediump float;    



uniform vec4 diffuseColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

in vec4 vertexColor;
in vec4 wPosition;
in vec4 pPosition;
layout(location = 0) out vec4 gColor;

void main()
{
vec4 color;

//	***********materialColor**********
	color = vertexColor * diffuseColor;
	
//	***********output**********
	gColor = color;
}