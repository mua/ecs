#version 300 es
precision mediump float;    

layout(location = 0) in vec3 position;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec4 worldPosition;
out vec4 screenPosition;

void main()
{


//	***********transform**********
	gl_Position = projection * view * model * vec4(position, 1.0);
	
}