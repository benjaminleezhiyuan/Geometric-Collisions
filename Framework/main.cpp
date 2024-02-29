#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>

const int WIDTH = 1920;
const int HEIGHT = 1080;
const int GRID_SIZE = 100;
float alpha{};



// Keyboard callback function
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_MINUS) {
            alpha -= 0.1f;
            if (alpha < 0.0f) alpha = 0.0f;
        }
        else if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) {
            alpha += 0.1f;
            if (alpha > 1.0f) alpha = 1.0f;
        }
    }
}

void drawGrid() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Calculate square size
    float squareSize = static_cast<float>(WIDTH) / GRID_SIZE;

    // Draw grid
    glBegin(GL_QUADS);
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            // Random color for each square with half transparency
            glColor4f(static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                alpha); //Alpha value changes based on level of glitch.

            glVertex2f(i * squareSize, j * squareSize);
            glVertex2f(i * squareSize + squareSize, j * squareSize);
            glVertex2f(i * squareSize + squareSize, j * squareSize + squareSize);
            glVertex2f(i * squareSize, j * squareSize + squareSize);
        }
    }
    glEnd();
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Grid of Squares", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Set keyboard callback function
    glfwSetKeyCallback(window, key_callback);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Set viewport size
    glViewport(0, 0, WIDTH, HEIGHT);

    // Set clear color to white
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Set orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIDTH, 0, HEIGHT, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Render here
        drawGrid();

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Terminate GLFW
    glfwTerminate();

    return 0;
}
