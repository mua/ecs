#version 300 es
precision mediump float;    



uniform vec4 diffuseColor;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

in vec4 vertexColor;
in vec4 worldPosition;
in vec4 screenPosition;

void main()
{
vec4 color;

//	***********materialColor**********
	color = vertexColor * diffuseColor;
	
//	***********output**********
	color.a = 1.0;
	gl_FragColor = color;
}