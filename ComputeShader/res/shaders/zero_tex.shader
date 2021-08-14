#version 430

layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 0) uniform image2D trail_map;

void main()
{
	vec4 pixel = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

	imageStore(trail_map, pixel_coords, pixel);
}