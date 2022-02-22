#version 460

in vec4 position;
in vec2 inTexCoord;

out vec2 texCoord;

out vec2 screenCoord;

void main()
{
	gl_Position = position;
	texCoord = inTexCoord;
	screenCoord = gl_Position.xy;
}