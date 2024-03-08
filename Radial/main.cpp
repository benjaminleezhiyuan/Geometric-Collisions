#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>

// Vertex shader source code
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    out vec2 TexCoord;

    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        TexCoord = (aPos + 1.0) / 2.0;
    }
)";


// Fragment shader source code
const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform vec2 center;
uniform float radius;
uniform float time; // Time parameter for animation
uniform float radial;

void main() {
    // Calculate angle of the current fragment
    vec2 toCenter = TexCoord - center;
    float angle = atan(toCenter.y, toCenter.x);

    // Convert angle to degrees
    angle = degrees(angle);

    // Adjust angle to start from 0 to 360 degrees
    angle = mod(angle + 360.0, 360.0);

    // Define the start and end angles for the desired range
    float startAngle = 156.0; // Starting angle (quadrant 4)
    float endAngle = radial + startAngle;    // Ending angle (quadrant 1)

    // Normalize the angles to the range [0, 360]
    startAngle = mod(startAngle + 360.0, 360.0);
    endAngle = mod(endAngle + 360.0, 360.0);

    // Handle cases where the start angle is greater than the end angle
    if (startAngle > endAngle) {
        // If start angle is greater than end angle, the range spans the discontinuity (e.g., 270° to 90°)
        // So, we need to discard fragments outside this range
        if (angle < startAngle && angle > endAngle) {
            discard;
        }
    } else {
        // If start angle is less than end angle, the range does not span the discontinuity (e.g., 90° to 270°)
        // So, we discard fragments outside this range
        if (angle < startAngle || angle > endAngle) {
            discard;
        }
    }

    // Inside the desired angle range, render the object
    FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color (modify as needed)
}

)";

// Function to compile shader
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // Check for compilation errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
    }

    return shader;
}

// Function to create shader program
GLuint createShaderProgram() {
    // Compile vertex and fragment shaders
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    // Create shader program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader program linking error:\n" << infoLog << std::endl;
    }

    // Delete individual shaders (no longer needed after linking)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "Clock Wipe Example", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Set up vertex data and buffers for a full-screen quad
    float vertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f,
    };

    GLuint indices[] = {
        0, 1, 2,
        0, 2, 3,
    };

    GLuint VBO, VAO, EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    // Create shader program
    GLuint shaderProgram = createShaderProgram();
    float radial{};
    while (!glfwWindowShouldClose(window)) {
        // Poll for and process events
        glfwPollEvents();

        // Update crop value based on keyboard input
        if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
        {
            // Increase crop value for X axis
            radial += 1.f;  // Adjust the step size as needed
            radial = glm::clamp(radial, 0.0f, 359.99f);  // Clamp to the range [0.0, 1.0]
        }

        if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
        {
            // Decrease crop value for X axis
            radial -= 1.f;  // Adjust the step size as needed
            radial = glm::clamp(radial, 0.0f, 359.99f);  // Clamp to the range [0.0, 1.0]
        }

        // Clear the screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Inside the main loop
        float currentTime = glfwGetTime();
        float normalizedTime = fmod(currentTime, 5.0) / 5.0; // Adjust the duration as needed
        float radius = 0.1;

        // Use the shader program
        glUseProgram(shaderProgram);

        // Set uniforms
        glUniform2f(glGetUniformLocation(shaderProgram, "center"), 0.5, 0.5);
        glUniform1f(glGetUniformLocation(shaderProgram, "radius"), radius);
        glUniform1f(glGetUniformLocation(shaderProgram, "time"), normalizedTime);
        glUniform1f(glGetUniformLocation(shaderProgram, "radial"), radial);

        // Draw a full-screen quad
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Reset shader program
        glUseProgram(0);

        // Swap the front and back buffers
        glfwSwapBuffers(window);
    }

    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    // Terminate GLFW
    glfwTerminate();

    return 0;
}
