#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include <iostream>
#include "classes.h"
#include "helper.h"

enum TestCase {
    SPHERE_VS_SPHERE,
    AABB_VS_SPHERE,
    AABB_VS_AABB,
    POINT_VS_SPHERE,
    POINT_VS_AABB,
    POINT_VS_PLANE,
    POINT_VS_TRIANGLE,
    PLANE_VS_AABB,
    PLANE_VS_SPHERE,
};

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

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Set up the scene objects
    glm::vec3 initialPointCoords(0.0f, 0.0f, -5.f);
    float sphere1Radius = 1.0f;
    glm::vec3 sphere1Position(0.0f, 0.0f, -5.0f);
    float sphere2Radius = 1.0f;
    glm::vec3 sphere2Position(0.0f, 0.0f, -5.0f);
    glm::vec3 initialBoxCenter(0.0f, 0.0f, -5.0f);
    glm::vec3 initialBoxHalfExtents(0.5f, 0.5f, 0.5f);
    glm::vec4 planeNormal(0.0f, 1.0f, 0.0f, 0.0f);
    float planeOffset = 0.0f;
    Triangle triangle = {
        glm::vec3(-1.0f, -1.0f, -5.f),
        glm::vec3(1.0f, -1.0f, -5.f),
        glm::vec3(0.0f, 1.0f, -5.f)
    };

    TestCase currentTestCase = SPHERE_VS_SPHERE;
    const char* testCaseNames[] = {
        "Sphere vs Sphere",
        "AABB vs Sphere",
        "AABB vs AABB",
        "Point vs Sphere",
        "Point vs AABB",
        "Point vs Plane",
        "Point vs Triangle",
        "Plane vs AABB",
        "Plane vs Sphere"
    };

    // Main render loop
    while (!glfwWindowShouldClose(window))
    {
        // Input
        processInput(window);

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui UI
        ImGui::Begin("Test Cases");
        ImGui::Text("Select a test case:");
        ImGui::Combo("Test Cases", (int*)&currentTestCase, testCaseNames, IM_ARRAYSIZE(testCaseNames));

        ImGui::Separator();
        ImGui::Text("Object Properties:");

        ImGui::Text("Sphere 1:");
        ImGui::InputFloat3("Position##sphere1", glm::value_ptr(sphere1Position));
        ImGui::InputFloat("Radius##sphere1", &sphere1Radius);

        ImGui::Text("Sphere 2:");
        ImGui::InputFloat3("Position##sphere2", glm::value_ptr(sphere2Position));
        ImGui::InputFloat("Radius##sphere2", &sphere2Radius);

        ImGui::Text("Point:");
        ImGui::InputFloat3("Coordinates", glm::value_ptr(initialPointCoords));

        ImGui::Text("AABB:");
        ImGui::InputFloat3("Box Center", glm::value_ptr(initialBoxCenter));
        ImGui::InputFloat3("Box Half Extents", glm::value_ptr(initialBoxHalfExtents));

        ImGui::Text("Plane:");
        ImGui::InputFloat4("Normal", glm::value_ptr(planeNormal));
        ImGui::InputFloat("Offset", &planeOffset);

        ImGui::Text("Triangle:");
        ImGui::InputFloat3("Vertex 1", glm::value_ptr(triangle.v1));
        ImGui::InputFloat3("Vertex 2", glm::value_ptr(triangle.v2));
        ImGui::InputFloat3("Vertex 3", glm::value_ptr(triangle.v3));

        ImGui::End();

        // Clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_PROGRAM_POINT_SIZE);

        // Rendering logic based on selected test case
        switch (currentTestCase)
        {
        case SPHERE_VS_SPHERE:
            SphereVsSphere(window, sphere1Radius, sphere2Radius, sphere1Position,sphere2Position);
            break;
        case AABB_VS_SPHERE:
            AABBvsSphere(window, sphere1Radius, initialBoxCenter, initialBoxHalfExtents);
            break;
        case AABB_VS_AABB:
            AABBvsAABB(window, initialBoxCenter, initialBoxHalfExtents, initialBoxCenter, initialBoxHalfExtents);
            break;
        case POINT_VS_SPHERE:
            PointVsSphere(window, initialPointCoords, sphere1Radius);
            break;
        case POINT_VS_AABB:
            PointVsAABB(window, initialPointCoords, initialBoxCenter, initialBoxHalfExtents);
            break;
        case POINT_VS_PLANE:
            PointVsPlane(window, initialPointCoords, planeNormal, planeOffset);
            break;
        case POINT_VS_TRIANGLE:
            PointVsTriangle(window, initialPointCoords, triangle);
            break;
        case PLANE_VS_AABB:
            PlaneVsAABB(window, planeNormal, planeOffset, initialBoxCenter, initialBoxHalfExtents);
            break;
        case PLANE_VS_SPHERE:
            PlaneVsSphere(window, planeNormal, planeOffset, sphere1Radius);
            break;
        }

        // Render ImGui
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup ImGui and terminate GLFW
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

