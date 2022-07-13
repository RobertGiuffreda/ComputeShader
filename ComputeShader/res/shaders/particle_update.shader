#version 430

struct particle
{
	vec4 color;
	vec2 pos;
	float dir;
	float pad;
};

layout(local_size_x = 32, local_size_y = 1, local_size_z = 1) in;
layout(rgba32f, binding = 0) uniform image2D trail_map;
layout(std430, binding = 1) buffer particle_map
{
	particle p_map[];
};

// Boundry Dimensions
uniform float width;
uniform float height;

// Particle values
uniform float move_dist;
uniform float sensor_angle;
uniform float sensor_dist;
uniform float turn_speed;
uniform float deposit;

// Time values
uniform float delta_time;
uniform float time;

// Hash function www.cs.ubc.ca/~rbridson/docs/schechter-sca08-turbulence.pdf
uint hash(uint state)
{
	state ^= 2747636419u;
	state *= 2654435769u;
	state ^= state >> 16;
	state *= 2654435769u;
	state ^= state >> 16;
	state *= 2654435769u;
	return state;
}


float uni_r(uint s)
{
	return float(s) / 4294967295.0;
}



float check(particle p, float angle)
{
	float check_angle = p.dir + angle;
	vec2 check_dir = vec2(cos(check_angle), sin(check_angle));

	vec2 check_pos = p.pos + (check_dir * sensor_dist);
	int cx = int(check_pos.x);
	int cy = int(check_pos.y);

	float sum = 0;
	vec4 weight = vec4(1.0f);

	for (int dx = -1; dx <= 1; dx++)
	{
		for (int dy = -1; dy <= 1; dy++)
		{
			int x = int(min(width - 1, max(0, cx + dx)));
			int y = int(min(height - 1, max(0, cy + dy)));
			sum += dot(weight, imageLoad(trail_map, ivec2(x, y)));

			//sum += dot(weight, imageLoad(trail_map, ivec2(cx + dx, cy + dy)));
		}
	}

	return sum;
}

void main()
{
	uint gid = gl_GlobalInvocationID.x;

	uint random = hash(uint(p_map[gid].pos.x * 32.1331f + p_map[gid].pos.y) + hash(uint(gid + time + delta_time)));

	/* Sensing Code */
	float random_turn = uni_r(random) * 2 * 3.1415f;

	/* Sensing Code */
	float left_c = check(p_map[gid], sensor_angle);
	float right_c = check(p_map[gid], -sensor_angle);
	float forward_c = check(p_map[gid], 0);

	if (forward_c > left_c && forward_c > right_c) {}
	else if (forward_c < left_c && forward_c < right_c)
	{
		p_map[gid].dir += (random_turn - 0.5f) * 2 * turn_speed * delta_time;
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

 /* Keep particle in boundry 
	Temprarily bounce without randomness 
	6.28 used as bad 2PI for now */
	if (n_pos.x < 0 || n_pos.x >= width || n_pos.y < 0 || n_pos.y >= height)
	{
		n_pos.x = min(width - 0.01f, max(n_pos.x, 0.0f));
		n_pos.y = min(height - 0.01f, max(n_pos.y, 0.0f));
		random = hash(random);
		p_map[gid].dir = uni_r(random) * 6.28;
	} else {
		ivec2 cd = ivec2(n_pos);
		vec4 old_trail = imageLoad(trail_map, cd);
		imageStore(trail_map, cd, old_trail + vec4(deposit * delta_time));
		// Deposit value accumulates: Should there be a ceiling it reaches??
	}

	// Update the ssbo's data with new data
	p_map[gid].pos = n_pos;
}