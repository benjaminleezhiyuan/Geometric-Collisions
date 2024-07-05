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
#include <limits>
#include <cmath>

GLFWwindow* window;

GLuint shaderProgram;
GLuint bvShaderProgram;
GLuint VBO, VAO, EBO;

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
std::vector<GLuint> VAOs, VBOs, EBOs;
std::vector<objl::Mesh> meshes;
std::vector<float> scales;

// Bounding box variables
std::vector<GLuint> bboxVAOs, bboxVBOs;
std::vector<std::vector<glm::vec3>> bboxVertices;

// Bounding sphere variables
struct BoundingSphere {
    glm::vec3 center;
    float radius;
};
std::vector<BoundingSphere> boundingSpheresRitter, boundingSpheresLarsson, boundingSpheresPCA;
std::vector<unsigned int> sphereVAOs, sphereVBOs, sphereEBOs;
std::vector<std::vector<unsigned int>> ritterSphereIndices, larssonSphereIndices, pcaSphereIndices;

std::vector<unsigned int> ritterSphereVAOs, ritterSphereVBOs, ritterSphereEBOs;
std::vector<unsigned int> larssonSphereVAOs, larssonSphereVBOs, larssonSphereEBOs;
std::vector<unsigned int> pcaSphereVAOs, pcaSphereVBOs, pcaSphereEBOs;

enum BoundingVolumeType {
    NONE,
    BOUNDING_BOX,
    RITTER_SPHERE,
    LARSSON_SPHERE,
    PCA_SPHERE
};

BoundingVolumeType selectedBoundingVolumeType = NONE;

BoundingSphere ritterBoundingSphere(const std::vector<objl::Vertex>& vertices) {
    if (vertices.empty()) return { glm::vec3(0.0f), 0.0f };

    // Find the most distant points to form the initial sphere
    auto [minPoint, maxPoint] = std::minmax_element(vertices.begin(), vertices.end(), [](const objl::Vertex& a, const objl::Vertex& b) {
        return a.Position.X < b.Position.X;
        });

    glm::vec3 p1(minPoint->Position.X, minPoint->Position.Y, minPoint->Position.Z);
    glm::vec3 p2(maxPoint->Position.X, maxPoint->Position.Y, maxPoint->Position.Z);

    glm::vec3 center = (p1 + p2) * 0.5f;
    float radius = glm::distance(p1, p2) * 0.5f;

    // Grow the sphere to include all points
    for (const auto& vertex : vertices) {
        glm::vec3 point(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
        float dist = glm::distance(center, point);
        if (dist > radius) {
            float newRadius = (radius + dist) * 0.5f;
            float k = (newRadius - radius) / dist;
            radius = newRadius;
            center += k * (point - center);
        }
    }

    return { center, radius };
}

BoundingSphere larssonBoundingSphere(const std::vector<objl::Vertex>& vertices) {
    if (vertices.empty()) return { glm::vec3(0.0f), 0.0f };

    // Start with a simple bounding sphere
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 0.0f;

    for (const auto& vertex : vertices) {
        glm::vec3 point(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
        center += point;
    }
    center /= vertices.size();

    for (const auto& vertex : vertices) {
        glm::vec3 point(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
        float dist = glm::distance(center, point);
        radius = std::max(radius, dist);
    }

    // Modified Larsson's method for refining the sphere
    for (const auto& vertex : vertices) {
        glm::vec3 point(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
        float dist = glm::distance(center, point);
        if (dist > radius) {
            radius = (radius + dist) / 2.0f;
            center = center + (point - center) * ((dist - radius) / dist);
        }
    }

    return { center, radius };
}

// Helper function to multiply a matrix by a vector
glm::vec3 multiplyMatrixVector(const glm::mat3& mat, const glm::vec3& vec) {
    return glm::vec3(
        mat[0][0] * vec.x + mat[1][0] * vec.y + mat[2][0] * vec.z,
        mat[0][1] * vec.x + mat[1][1] * vec.y + mat[2][1] * vec.z,
        mat[0][2] * vec.x + mat[1][2] * vec.y + mat[2][2] * vec.z
    );
}

// Helper function to normalize a vector
glm::vec3 normalize(const glm::vec3& vec) {
    float len = glm::length(vec);
    return len > std::numeric_limits<float>::epsilon() ? vec / len : glm::vec3(0.0f);
}

// Function to calculate the dominant eigenvector using power iteration
glm::vec3 powerIteration(const glm::mat3& matrix, int iterations = 100) {
    glm::vec3 b_k(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < iterations; ++i) {
        b_k = multiplyMatrixVector(matrix, b_k);
        b_k = normalize(b_k);
    }
    return b_k;
}

// Function to calculate the eigenvalues and eigenvectors
void eigenDecomposition(const glm::mat3& matrix, glm::vec3& eigenvalues, glm::mat3& eigenvectors) {
    glm::vec3 dominantEigenvector = powerIteration(matrix);
    eigenvectors[0] = dominantEigenvector;
    // Calculate the remaining eigenvalues and eigenvectors (simplified for 3x3 matrices)
    glm::mat3 I = glm::mat3(1.0f);
    glm::mat3 A = matrix - glm::outerProduct(dominantEigenvector, dominantEigenvector) * glm::dot(dominantEigenvector, multiplyMatrixVector(matrix, dominantEigenvector));
    eigenvectors[1] = powerIteration(A);
    eigenvectors[2] = glm::cross(eigenvectors[0], eigenvectors[1]);

    eigenvalues = glm::vec3(
        glm::dot(dominantEigenvector, multiplyMatrixVector(matrix, dominantEigenvector)),
        glm::dot(eigenvectors[1], multiplyMatrixVector(matrix, eigenvectors[1])),
        glm::dot(eigenvectors[2], multiplyMatrixVector(matrix, eigenvectors[2]))
    );
}

BoundingSphere pcaBoundingSphere(const std::vector<objl::Vertex>& vertices) {
    if (vertices.empty()) return { glm::vec3(0.0f), 0.0f };

    // Calculate the mean (centroid) of the vertices
    glm::vec3 centroid(0.0f);
    for (const auto& vertex : vertices) {
        centroid += glm::vec3(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
    }
    centroid /= static_cast<float>(vertices.size());

    // Calculate the covariance matrix
    glm::mat3 covarianceMatrix(0.0f);
    for (const auto& vertex : vertices) {
        glm::vec3 pos(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
        glm::vec3 diff = pos - centroid;
        covarianceMatrix += glm::outerProduct(diff, diff);
    }
    covarianceMatrix /= static_cast<float>(vertices.size());

    // Perform eigen decomposition on the covariance matrix
    glm::vec3 eigenvalues;
    glm::mat3 eigenvectors;
    eigenDecomposition(covarianceMatrix, eigenvalues, eigenvectors);

    // Find the maximum extent along the principal components
    float maxExtent = 0.0f;
    for (const auto& vertex : vertices) {
        glm::vec3 pos(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
        glm::vec3 diff = pos - centroid;
        float extent = glm::dot(diff, eigenvectors[0]);
        maxExtent = std::max(maxExtent, extent);
    }

    return { centroid, maxExtent };
}

void generateSphereVertices(const glm::vec3& center, float radius, int sectors, int stacks, std::vector<glm::vec3>& vertices, std::vector<unsigned int>& indices) {
    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius;    // vertex normal
    float s, t;                                     // vertex texCoord

    float sectorStep = 2 * glm::pi<float>() / sectors;
    float stackStep = glm::pi<float>() / stacks;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stacks; ++i) {
        stackAngle = glm::pi<float>() / 2 - i * stackStep;        // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);                           // r * cos(u)
        z = radius * sinf(stackAngle);                            // r * sin(u)

        for (int j = 0; j <= sectors; ++j) {
            sectorAngle = j * sectorStep;                         // starting from 0 to 2pi

            x = xy * cosf(sectorAngle);                           // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);                           // r * cos(u) * sin(v)
            vertices.push_back(center + glm::vec3(x, y, z));
        }
    }

    // indices
    int k1, k2;
    for (int i = 0; i < stacks; ++i) {
        k1 = i * (sectors + 1);     // beginning of current stack
        k2 = k1 + sectors + 1;      // beginning of next stack

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
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

            // Calculate bounding box
            glm::vec3 min(FLT_MAX, FLT_MAX, FLT_MAX);
            glm::vec3 max(FLT_MIN, FLT_MIN, FLT_MIN);
            for (const auto& vertex : mesh.Vertices) {
                min = glm::min(min, glm::vec3(vertex.Position.X, vertex.Position.Y, vertex.Position.Z));
                max = glm::max(max, glm::vec3(vertex.Position.X, vertex.Position.Y, vertex.Position.Z));
            }

            std::vector<glm::vec3> bboxVertices = {
                min,
                glm::vec3(max.x, min.y, min.z),
                glm::vec3(max.x, max.y, min.z),
                glm::vec3(min.x, max.y, min.z),
                glm::vec3(min.x, min.y, max.z),
                glm::vec3(max.x, min.y, max.z),
                glm::vec3(max.x, max.y, max.z),
                glm::vec3(min.x, max.y, max.z)
            };

            std::vector<unsigned int> bboxIndices = {
                0, 1, 1, 2, 2, 3, 3, 0,
                4, 5, 5, 6, 6, 7, 7, 4,
                0, 4, 1, 5, 2, 6, 3, 7
            };

            unsigned int bboxVAO, bboxVBO, bboxEBO;
            glGenVertexArrays(1, &bboxVAO);
            glGenBuffers(1, &bboxVBO);
            glGenBuffers(1, &bboxEBO);

            glBindVertexArray(bboxVAO);

            glBindBuffer(GL_ARRAY_BUFFER, bboxVBO);
            glBufferData(GL_ARRAY_BUFFER, bboxVertices.size() * sizeof(glm::vec3), &bboxVertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bboxEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, bboxIndices.size() * sizeof(unsigned int), &bboxIndices[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);

            bboxVAOs.push_back(bboxVAO);
            bboxVBOs.push_back(bboxVBO);

            // Calculate bounding spheres using both methods
            BoundingSphere ritterSphere = ritterBoundingSphere(mesh.Vertices);
            BoundingSphere larssonSphere = larssonBoundingSphere(mesh.Vertices);
            BoundingSphere pcaSphere = pcaBoundingSphere(mesh.Vertices);


            boundingSpheresRitter.push_back(ritterSphere);
            boundingSpheresLarsson.push_back(larssonSphere);
            boundingSpheresPCA.push_back(pcaSphere);


            // Create Ritter sphere vertices
            std::vector<glm::vec3> ritterSphereVerts;
            std::vector<unsigned int> ritterSphereIdx;
            generateSphereVertices(ritterSphere.center, ritterSphere.radius, 36, 18, ritterSphereVerts, ritterSphereIdx);
            ritterSphereIndices.push_back(ritterSphereIdx);

            unsigned int ritterSphereVAO, ritterSphereVBO, ritterSphereEBO;
            glGenVertexArrays(1, &ritterSphereVAO);
            glGenBuffers(1, &ritterSphereVBO);
            glGenBuffers(1, &ritterSphereEBO);

            glBindVertexArray(ritterSphereVAO);

            glBindBuffer(GL_ARRAY_BUFFER, ritterSphereVBO);
            glBufferData(GL_ARRAY_BUFFER, ritterSphereVerts.size() * sizeof(glm::vec3), &ritterSphereVerts[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ritterSphereEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, ritterSphereIdx.size() * sizeof(unsigned int), &ritterSphereIdx[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);

            ritterSphereVAOs.push_back(ritterSphereVAO);
            ritterSphereVBOs.push_back(ritterSphereVBO);
            ritterSphereEBOs.push_back(ritterSphereEBO);

            // Create Larsson sphere vertices
            std::vector<glm::vec3> larssonSphereVerts;
            std::vector<unsigned int> larssonSphereIdx;
            generateSphereVertices(larssonSphere.center, larssonSphere.radius, 36, 18, larssonSphereVerts, larssonSphereIdx);
            larssonSphereIndices.push_back(larssonSphereIdx);

            unsigned int larssonSphereVAO, larssonSphereVBO, larssonSphereEBO;
            glGenVertexArrays(1, &larssonSphereVAO);
            glGenBuffers(1, &larssonSphereVBO);
            glGenBuffers(1, &larssonSphereEBO);

            glBindVertexArray(larssonSphereVAO);

            glBindBuffer(GL_ARRAY_BUFFER, larssonSphereVBO);
            glBufferData(GL_ARRAY_BUFFER, larssonSphereVerts.size() * sizeof(glm::vec3), &larssonSphereVerts[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, larssonSphereEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, larssonSphereIdx.size() * sizeof(unsigned int), &larssonSphereIdx[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);

            larssonSphereVAOs.push_back(larssonSphereVAO);
            larssonSphereVBOs.push_back(larssonSphereVBO);
            larssonSphereEBOs.push_back(larssonSphereEBO);

            // Create PCA sphere vertices
            std::vector<glm::vec3> pcaSphereVerts;
            std::vector<unsigned int> pcaSphereIdx;
            generateSphereVertices(pcaSphere.center, pcaSphere.radius, 36, 18, pcaSphereVerts, pcaSphereIdx);
            pcaSphereIndices.push_back(pcaSphereIdx);

            unsigned int pcaSphereVAO, pcaSphereVBO, pcaSphereEBO;
            glGenVertexArrays(1, &pcaSphereVAO);
            glGenBuffers(1, &pcaSphereVBO);
            glGenBuffers(1, &pcaSphereEBO);

            glBindVertexArray(pcaSphereVAO);

            glBindBuffer(GL_ARRAY_BUFFER, pcaSphereVBO);
            glBufferData(GL_ARRAY_BUFFER, pcaSphereVerts.size() * sizeof(glm::vec3), &pcaSphereVerts[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pcaSphereEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, pcaSphereIdx.size() * sizeof(unsigned int), &pcaSphereIdx[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glBindVertexArray(0);

            pcaSphereVAOs.push_back(pcaSphereVAO);
            pcaSphereVBOs.push_back(pcaSphereVBO);
            pcaSphereEBOs.push_back(pcaSphereEBO);
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

int main() {
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

    // Load all models from the specified directory with a specific scale
    loadModelsFromDirectory("../Assets/power4/part_a", 0.0001f);
    loadModelsFromDirectory("../Assets/power4/part_b", 0.0001f);
    loadModelsFromDirectory("../Assets/power5/part_a", 0.0001f);
    loadModelsFromDirectory("../Assets/power5/part_b", 0.0001f);
    loadModelsFromDirectory("../Assets/power5/part_c", 0.0001f);
    loadModelsFromDirectory("../Assets/power6/part_a", 0.0001f);
    loadModelsFromDirectory("../Assets/power6/part_b", 0.0001f);


    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    shader();
    bvShader();

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool showBoundingBox{ false }, showRitterSphere{ false }, showLarssonSphere{ false }, showPCASphere{ false };
     
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Process input
        processInput(window);

        glClearColor(1.f, 1.f, 1.f, 1.f);
        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use shader program
        glUseProgram(shaderProgram);

        // Set view and projection matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Set light and view positions
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), -30.f, 50.0f, 20.0f);
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
        }

        // Available bounding volume options
        const char* boundingVolumeOptions[] = { "None", "Bounding Box", "Ritter Sphere", "Larsson Sphere", "PCA Sphere" };

        // Start the ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create ImGui controls
        ImGui::Begin("Bounding Volume Controls");
        ImGui::Combo("Bounding Volume Type", reinterpret_cast<int*>(&selectedBoundingVolumeType), boundingVolumeOptions, IM_ARRAYSIZE(boundingVolumeOptions));
        ImGui::End();

        // Use bounding volume shader program
        glUseProgram(bvShaderProgram);

        // Set view and projection matrices for bounding volumes
        glUniformMatrix4fv(glGetUniformLocation(bvShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(bvShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Set the color for bounding volumes
        glm::vec3 bboxColor(0.0f, 0.0f, 1.0f); // Blue for bounding box
        glm::vec3 ritterSphereColor(1.0f, 0.0f, 0.0f); // Red for Ritter sphere
        glm::vec3 larssonSphereColor(0.0f, 1.0f, 0.0f); // Green for Larsson sphere
        glm::vec3 pcaSphereColor(1.0f, 1.0f, 0.0f); // Yellow for PCA sphere

        // Render bounding volumes based on selection
        switch (selectedBoundingVolumeType) {
        case BOUNDING_BOX:
            glUniform3f(glGetUniformLocation(bvShaderProgram, "boundingVolumeColor"), bboxColor.r, bboxColor.g, bboxColor.b); // Set bounding box color
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode
            for (size_t i = 0; i < bboxVAOs.size(); ++i) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::scale(model, glm::vec3(scales[i], scales[i], scales[i]));
                int modelLoc = glGetUniformLocation(bvShaderProgram, "model");
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                glBindVertexArray(bboxVAOs[i]);
                glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0); // 24 indices for the bounding box lines
                glBindVertexArray(0);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset to fill mode
            break;

        case RITTER_SPHERE:
            glUniform3f(glGetUniformLocation(bvShaderProgram, "boundingVolumeColor"), ritterSphereColor.r, ritterSphereColor.g, ritterSphereColor.b); // Set Ritter sphere color
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode
            for (size_t i = 0; i < ritterSphereVAOs.size(); ++i) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::scale(model, glm::vec3(scales[i], scales[i], scales[i]));
                int modelLoc = glGetUniformLocation(bvShaderProgram, "model");
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                glBindVertexArray(ritterSphereVAOs[i]);
                glDrawElements(GL_LINE_LOOP, ritterSphereIndices[i].size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset to fill mode
            break;

        case LARSSON_SPHERE:
            glUniform3f(glGetUniformLocation(bvShaderProgram, "boundingVolumeColor"), larssonSphereColor.r, larssonSphereColor.g, larssonSphereColor.b); // Set Larsson sphere color
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode
            for (size_t i = 0; i < larssonSphereVAOs.size(); ++i) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::scale(model, glm::vec3(scales[i], scales[i], scales[i]));
                int modelLoc = glGetUniformLocation(bvShaderProgram, "model");
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                glBindVertexArray(larssonSphereVAOs[i]);
                glDrawElements(GL_LINE_LOOP, larssonSphereIndices[i].size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset to fill mode
            break;

        case PCA_SPHERE:
            glUniform3f(glGetUniformLocation(bvShaderProgram, "boundingVolumeColor"), pcaSphereColor.r, pcaSphereColor.g, pcaSphereColor.b); // Set PCA sphere color
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe mode
            for (size_t i = 0; i < pcaSphereVAOs.size(); ++i) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::scale(model, glm::vec3(scales[i], scales[i], scales[i]));
                int modelLoc = glGetUniformLocation(bvShaderProgram, "model");
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

                glBindVertexArray(pcaSphereVAOs[i]);
                glDrawElements(GL_LINE_LOOP, pcaSphereIndices[i].size(), GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Reset to fill mode
            break;

        case NONE:
        default:
            break;
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup VAOs, VBOs, and EBOs
    for (size_t i = 0; i < VAOs.size(); ++i) {
        glDeleteVertexArrays(1, &VAOs[i]);
        glDeleteBuffers(1, &VBOs[i]);
        glDeleteBuffers(1, &EBOs[i]);
    }
    for (size_t i = 0; i < bboxVAOs.size(); ++i) {
        glDeleteVertexArrays(1, &bboxVAOs[i]);
        glDeleteBuffers(1, &bboxVBOs[i]);
    }
    for (size_t i = 0; i < ritterSphereVAOs.size(); ++i) {
        glDeleteVertexArrays(1, &ritterSphereVAOs[i]);
        glDeleteBuffers(1, &ritterSphereVBOs[i]);
        glDeleteBuffers(1, &ritterSphereEBOs[i]);
    }
    for (size_t i = 0; i < larssonSphereVAOs.size(); ++i) {
        glDeleteVertexArrays(1, &larssonSphereVAOs[i]);
        glDeleteBuffers(1, &larssonSphereVBOs[i]);
        glDeleteBuffers(1, &larssonSphereEBOs[i]);
    }
    for (size_t i = 0; i < pcaSphereVAOs.size(); ++i) {
        glDeleteVertexArrays(1, &pcaSphereVAOs[i]);
        glDeleteBuffers(1, &pcaSphereVBOs[i]);
        glDeleteBuffers(1, &pcaSphereEBOs[i]);
    }
    glDeleteProgram(shaderProgram);

    // Terminate GLFW
    glfwTerminate();
    return 0;
}