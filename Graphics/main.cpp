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
    SPHERE_VS_AABB,
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
    Point point{glm::vec3(0.f,0.f,0.f)};
    
    Sphere sphere1{ glm::vec3(0.f,0.f, 0.f),0.5f };
    Sphere sphere2{ glm::vec3(0.f,0.f, 0.f),0.5f };

    AABB aabb1{ glm::vec3(0.0f, 0.0f, 0.0f) ,glm::vec3(0.5f, 0.5f, 0.5f) };
    AABB aabb2{ glm::vec3(0.0f, 0.0f, 0.0f) ,glm::vec3(0.5f, 0.5f, 0.5f) };

    Plane plane{ glm::vec4(0.0f, 5.0f, 1.0f, 0.0f) };

    Triangle triangle{
        glm::vec3(-0.5f, -0.5f, 0.f),
        glm::vec3(0.5f, -0.5f, 0.f),
        glm::vec3(0.0f, 0.5f, 0.f)
    };
   
    TestCase currentTestCase = PLANE_VS_SPHERE;
    const char* testCaseNames[] = {
        "Sphere vs Sphere",
        "AABB vs Sphere",
        "Sphere vs AABB",
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
        ImGui::InputFloat3("Position##sphere1", glm::value_ptr(sphere1.position));
        ImGui::InputFloat("Radius##sphere1", &sphere1.radius);

        ImGui::Text("Sphere 2:");
        ImGui::InputFloat3("Position##sphere2", glm::value_ptr(sphere2.position));
        ImGui::InputFloat("Radius##sphere2", &sphere2.radius);

        ImGui::Text("Point:");
        ImGui::InputFloat3("Coordinates", glm::value_ptr(point.coordinates));

        ImGui::Text("AABB 1:");
        ImGui::InputFloat3("Box 1 Center", glm::value_ptr(aabb1.center));
        ImGui::InputFloat3("Box 1 Half Extents", glm::value_ptr(aabb1.halfExtents));

        ImGui::Text("AABB 2:");
        ImGui::InputFloat3("Box 2 Center", glm::value_ptr(aabb2.center));
        ImGui::InputFloat3("Box 2 Half Extents", glm::value_ptr(aabb2.halfExtents));

        ImGui::Text("Plane:");
        ImGui::InputFloat4("Normal", glm::value_ptr(plane.normal));

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
            SphereVsSphere(window, sphere1, sphere2);
            break;
        case AABB_VS_SPHERE:
            AABBVsSphere(window, aabb1, sphere1);
            break;
        case SPHERE_VS_AABB:
            SphereVsAABB(window, sphere1, aabb1);
            break;
        case AABB_VS_AABB:
            AABBvsAABB(window, aabb1,aabb2);
            break;
        case POINT_VS_SPHERE:
            PointVsSphere(window, point, sphere1);
            break;
        case POINT_VS_AABB:
            PointVsAABB(window, point, aabb1);
            break;
        case POINT_VS_PLANE:
            PointVsPlane(window, point, plane);
            break;
        case POINT_VS_TRIANGLE:
            PointVsTriangle(window, point, triangle);
            break;
        case PLANE_VS_AABB:
            PlaneVsAABB(window, plane, aabb1);
            break;
        case PLANE_VS_SPHERE:
            PlaneVsSphere(window, plane, sphere1);
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

