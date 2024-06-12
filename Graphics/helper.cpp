#include "helper.h"

GLuint shaderProgram;

void shader()
{
    // Vertex shader source code
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
    )";

    // Fragment shader source code
    const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    uniform vec3 color;

    void main()
    {
        FragColor = vec4(color, 1.0);
    }
    )";

    // Build and compile the shader program
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void SphereVsSphere(GLFWwindow* window, float radius1, float radius2)
{
    // Generate sphere data for the largest sphere
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    generateSphere(sphereVertices, sphereIndices, std::max(radius1, radius2), 36, 18);

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Projection matrix
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

    Sphere sphere1 = { glm::vec3(0.f, 0.0f, -5.0f), radius1 };
    Sphere sphere2 = { glm::vec3(0.f, 0.0f, -5.0f), radius2 };

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);

        // View matrix
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

        // Set the projection and view matrix uniforms
        GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Animate the spheres moving left and right
        float time = static_cast<float>(glfwGetTime());
        sphere1.position.x = sin(time) * 2.0f;
        sphere2.position.x = -sin(time) * 2.0f;

        // Check for intersection
        bool intersecting = checkIntersection(sphere1, sphere2);

        // Draw sphere 1
        glm::mat4 model1 = glm::translate(glm::mat4(1.0f), sphere1.position);
        glm::mat4 scale1 = glm::scale(glm::mat4(1.0f), glm::vec3(radius1));
        model1 = model1 * scale1;
        GLint modelLoc1 = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc1, 1, GL_FALSE, glm::value_ptr(model1));

        GLint colorLoc1 = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc1, intersecting ? 1.0f : 0.0f, intersecting ? 0.0f : 1.0f, 0.0f);

        glBindVertexArray(VAO);
        glDrawElements(GL_LINE_LOOP, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        // Draw sphere 2
        glm::mat4 model2 = glm::translate(glm::mat4(1.0f), sphere2.position);
        glm::mat4 scale2 = glm::scale(glm::mat4(1.0f), glm::vec3(radius2));
        model2 = model2 * scale2;
        GLint modelLoc2 = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, glm::value_ptr(model2));

        GLint colorLoc2 = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc2, intersecting ? 1.0f : 0.0f, intersecting ? 0.0f : 1.0f, 0.0f);

        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void AABBvsSphere(GLFWwindow* window, float radius, const glm::vec3& initialBoxCenter, const glm::vec3& initialBoxHalfExtents)
{
    // Generate sphere data
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    generateSphere(sphereVertices, sphereIndices, radius, 36, 18);

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Projection matrix
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

    Sphere sphere = { glm::vec3(0.f, 0.0f, -5.0f), radius };
    AABB box = { initialBoxCenter - initialBoxHalfExtents, initialBoxCenter + initialBoxHalfExtents, initialBoxCenter, initialBoxHalfExtents };

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);

        // View matrix
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

        // Set the projection and view matrix uniforms
        GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Animate the sphere moving left and right
        float time = static_cast<float>(glfwGetTime());
        sphere.position.x = sin(time) * 2.0f;

        // Animate the box moving left and right (opposite direction of the sphere)
        float boxOffset = -sin(time) * 2.0f;
        box.center = initialBoxCenter + glm::vec3(boxOffset, 0.0f, 0.0f);
        box.min = box.center - box.halfExtents;
        box.max = box.center + box.halfExtents;

        // Check for intersection
        bool intersecting = checkIntersection(sphere, box);

        // Draw sphere
        glm::mat4 model1 = glm::translate(glm::mat4(1.0f), sphere.position);
        glm::mat4 scale1 = glm::scale(glm::mat4(1.0f), glm::vec3(radius));
        model1 = model1 * scale1;
        GLint modelLoc1 = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc1, 1, GL_FALSE, glm::value_ptr(model1));

        GLint colorLoc1 = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc1, intersecting ? 1.0f : 0.0f, intersecting ? 0.0f : 1.0f, 0.0f);

        glBindVertexArray(VAO);
        glDrawElements(GL_LINE_LOOP, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        // Draw AABB
        std::vector<float> boxVertices = {
            box.min.x, box.min.y, box.min.z, // Bottom face
            box.max.x, box.min.y, box.min.z,
            box.max.x, box.min.y, box.max.z,
            box.min.x, box.min.y, box.max.z,

            box.min.x, box.max.y, box.min.z, // Top face
            box.max.x, box.max.y, box.min.z,
            box.max.x, box.max.y, box.max.z,
            box.min.x, box.max.y, box.max.z,
        };

        std::vector<unsigned int> boxIndices = {
            0, 1, 1, 2, 2, 3, 3, 0, // Bottom face
            4, 5, 5, 6, 6, 7, 7, 4, // Top face
            0, 4, 1, 5, 2, 6, 3, 7  // Connecting edges
        };

        GLuint boxVBO, boxVAO, boxEBO;
        glGenVertexArrays(1, &boxVAO);
        glGenBuffers(1, &boxVBO);
        glGenBuffers(1, &boxEBO);

        glBindVertexArray(boxVAO);

        glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
        glBufferData(GL_ARRAY_BUFFER, boxVertices.size() * sizeof(float), boxVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, boxIndices.size() * sizeof(unsigned int), boxIndices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glm::mat4 model2 = glm::mat4(1.0f);
        GLint modelLoc2 = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, glm::value_ptr(model2));

        GLint colorLoc2 = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc2, intersecting ? 1.0f : 0.0f, intersecting ? 0.0f : 1.0f, 0.0f);

        glBindVertexArray(boxVAO);
        glDrawElements(GL_LINES, boxIndices.size(), GL_UNSIGNED_INT, 0);

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Cleanup
        glDeleteVertexArrays(1, &boxVAO);
        glDeleteBuffers(1, &boxVBO);
        glDeleteBuffers(1, &boxEBO);
    }

    // Cleanup sphere
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void AABBvsAABB(GLFWwindow* window, const glm::vec3& initialBox1Center, const glm::vec3& initialBox1HalfExtents, const glm::vec3& initialBox2Center, const glm::vec3& initialBox2HalfExtents) {

    // Generate data for the AABBs
    std::vector<float> boxVertices = {
        -0.5f, -0.5f, -0.5f, // Bottom face
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        -0.5f,  0.5f, -0.5f, // Top face
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
    };

    std::vector<unsigned int> boxIndices = {
        0, 1, 1, 2, 2, 3, 3, 0, // Bottom face
        4, 5, 5, 6, 6, 7, 7, 4, // Top face
        0, 4, 1, 5, 2, 6, 3, 7  // Connecting edges
    };

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, boxVertices.size() * sizeof(float), boxVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, boxIndices.size() * sizeof(unsigned int), boxIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Projection matrix
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

    AABB box1 = { initialBox1Center - initialBox1HalfExtents, initialBox1Center + initialBox1HalfExtents, initialBox1Center, initialBox1HalfExtents };
    AABB box2 = { initialBox2Center - initialBox2HalfExtents, initialBox2Center + initialBox2HalfExtents, initialBox2Center, initialBox2HalfExtents };

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);

        // View matrix
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));

        // Set the projection and view matrix uniforms
        GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Animate the boxes moving left and right
        float time = static_cast<float>(glfwGetTime());
        float box1Offset = sin(time) * 2.0f;
        float box2Offset = -sin(time) * 2.0f;

        box1.center = initialBox1Center + glm::vec3(box1Offset, 0.0f, 0.0f);
        box1.min = box1.center - box1.halfExtents;
        box1.max = box1.center + box1.halfExtents;

        box2.center = initialBox2Center + glm::vec3(box2Offset, 0.0f, 0.0f);
        box2.min = box2.center - box2.halfExtents;
        box2.max = box2.center + box2.halfExtents;

        // Check for intersection
        bool intersecting = checkIntersection(box1, box2);

        // Draw box 1
        glm::mat4 model1 = glm::translate(glm::mat4(1.0f), box1.center);
        model1 = glm::scale(model1, box1.halfExtents * 2.0f);
        GLint modelLoc1 = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc1, 1, GL_FALSE, glm::value_ptr(model1));

        GLint colorLoc1 = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc1, intersecting ? 1.0f : 0.0f, intersecting ? 0.0f : 1.0f, 0.0f);

        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, boxIndices.size(), GL_UNSIGNED_INT, 0);

        // Draw box 2
        glm::mat4 model2 = glm::translate(glm::mat4(1.0f), box2.center);
        model2 = glm::scale(model2, box2.halfExtents * 2.0f);
        GLint modelLoc2 = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, glm::value_ptr(model2));

        GLint colorLoc2 = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc2, intersecting ? 1.0f : 0.0f, intersecting ? 0.0f : 1.0f, 0.0f);

        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, boxIndices.size(), GL_UNSIGNED_INT, 0);

        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, int sectorCount, int stackCount)
{
    const float PI = 3.14159265359f;
    float x, y, z, xy;                            // vertex position
    float lengthInv = 1.0f / radius;    // vertex normal

    float sectorStep = 2 * PI / sectorCount;
    float stackStep = PI / stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stackCount; ++i)
    {
        stackAngle = PI / 2 - i * stackStep;        // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);             // r * cos(u)
        z = radius * sinf(stackAngle);              // r * sin(u)

        for (int j = 0; j <= sectorCount; ++j)
        {
            sectorAngle = j * sectorStep;           // starting from 0 to 2pi

            // vertex position (x, y, z)
            x = xy * cosf(sectorAngle);             // r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);             // r * cos(u) * sin(v)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // indices
    unsigned int k1, k2;
    for (int i = 0; i < stackCount; ++i)
    {
        k1 = i * (sectorCount + 1);     // beginning of current stack
        k2 = k1 + sectorCount + 1;      // beginning of next stack

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stackCount - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

bool checkIntersection(const Sphere& sphere1, const Sphere& sphere2)
{
    float distance = glm::distance(sphere1.position, sphere2.position);
    return distance <= (sphere1.radius + sphere2.radius);
}

bool checkIntersection(const Sphere& sphere, const AABB& box)
{
    glm::vec3 closestPoint = glm::clamp(sphere.position, box.min, box.max);
    float distance = glm::distance(closestPoint, sphere.position);
    return distance < sphere.radius;
}

bool checkIntersection(const AABB& box1, const AABB& box2)
{
    return (std::abs(box1.center.x - box2.center.x) <= (box1.halfExtents.x + box2.halfExtents.x)) &&
        (std::abs(box1.center.y - box2.center.y) <= (box1.halfExtents.y + box2.halfExtents.y)) &&
        (std::abs(box1.center.z - box2.center.z) <= (box1.halfExtents.z + box2.halfExtents.z));
}