#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include "shaders/shader.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}  


void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}








int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  
    // Create window and error handling
    GLFWwindow* window = glfwCreateWindow(800, 600, "Visualizer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Do stuff in specified window
    glfwMakeContextCurrent(window);

    // load glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }    

    glViewport(0, 0, 800, 600);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);  

    while(!glfwWindowShouldClose(window))
    {
        processInput(window);
        glClearColor(25.0f/255.0f, 23.0f/255.0f, 36.0f/255.0f, 1.0f);

        glfwPollEvents();    
        glfwSwapBuffers(window);
    }
    return 0;
}
