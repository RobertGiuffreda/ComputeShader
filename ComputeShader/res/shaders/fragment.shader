#version 430

out vec4 color;

in vec2 tex_coord;

uniform sampler2D color_map;

void main()
{
	color = texture(color_map, tex_coord);
}