#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm.hpp>
#include <iostream>

#include "Random.h"
#include "Shader.h"
#include "ComputeShader.h"

void Print_Group_Values();

/* Sensor is in degrees */
const float SCR_WIDTH = 720.0f;
const float SCR_HEIGHT = 720.0f;

const float TEX_WIDTH = 720.0f;
const float TEX_HEIGHT = 720.0f;

const float move_dist = 10.0f;
const float sensor_angle = glm::radians(45.0f);
const float sensor_dist = 20.0f;
const float turn_speed = 5.0f;

/* Dissapation and decay */
const float decay_rate = 0.08f;
const float blur_factor = 1.0f;

/* Number of Particles */
const unsigned int PNUM = 300000;

float delta_time = 0.0f;
float last_frame = 0.0f;

int main(void)
{
	GLFWwindow* window;

	/* Initialize the Library*/
	if (!glfwInit())
		return -1;

	/* Create Windowed mode window and its OpenGL context */
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello World!", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make windows context current */
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initiate glad" << std::endl;
		glfwTerminate();
		return -1;
	}

	/*
	 * Create the Particle map. Shader storage buffer object of particle positions and directions 
	 * There are issues with padding and Shader Storage Buffer Objects.
	 * vec2 and a scalar will cause issue. Thus the padding
	 */
	struct particle {
		glm::vec2 pos;
		float dir;
		float pad;
	};

	unsigned int ssbo = 0;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(particle) * PNUM, nullptr, GL_DYNAMIC_COPY);

	/*
	* Map buffer to make to addressable by the cpu. Get the address of the buffer in memory 
	* Place the inital values of the particles in the buffer
	* Unmap the buffer before use again
	*/
	int buffmask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	struct particle *particles = (struct particle *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, PNUM * sizeof(struct particle), buffmask);
	for (int i = 0; i < PNUM; i++)
	{
		float third_w = uniform() * TEX_WIDTH;
		float third_h = uniform() * TEX_HEIGHT;
		particles[i].pos = glm::vec2(third_w, third_h);
		/* Get direction to point towards center */
		particles[i].dir = glm::radians(270.0f);
		particles[i].pad = 0;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	/* Bind the buffer to binding = 1 so the compute shader can see it */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Create framebuffer and attach trail_map to use render as a copy
	unsigned int trail_map;
	glGenTextures(1, &trail_map);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, trail_map);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, trail_map, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	unsigned int blur_trail_map;
	glGenTextures(1, &blur_trail_map);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, blur_trail_map);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(1, blur_trail_map, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);


	/* Create compute shader to zero the texture */
	ComputeShader compshader("res/shaders/zero_tex.shader");
	compshader.Bind();
	glDispatchCompute((unsigned int)TEX_WIDTH, (unsigned int)TEX_HEIGHT, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	/* Create compute shader to update particle positions */
	ComputeShader p_update("res/shaders/particle_update.shader");
	p_update.Bind();
	p_update.SetUniform1f("width", TEX_WIDTH);
	p_update.SetUniform1f("height", TEX_HEIGHT);
	p_update.SetUniform1f("move_dist", move_dist);
	p_update.SetUniform1f("sensor_dist", sensor_dist);
	p_update.SetUniform1f("sensor_angle", sensor_angle);
	p_update.SetUniform1f("turn_speed", turn_speed);

	/* Create diffuse and decay compute shader */
	ComputeShader decay("res/shaders/trail_update.shader");
	decay.Bind();
	decay.SetUniform1f("decay_rate", decay_rate);
	decay.SetUniform1f("blur_factor", blur_factor);

	ComputeShader copy("res/shaders/copy_texture.shader");
	copy.Bind();

	/* Create shader to display texture */
	Shader shader("res/shaders/vertex.shader", "res/shaders/fragment.shader");
	shader.Bind();
	shader.SetUniform1i("particle_tex", 0);

	/* Vertices of polygon to display texture */
	float screen_poly[] = {
		 1.0f,  1.0f, 0.0f, 	1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 	1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 	0.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 	0.0f, 1.0f,
	};

	unsigned int screen_indices[] = {
		0, 1, 2,
		2, 3, 0,
	};

	/* OpenGL data structures to display texture on screen */
	unsigned int vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	unsigned int vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)* (5 * 4), screen_poly, GL_STATIC_DRAW);

	unsigned int ibo;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 6, screen_indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	/* Loop until user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		float curr_frame = (float)glfwGetTime();
		delta_time = curr_frame - last_frame;
		last_frame = curr_frame;

		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);

		/* Call particle update compute shader */
		p_update.Bind();
		p_update.SetUniform1f("delta_time", delta_time);
		glDispatchCompute(PNUM, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		/* Call trail map update compute shader */
		decay.Bind();
		decay.SetUniform1f("delta_time", delta_time);
		glDispatchCompute((unsigned int)TEX_WIDTH, (unsigned int)TEX_HEIGHT, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		/* Copy blur_map to trail_map */
		copy.Bind();
		glDispatchCompute((unsigned int)TEX_WIDTH, (unsigned int)TEX_HEIGHT, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		/* Texture stuff might not be necessary. P-sure its not actually */
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, trail_map);

		/* Display Screen Sized Texture */
		shader.Bind();
		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
		
		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events*/
		glfwPollEvents();
	}
	/* Cleanup OpenGL data structures */
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ibo);
	glDeleteTextures(1, &trail_map);
	glDeleteTextures(1, &blur_trail_map);
	glDeleteBuffers(1, &ssbo);
	glfwTerminate();
	return 0;
}

void Print_Group_Values()
{
	/* Max work group size getting */
	int work_grp_cnt[3];
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
	std::cout << "Max global work group counts x: " << work_grp_cnt[0] << std::endl;
	std::cout << "Max global work group counts y: " << work_grp_cnt[1] << std::endl;
	std::cout << "Max global work group counts z: " << work_grp_cnt[2] << std::endl;
	// All output 65535

	int work_grp_invocations;
	glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_invocations);
	std::cout << "Max invocations: " << work_grp_invocations << std::endl;
	// Output 1024
}