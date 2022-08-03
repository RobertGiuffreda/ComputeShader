#version 430

layout(local_size_x = 8, local_size_y = 8) in;
layout(rgba32f, binding = 0) uniform image2D trail_map;
layout(rgba32f, binding = 1) uniform image2D blur_map;

uniform float decay_rate;
uniform float blur_factor;
uniform float delta_time;

void main()
{
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
	vec4 sum = vec4(0.0f);
	for (int offX = -1; offX <= 1; offX++) {
		for (int offY = -1; offY <= 1; offY++) {
			ivec2 tmp_coord = ivec2(pixel_coords.x + offX, pixel_coords.y + offY);
			vec4 tmp_color = imageLoad(trail_map, tmp_coord);
			if (abs(offX) + abs(offY) == 2) tmp_color *= 1.0f;
			if (abs(offX) + abs(offY) == 1) tmp_color *= 2.0f;
			if (abs(offX) + abs(offY) == 0) tmp_color *= 4.0f;
			sum += tmp_color;
		}
	}
	sum /= 16.0f;

	sum = mix(imageLoad(trail_map, pixel_coords), sum, blur_factor * delta_time);
	sum = mix(sum, sum * decay_rate, delta_time);

	imageStore(blur_map, pixel_coords, sum);
}