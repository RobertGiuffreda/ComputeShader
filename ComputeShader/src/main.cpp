#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm.hpp>

int main(void)
{
	GLFWwindow* window;

	/* Initialize the Library*/
	if (!glfwInit())
		return -1;

	/* Create Windowed mode window and its OpenGL context */
	window = glfwCreateWindow(1080, 720, "Hello World!", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make windows context current */
	glfwMakeContextCurrent(window);

	/* Loop until user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);
		
		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events*/
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}