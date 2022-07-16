#version 430

layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba32f, binding = 0) uniform image2D trail_map;
layout(rgba32f, binding = 1) uniform image2D blur_map;
//layout(rgba32f, binding = 2) uniform image2D color_map;

void main()
{
	// Get coordinate for this computation
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	// Copy from blur_map to trail_map
	vec4 to_copy = imageLoad(blur_map, pixel_coords);
	imageStore(trail_map, pixel_coords, to_copy);
}