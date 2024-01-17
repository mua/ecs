#version 300 es
precision mediump float;    

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

uniform vec4 diffuseColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec4 vertexColor;
out vec4 wPosition;
out vec4 pPosition;

void main()
{


//	***********vertexColor**********
	vertexColor = color;
	
//	***********transform**********
	wPosition = model * vec4(position, 1.0);
	pPosition = projection * view * wPosition;
	gl_Position = pPosition;
	
}