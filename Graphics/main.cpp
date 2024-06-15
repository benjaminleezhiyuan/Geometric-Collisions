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
    RAY_VS_PLANE,
    RAY_VS_TRIANGLE,
    RAY_VS_AABB,
    RAY_VS_SPHERE
};

TestCase currentTestCase;

GLFWwindow* window;

GLuint shaderProgram;

bool animate = true;

float rayLength{ 2.f };

//Pip stuff
glm::mat4 view,projection;
GLuint fbo;
GLuint fboTexture;
GLuint rbo;
const unsigned int FBO_WIDTH = 1920;  // Width of the FBO texture
const unsigned int FBO_HEIGHT = 1080; // Height of the FBO texture
float renderWidth, renderHeight;

glm::vec3 eye{ 0.0f, 5.0f, 0.0f }, target{ 0.0f, 0.0f, 0.0f }, up{ 0.0f, 0.0f, -1.0f };


// Set up the scene objects
Point point{ glm::vec3(0.f,0.f,0.f) };

Sphere sphere1{ glm::vec3(0.f,0.f, 0.f),0.5f };
Sphere sphere2{ glm::vec3(0.f,0.f, 0.f),0.5f };

AABB aabb1{ glm::vec3(0.0f, 0.0f, 0.0f) ,glm::vec3(0.5f, 0.5f, 0.5f) };
AABB aabb2{ glm::vec3(0.0f, 0.0f, 0.0f) ,glm::vec3(0.5f, 0.5f, 0.5f) };

Plane plane{ glm::vec4(0.0f, 1.0f, 0.2f, 0.0f) };

Triangle triangle{
    glm::vec3(-0.5f, -0.5f, 0.f),
    glm::vec3(0.5f, -0.5f, 0.f),
    glm::vec3(0.0f, 0.5f, 0.f)
};

Ray ray{
    glm::vec3(-1.2f, 0.5f, 0.5f), // start position
    glm::vec3(1.0f, -0.5f, -0.4f)  // direction
};

void setupFBO()
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create a texture to render the scene into
    glGenTextures(1, &fboTexture);
    glBindTexture(GL_TEXTURE_2D, fboTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, FBO_WIDTH, FBO_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Attach the texture to the framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

    // Create a renderbuffer object for depth and stencil attachment
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, FBO_WIDTH, FBO_HEIGHT);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // Attach the renderbuffer object to the framebuffer
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void renderToFBO()
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, FBO_WIDTH, FBO_HEIGHT);

    // Clear the color and depth buffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up a different camera position
    projection = glm::perspective(glm::radians(45.0f), (float)renderWidth / (float)renderHeight, 0.1f, 100.0f);
    view = glm::lookAt(eye, target,up);

    // Set the projection and view matrix uniforms in the shader
    glUseProgram(shaderProgram);
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // Rendering logic based on selected test case
    switch (currentTestCase)
    {
    case SPHERE_VS_SPHERE:
        SphereVsSphere(sphere1, sphere2);
        break;
    case AABB_VS_SPHERE:
        AABBVsSphere( aabb1, sphere1);
        break;
    case SPHERE_VS_AABB:
        SphereVsAABB( sphere1, aabb1);
        break;
    case AABB_VS_AABB:
        AABBvsAABB( aabb1, aabb2);
        break;
    case POINT_VS_SPHERE:
        PointVsSphere( point, sphere1);
        break;
    case POINT_VS_AABB:
        PointVsAABB( point, aabb1);
        break;
    case POINT_VS_PLANE:
        PointVsPlane( point, plane);
        break;
    case POINT_VS_TRIANGLE:
        PointVsTriangle( point, triangle);
        break;
    case PLANE_VS_AABB:
        PlaneVsAABB( plane, aabb1);
        break;
    case PLANE_VS_SPHERE:
        PlaneVsSphere( plane, sphere1);
        break;
    case RAY_VS_PLANE:
        RayVsPlane( ray, plane);
        break;
    case RAY_VS_TRIANGLE:
        RayVsTriangle( ray, triangle);
        break;
    case RAY_VS_AABB:
        RayVsAABB( ray, aabb1);
        break;
    case RAY_VS_SPHERE:
        RayVsSphere( ray, sphere1);
        break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int main()
{
    // Window settings
    const unsigned int SCR_WIDTH = 1920;
    const unsigned int SCR_HEIGHT = 1080;

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
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Spheres Intersection", NULL, NULL);
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
    rungenerateSphere();

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    currentTestCase = SPHERE_VS_SPHERE;
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
        "Plane vs Sphere",
        "Ray vs Plane",
        "Ray vs Triangle",
        "Ray vs AABB",
        "Ray vs Sphere"
    };

    setupFBO();

    // Main render loop
    while (!glfwWindowShouldClose(window))
    {
        // Input
        processInput(window);

        // Get the size of the main window's framebuffer
        int mainRenderWidth, mainRenderHeight;
        glfwGetFramebufferSize(window, &mainRenderWidth, &mainRenderHeight);

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui UI
        ImGui::Begin("Test Cases");
        ImGui::Text("Select a test case:");
        ImGui::Combo("Test Cases", (int*)&currentTestCase, testCaseNames, IM_ARRAYSIZE(testCaseNames));

        // Place the checkbox on the same line as the combo box
        ImGui::Checkbox("Show Animation", &animate);

        ImGui::Separator();
        ImGui::Text("Object Properties:");

        ImGui::Text("Sphere 1:");
        ImGui::SliderFloat3("Position##sphere1", glm::value_ptr(sphere1.position),-3.f,3.f);
        ImGui::SliderFloat("Radius##sphere1", &sphere1.radius,0.1f,3.f);

        ImGui::Text("Sphere 2:");
        ImGui::SliderFloat3("Position##sphere2", glm::value_ptr(sphere2.position),-3.f,3.f);
        ImGui::SliderFloat("Radius##sphere2", &sphere2.radius,0.1f,3.f);

        ImGui::Text("Point:");
        ImGui::SliderFloat3("Coordinates", glm::value_ptr(point.coordinates),-3.f,3.f);

        ImGui::Text("AABB 1:");
        ImGui::SliderFloat3("Box 1 Center", glm::value_ptr(aabb1.center), - 3.f, 3.f);
        ImGui::SliderFloat3("Box 1 Half Extents", glm::value_ptr(aabb1.halfExtents), -3.f, 3.f);

        ImGui::Text("AABB 2:");
        ImGui::SliderFloat3("Box 2 Center", glm::value_ptr(aabb2.center), - 3.f, 3.f);
        ImGui::SliderFloat3("Box 2 Half Extents", glm::value_ptr(aabb2.halfExtents), -3.f, 3.f);

        ImGui::Text("Triangle:");
        ImGui::SliderFloat3("Vertex 1", glm::value_ptr(triangle.v1), -3.f, 3.f);
        ImGui::SliderFloat3("Vertex 2", glm::value_ptr(triangle.v2), -3.f, 3.f);
        ImGui::SliderFloat3("Vertex 3", glm::value_ptr(triangle.v3), -3.f, 3.f);

        ImGui::Text("Ray:");
        ImGui::SliderFloat3("Start Position", glm::value_ptr(ray.start), -5.f, 5.f);
        ImGui::SliderFloat3("Direction", glm::value_ptr(ray.direction), -1.f, 1.f);
        ImGui::SliderFloat("Ray Length", &rayLength, 0.1f, 10.f);

        ImGui::End();

        // Create a draggable ImGui window to display the FBO texture
        ImGui::Begin("FBO Render");
        ImGui::Text("Render from a different camera position");
        ImGui::Text("Second Camera");
        ImGui::SliderFloat3("Eye", glm::value_ptr(eye), -5.0f, 5.0f);
        ImGui::SliderFloat3("Target", glm::value_ptr(target), -5.0f, 5.0f);
        ImGui::SliderFloat3("Up", glm::value_ptr(up), -5.0f, 5.0f);

        // Get the size of the ImGui window
        ImVec2 imguiWindowSize = ImGui::GetContentRegionAvail();
        float imguiWindowWidth = imguiWindowSize.x;
        float imguiWindowHeight = imguiWindowSize.y;

        // Calculate the aspect ratio of the texture
        float aspectRatio = (float)FBO_WIDTH / (float)FBO_HEIGHT;

        // Calculate the appropriate width and height to maintain the aspect ratio
        if (imguiWindowWidth / imguiWindowHeight > aspectRatio)
        {
            renderWidth = imguiWindowHeight * aspectRatio;
            renderHeight = imguiWindowHeight;
        }
        else
        {
            renderWidth = imguiWindowWidth;
            renderHeight = imguiWindowWidth / aspectRatio;
        }

        // Render to ImGui window using the calculated size
        ImGui::Image((void*)(intptr_t)fboTexture, ImVec2(renderWidth, renderHeight));
        ImGui::End();

        renderToFBO();

        // Clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_PROGRAM_POINT_SIZE);

        view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        projection = glm::perspective(glm::radians(45.0f), static_cast<float>(mainRenderWidth) / static_cast<float>(mainRenderHeight), 0.1f, 100.0f);

        glViewport(0, 0, mainRenderWidth, mainRenderHeight);
        glPointSize(5.f);
        // Rendering logic based on selected test case
        switch (currentTestCase)
        {
        case SPHERE_VS_SPHERE:
            SphereVsSphere(sphere1, sphere2);
            break;
        case AABB_VS_SPHERE:
            AABBVsSphere( aabb1, sphere1);
            break;
        case SPHERE_VS_AABB:
            SphereVsAABB( sphere1, aabb1);
            break;
        case AABB_VS_AABB:
            AABBvsAABB( aabb1,aabb2);
            break;
        case POINT_VS_SPHERE:
            PointVsSphere( point, sphere1);
            break;
        case POINT_VS_AABB:
            PointVsAABB( point, aabb1);
            break;
        case POINT_VS_PLANE:
            PointVsPlane( point, plane);
            break;
        case POINT_VS_TRIANGLE:
            PointVsTriangle( point, triangle);
            break;
        case PLANE_VS_AABB:
            PlaneVsAABB( plane, aabb1);
            break;
        case PLANE_VS_SPHERE:
            PlaneVsSphere( plane, sphere1);
            break;
        case RAY_VS_PLANE:
            RayVsPlane( ray, plane);
            break;
        case RAY_VS_TRIANGLE:
            RayVsTriangle( ray, triangle);
            break;
        case RAY_VS_AABB:
            RayVsAABB( ray, aabb1);
            break;
        case RAY_VS_SPHERE:
            RayVsSphere( ray, sphere1);
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

