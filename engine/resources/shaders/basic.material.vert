#version 300 es
precision mediump float;    

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 normal;

uniform vec4 diffuseColor;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec4 vertexColor;
out vec4 worldPosition;
out vec4 screenPosition;

void main()
{


//	***********vertexColor**********
	vertexColor = color;
	
//	***********transform**********
	gl_Position = projection * view * model * vec4(position, 1.0);
	
}