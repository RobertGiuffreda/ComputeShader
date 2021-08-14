#version 430

struct particle
{
	vec2 pos;
	float dir;
};

layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D trail_map;
layout(std430, binding = 1) buffer particle_map
{
	particle p_map[];
};

// Called with 
void main()
{
	vec4 pixel = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	ivec2 pixel_coords = ivec2(p_map[gl_GlobalInvocationID.x].pos);
	imageStore(trail_map, pixel_coords, pixel);
}