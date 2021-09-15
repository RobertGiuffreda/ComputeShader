#version 430

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex;

out vec2 tex_coord;

void main()
{
	gl_Position = vec4(pos, 1.0f);
	tex_coord = tex;
}