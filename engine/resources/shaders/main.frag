#version 300 es
precision highp float;

//inputs:

in vec4 vVertexColor;

void main()
{
	vec4 color = vVertexColor;
	gl_FragColor = color;
}
