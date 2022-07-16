#version 430

struct settings
{
	vec4 color;
};

layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba32f, binding = 0) uniform image2D trail_map;
layout(rgba32f, binding = 2) uniform image2D color_map;

layout(std430, binding = 2) buffer settings_map
{
	settings s[];
};

uniform int group_num;

void main()
{
	ivec2 px = ivec2(gl_GlobalInvocationID.xy);
	vec4 map = imageLoad(trail_map, px);

	vec4 color = vec4(0.0f);
	for (int i = 0; i < group_num; ++i) {
		vec4 mask = vec4(i==0, i==1, i==2, i==3);
		color += s[i].color * dot(mask, map);
	}
	imageStore(color_map, px, color);
}