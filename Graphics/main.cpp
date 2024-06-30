#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include <iostream>
#include "classes.h"
#include "helper.h"
#include "OBJ_Loader.h"

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
GLuint VBO, VAO, EBO;

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

struct Object
{
    
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
    if(renderHeight>0)
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

objl::Loader loader;

// Camera parameters
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 10.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400, lastY = 300;
float fov = 45.0f;
bool leftMouseButtonPressed = false;
float cameraSpeed = 2.5f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Containers for multiple models
std::vector<unsigned int> VAOs, VBOs, EBOs;
std::vector<objl::Mesh> meshes;
std::vector<float> scales;
std::vector<AABB> aabbs;

AABB computeAABB(const objl::Mesh& mesh) {
    glm::vec3 min(FLT_MAX, FLT_MAX, FLT_MAX);
    glm::vec3 max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    for (const auto& vertex : mesh.Vertices) {
        if (vertex.Position.X < min.x) min.x = vertex.Position.X;
        if (vertex.Position.Y < min.y) min.y = vertex.Position.Y;
        if (vertex.Position.Z < min.z) min.z = vertex.Position.Z;

        if (vertex.Position.X > max.x) max.x = vertex.Position.X;
        if (vertex.Position.Y > max.y) max.y = vertex.Position.Y;
        if (vertex.Position.Z > max.z) max.z = vertex.Position.Z;
    }

    return { min, max };
}

void createAABBVAO(AABB& aabb) {
    glm::vec3 min = aabb.min;
    glm::vec3 max = aabb.max;

    float vertices[] = {
        // Positions
        min.x, min.y, min.z,  // 0
        max.x, min.y, min.z,  // 1
        max.x, max.y, min.z,  // 2
        min.x, max.y, min.z,  // 3
        min.x, min.y, max.z,  // 4
        max.x, min.y, max.z,  // 5
        max.x, max.y, max.z,  // 6
        min.x, max.y, max.z   // 7
    };

    unsigned int indices[] = {
        // Front face
        0, 1, 1, 2, 2, 3, 3, 0,
        // Back face
        4, 5, 5, 6, 6, 7, 7, 4,
        // Connecting edges
        0, 4, 1, 5, 2, 6, 3, 7
    };

    glGenVertexArrays(1, &aabb.VAO);
    glGenBuffers(1, &aabb.VBO);

    glBindVertexArray(aabb.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, aabb.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glGenBuffers(1, &aabb.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, aabb.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void loadModel(const std::string& path, float scale) {
    objl::Loader loader;
    if (loader.LoadFile(path)) {
        for (auto& mesh : loader.LoadedMeshes) {
            meshes.push_back(mesh);
            unsigned int VAO, VBO, EBO;

            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, mesh.Vertices.size() * sizeof(objl::Vertex), &mesh.Vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.Indices.size() * sizeof(unsigned int), &mesh.Indices[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);

            VAOs.push_back(VAO);
            VBOs.push_back(VBO);
            EBOs.push_back(EBO);
            scales.push_back(scale);

            // Compute and store AABB
            AABB aabb = computeAABB(mesh);
            createAABBVAO(aabb);
            aabbs.push_back(aabb);
        }
    }
}

void loadModelsFromDirectory(const std::string& directory, float scale) {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() == ".obj") {
            std::cout << "Loading " << entry.path().string() << std::endl;
            loadModel(entry.path().string(), scale);
        }
    }
}

// Mouse callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (leftMouseButtonPressed) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
}

// Mouse button callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            leftMouseButtonPressed = true;
        }
        else if (action == GLFW_RELEASE) {
            leftMouseButtonPressed = false;
            firstMouse = true;
        }
    }
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    float speed = cameraSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
}

void assignment1()
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
    ImGui::SliderFloat3("Position##sphere1", glm::value_ptr(sphere1.position), -3.f, 3.f);
    ImGui::SliderFloat("Radius##sphere1", &sphere1.radius, 0.1f, 3.f);

    ImGui::Text("Sphere 2:");
    ImGui::SliderFloat3("Position##sphere2", glm::value_ptr(sphere2.position), -3.f, 3.f);
    ImGui::SliderFloat("Radius##sphere2", &sphere2.radius, 0.1f, 3.f);

    ImGui::Text("Point:");
    ImGui::SliderFloat3("Coordinates", glm::value_ptr(point.coordinates), -3.f, 3.f);

    ImGui::Text("AABB 1:");
    ImGui::SliderFloat3("Box 1 Center", glm::value_ptr(aabb1.center), -3.f, 3.f);
    ImGui::SliderFloat3("Box 1 Half Extents", glm::value_ptr(aabb1.halfExtents), -3.f, 3.f);

    ImGui::Text("AABB 2:");
    ImGui::SliderFloat3("Box 2 Center", glm::value_ptr(aabb2.center), -3.f, 3.f);
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
    if (mainRenderHeight > 0)
        projection = glm::perspective(glm::radians(45.0f), static_cast<float>(mainRenderWidth) / static_cast<float>(mainRenderHeight), 0.1f, 100.0f);

    // Set the projection and view matrix uniforms in the shader
    glUseProgram(shaderProgram);
    GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    glViewport(0, 0, mainRenderWidth, mainRenderHeight);
    glPointSize(5.f);

    // Rendering logic based on selected test case
    switch (currentTestCase)
    {
    case SPHERE_VS_SPHERE:
        SphereVsSphere(sphere1, sphere2);
        break;
    case AABB_VS_SPHERE:
        AABBVsSphere(aabb1, sphere1);
        break;
    case SPHERE_VS_AABB:
        SphereVsAABB(sphere1, aabb1);
        break;
    case AABB_VS_AABB:
        AABBvsAABB(aabb1, aabb2);
        break;
    case POINT_VS_SPHERE:
        PointVsSphere(point, sphere1);
        break;
    case POINT_VS_AABB:
        PointVsAABB(point, aabb1);
        break;
    case POINT_VS_PLANE:
        PointVsPlane(point, plane);
        break;
    case POINT_VS_TRIANGLE:
        PointVsTriangle(point, triangle);
        break;
    case PLANE_VS_AABB:
        PlaneVsAABB(plane, aabb1);
        break;
    case PLANE_VS_SPHERE:
        PlaneVsSphere(plane, sphere1);
        break;
    case RAY_VS_PLANE:
        RayVsPlane(ray, plane);
        break;
    case RAY_VS_TRIANGLE:
        RayVsTriangle(ray, triangle);
        break;
    case RAY_VS_AABB:
        RayVsAABB(ray, aabb1);
        break;
    case RAY_VS_SPHERE:
        RayVsSphere(ray, sphere1);
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

int main()
{
    const unsigned int SCR_WIDTH = 1920;
    const unsigned int SCR_HEIGHT = 1080;

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL OBJ Renderer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Set viewport
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    shader();

    // Load all models from the specified directory with a specific scale
    loadModelsFromDirectory("../Assets/power4/part_a", 0.0001f);
   /* loadModelsFromDirectory("../Assets/power4/part_b", 0.0001f);
    loadModelsFromDirectory("../Assets/power5/part_a", 0.0001f);
    loadModelsFromDirectory("../Assets/power5/part_b", 0.0001f);
    loadModelsFromDirectory("../Assets/power5/part_c", 0.0001f);
    loadModelsFromDirectory("../Assets/power6/part_a", 0.0001f);
    loadModelsFromDirectory("../Assets/power6/part_b", 0.0001f);*/

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Process input
        processInput(window);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader program
        glUseProgram(shaderProgram);

        // Set transformations and uniforms
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Set light and view positions
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), -30.f, 50.0f, 10.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 0.5f, 0.31f);

        // Draw each loaded model with its corresponding scale
        for (size_t i = 0; i < VAOs.size(); ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(scales[i], scales[i], scales[i]));
            int modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(VAOs[i]);
            glDrawElements(GL_TRIANGLES, meshes[i].Indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // Render the AABB
            glBindVertexArray(aabbs[i].VAO);
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    for (size_t i = 0; i < VAOs.size(); ++i) {
        glDeleteVertexArrays(1, &VAOs[i]);
        glDeleteBuffers(1, &VBOs[i]);
        glDeleteBuffers(1, &EBOs[i]);
    }
    glDeleteProgram(shaderProgram);

    // Terminate GLFW
    glfwTerminate();
    return 0;
}