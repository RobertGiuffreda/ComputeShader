#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm.hpp>
#include <iostream>

#include "Random.h"
#include "Shader.h"
#include "ComputeShader.h"

void Print_Group_Values();

const float SCR_WIDTH = 720.0f;
const float SCR_HEIGHT = 480.0f;

const float TEX_WIDTH = 240.0f;
const float TEX_HEIGHT = 160.0f;

const unsigned int PNUM = 512;

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

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initiate glad" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* Create the Particle map. Shader storage buffer object of particle positions and directions */
	struct particle {
		glm::vec2 pos;
		float direction;
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
	struct particle* particles = (struct particle*)glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, PNUM * sizeof(struct particle), buffmask);
	for (int i = 0; i < PNUM; i++)
	{
		particles[i].pos = glm::vec2(uniform() * SCR_WIDTH, uniform() * SCR_HEIGHT);
		particles[i].direction = uniform() * 360;
	}
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	/* Bind the buffer to binding = 1 so the compute shader can see it */
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	/* Create the Trailmap to be passed to the compute shader */
	unsigned int trail_map;
	glGenTextures(1, &trail_map);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, trail_map);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, trail_map, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	/* Create compute shader to zero the texture */
	ComputeShader compshader("res/shaders/zero_tex.shader");
	compshader.Bind();
	glDispatchCompute((unsigned int)SCR_WIDTH, (unsigned int)SCR_HEIGHT, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


	ComputeShader p_update("res/shaders/particle_update.shader");
	p_update.Bind();
	glDispatchCompute(PNUM, 1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	/* Create shader to display texture and sex uniform texture */
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
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);

		/* Display Screen Sized Texture */
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, trail_map);
		
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