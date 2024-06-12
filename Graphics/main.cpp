#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include "classes.h"
#include "helper.h"

int main()
{
    // Window settings
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Spheres Intersection", NULL, NULL);
    if (!window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Set the viewport
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    shader();

    // Set up the scene objects
    glm::vec3 initialPointCoords(0.0f, 0.0f, -5.f);
    float sphereRadius = 1.0f;
    glm::vec3 initialBoxCenter(0.0f, 0.0f, -5.0f);
    glm::vec3 initialBoxHalfExtents(0.5f, 0.5f, 0.5f);
    glm::vec4 planeNormal(0.0f, 1.0f, 0.0f, 0.0f);
    float planeOffset = 0.0f;
    Triangle triangle = {
        glm::vec3(-1.0f, -1.0f, -5.f),
        glm::vec3(1.0f, -1.0f, -5.f),
        glm::vec3(0.0f, 1.0f, -5.f)
    };

    while (!glfwWindowShouldClose(window))
    {
        // Allow larger point size
        glEnable(GL_PROGRAM_POINT_SIZE);
        // Input
        processInput(window);

        //SphereVsSphere(window, sphereRadius, sphereRadius);
        
        //AABBvsSphere(window, sphereRadius, initialBoxCenter, initialBoxHalfExtents);

        //AABBvsAABB(window, initialBoxCenter, initialBoxHalfExtents, initialBoxCenter, initialBoxHalfExtents);

        PointVsSphere(window, initialPointCoords, sphereRadius);

        //PointVsAABB(window, initialPointCoords, initialBoxCenter, initialBoxHalfExtents);
        
        //PointVsPlane(window, initialPointCoords, planeNormal, planeOffset);

        //PointVsTriangle(window, initialPointCoords, triangle);

    
    }

    // Cleanup and terminate GLFW
    glfwTerminate();
    return 0;
}

