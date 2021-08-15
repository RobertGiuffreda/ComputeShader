#version 430

struct particle
{
	vec2 pos;
	float dir;
	float pad;
};

layout(local_size_x = 16, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D trail_map;
layout(std430, binding = 1) buffer particle_map
{
	particle p_map[];
};

uniform float width;
uniform float height;
uniform float move_dist;
uniform float sensor_angle;
uniform float sensor_dist;
uniform float turn_speed;
uniform float delta_time;

// Hash11 from https://www.shadertoy.com/view/4djSRW
float hash(float p)
{
	p = fract(p * .1031);
	p *= p + 33.33;
	p *= p + p;
	return fract(p);
}

float check(particle p, float angle)
{
	float check_angle = p.dir + angle;
	vec2 check_dir = vec2(cos(check_angle), sin(check_angle));

	vec2 check_pos = p.pos + (check_dir * sensor_dist);
	int centerX = int(check_pos.x);
	int centerY = int(check_pos.y);

	float sum = 0;
	vec4 weight = vec4(1.0f, 1.0f, 1.0f, 1.0f);

	for (int offx = -1; offx <= 1; offx++)
	{
		for (int offy = -1; offy <= 1; offy++)
		{
			int xval = int(min(width - 1, max(0, centerX + offx)));
			int yval = int(min(height - 1, max(0, centerY + offy)));
			sum += dot(weight, imageLoad(trail_map, ivec2(xval, yval)));
		}
	}

	return sum;
}

void main()
{
	uint gid = gl_GlobalInvocationID.x;

	float random = p_map[gid].pos.y * gid + p_map[gid].pos.x + hash(p_map[gid].pos.x * p_map[gid].pos.y* 1000);

	float random_turn = hash(random) * 2 * 3.1415f;

	/* Sensing Code */
	float left_c = check(p_map[gid], sensor_angle);
	float right_c = check(p_map[gid], -sensor_angle);
	float forward_c = check(p_map[gid], 0);

	if (forward_c > left_c && forward_c > right_c)
	{
		p_map[gid].dir += 0.0f;
	}
	else if (forward_c < left_c && forward_c < right_c)
	{
		p_map[gid].dir += (random_turn - 0.5f) * 2 * turn_speed *delta_time;
	}
	else if (right_c > left_c)
	{
		p_map[gid].dir -= random_turn * turn_speed * delta_time;
	}
	else if (left_c > right_c)
	{
		p_map[gid].dir += random_turn * turn_speed * delta_time;
	}

	vec2 dir = vec2(cos(p_map[gid].dir), sin(p_map[gid].dir));
	vec2 n_pos = p_map[gid].pos + dir * move_dist * delta_time;

	/* Implementation of sensing and choosing path */
	

 /* Keep particle in boundry 
	Temprarily bounce without randomness 
	6.28 used as bad 2PI for now */
	if (n_pos.x < 0 || n_pos.x >= width || n_pos.y < 0 || n_pos.y >= height)
	{
		n_pos.x = min(width - 0.01f, max(n_pos.x, 0.0f));
		n_pos.y = min(height - 0.01f, max(n_pos.y, 0.0f));
		p_map[gid].dir = hash(random) * 6.28;
	}

	// Update the ssbo's data with new data
	p_map[gid].pos = n_pos;

	// Color the trailmap depending on new data
	ivec2 pixel_coords = ivec2(p_map[gid].pos);
	imageStore(trail_map, pixel_coords, vec4(1.0f, 1.0f, 1.0f, 1.0f));
}