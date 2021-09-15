#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm.hpp>
#include <iostream>

#include "Random.h"
#include "Shader.h"
#include "ComputeShader.h"

void Print_Group_Values();
void KeyboardHandle(GLFWwindow* window);

const float radius = 300.0f;

const float o_radius = 300.0f;
const float i_radius = 10.0f;

const float s_radius = 50.0f;

/* Sensor is in degrees */
const float SCR_WIDTH = 720.0f;
const float SCR_HEIGHT = 720.0f;

const float TEX_WIDTH = 720.0f;
const float TEX_HEIGHT = 720.0f;

float move_dist = 10.0f;
float sensor_angle = glm::radians(45.0f);
float sensor_dist = 50.0f;
float turn_speed = 0.3f;

/* Dissapation and decay */
float decay_rate = 0.4f;
float blur_factor = 1.0f;

/* Number of Particles */
const unsigned int PNUM = 2000000;

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
		glm::vec4 color;
		glm::vec2 pos;
		float dir;
		float pad;
	};

	unsigned int ssbo = 0;
	glGenBuffers(1, &ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(struct particle) * PNUM, nullptr, GL_DYNAMIC_COPY);

	/*
	* Map buffer to make to addressable by the cpu. Get the address of the buffer in memory 
	* Place the inital values of the particles in the buffer
	* Unmap the buffer before use again
	*/
	int buffmask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
	struct particle *particles = (struct particle *)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, PNUM * sizeof(struct particle), buffmask);
	for (int i = 0; i < PNUM; i++)
	{
		/* Color */
		particles[i].color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		/* Position and direction */
		float rad_diff = o_radius - i_radius;
		glm::vec2 third = glm::normalize(glm::vec2(standardNormal(), standardNormal()));
		float rad = (glm::pow(uniform(), (1.0f / 2.0f)) * rad_diff) + i_radius;
		third *= rad;
		/* Get direction to point towards center */
		float arcos = glm::acos(third.x/rad);
		if (third.y >= 0 && rad != 0) arcos += 0;
		else if (third.y < 0) arcos = -arcos;
		else if (rad == 0) arcos = 0;

		particles[i].dir = arcos + 3.141592;
		particles[i].pos = third + glm::vec2(TEX_WIDTH / 2, TEX_HEIGHT / 2);
		// particles[i].pos = glm::vec2(uniform() * TEX_WIDTH, uniform() * TEX_HEIGHT);
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
	shader.SetUniform1i("noise", 0);

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

		KeyboardHandle(window);

		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);

		/* Call particle update compute shader */
		p_update.Bind();
		p_update.SetUniform1f("width", TEX_WIDTH);
		p_update.SetUniform1f("height", TEX_HEIGHT);
		p_update.SetUniform1f("move_dist", move_dist);
		p_update.SetUniform1f("sensor_dist", sensor_dist);
		p_update.SetUniform1f("sensor_angle", sensor_angle);
		p_update.SetUniform1f("turn_speed", turn_speed);
		p_update.SetUniform1f("delta_time", delta_time);
		p_update.SetUniform1f("time", glfwGetTime());
		glDispatchCompute(PNUM, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

		/* Display Screen Sized Texture */
		shader.Bind();
		glBindVertexArray(vao);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		/* Call trail map update compute shader */
		decay.Bind();
		decay.SetUniform1f("delta_time", delta_time);
		glDispatchCompute((unsigned int)TEX_WIDTH, (unsigned int)TEX_HEIGHT, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		/* Copy blur_map to trail_map */
		copy.Bind();
		glDispatchCompute((unsigned int)TEX_WIDTH, (unsigned int)TEX_HEIGHT, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		
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

void KeyboardHandle(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		move_dist += 5.0f;
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		move_dist -= 5.0f;


	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		sensor_dist += 5.0f;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		sensor_dist -= 5.0f;

	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
		sensor_angle += 1.0f;
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		sensor_angle -= 1.0f;

	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		turn_speed += 0.2f;
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		turn_speed -= 0.2f;

	if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
		decay_rate += 0.01f;
	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		decay_rate -= 0.01f;

	if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
		blur_factor += 0.1f;
	if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
		blur_factor -= 0.1f;
}