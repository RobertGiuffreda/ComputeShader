#version 330

out vec4 color;

in vec2 tex_coord;

uniform sampler2D particle_tex;

void main()
{
	color = texture(particle_tex, tex_coord);
}