#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <ctime>
#include <iostream>

// Shader sources
const GLchar* vertexSource =
"#version 410 core\n"
"in vec2 position;"
"void main() {"
"   gl_Position = vec4(position, 0.0, 1.0);"
"}";

const GLchar* fragmentSource = R"(
#version 410 core
out vec4 FragColor;

// Function to generate a random float between 0 and 1 using time as a seed
float rand(float time, vec2 co) {
    return fract(sin(dot(co.xy + time, vec2(12.9898, 78.233))) * 43758.5453);
}

uniform int gridSize;
uniform float currentTime; // Time passed from the CPU

void main() {
    vec2 uv = gl_FragCoord.xy / 800.0;

    // Calculate the square's grid coordinates
    int x = int(uv.x * float(gridSize));
    int y = int(uv.y * float(gridSize));

    // Calculate color for the square using UV coordinates, grid coordinates, and current time
    vec3 color = vec3(rand(currentTime, vec2(x, y)), rand(currentTime, vec2(x + 1, y)), rand(currentTime, vec2(x, y + 1)));

    // Apply color and alpha for the square
    FragColor = vec4(color, 1.0);
}

)";

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set OpenGL version to 4.1
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(800, 600, "Checkerboard Pattern", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Define the vertices for the checkerboard
    GLfloat vertices[] = {
        // First triangle
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
        // Second triangle
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f
    };

    // Create Vertex Buffer Object (VBO)
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Create Vertex Array Object (VAO)
    GLuint VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

    // Create Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, NULL);
    glCompileShader(vertexShader);

    // Create Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShader);

    // Create Shader Program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the color buffer
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Retrieve the location of the uniform variable
        GLint gridSizeLocation = glGetUniformLocation(shaderProgram, "gridSize");

        // Initialize a variable to hold the last time the uniform was updated
        static double lastUpdateTime = glfwGetTime();

        // Update the uniform value every x seconds
        double currentTime = glfwGetTime();
        if (currentTime - lastUpdateTime >= 0.1) {
            // Pass the current time to the shader as a uniform variable
            GLint timeLocation = glGetUniformLocation(shaderProgram, "currentTime");
            glUniform1f(timeLocation, currentTime);

            // Update the last update time
            lastUpdateTime = currentTime;
        }
        // Set initial grid size
        int gridSize = 10;
        
        glUniform1i(gridSizeLocation, gridSize);

        // Draw the checkerboard
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // Terminate GLFW
    glfwTerminate();
    return 0;
}
