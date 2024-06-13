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
    uniform float pointSize;
    
    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        gl_PointSize = 5.f;
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

void SphereVsSphere(GLFWwindow* window, float radius1, float radius2, const glm::vec3 position1, const glm::vec3 position2)
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

    Sphere sphere1 = { position1, radius1 };
    Sphere sphere2 = { position2, radius2 };

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
    glDrawElements(GL_LINES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

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
        glDrawElements(GL_LINES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

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

     

        // Cleanup
        glDeleteVertexArrays(1, &boxVAO);
        glDeleteBuffers(1, &boxVBO);
        glDeleteBuffers(1, &boxEBO);
    

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

       
    

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void PointVsSphere(GLFWwindow* window, const glm::vec3& point, float radius)
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
    Point point1 = { point };

   
        // Input
        processInput(window);

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


        // Animate the point and sphere moving left and right
        float time = static_cast<float>(glfwGetTime());
        point1.coordinates.x = sin(time) * 2.0f;
        sphere.position.x = -sin(time) * 2.0f;

        // Check for intersection
        bool isInside = checkIntersection(point1, sphere);

        // Draw sphere
        glm::mat4 model = glm::translate(glm::mat4(1.0f), sphere.position);
        glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(radius));
        model = model * scale;
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc, isInside ? 1.0f : 0.0f, isInside ? 0.0f : 1.0f, 0.0f);

        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        // Draw point
        glm::mat4 pointModel = glm::translate(glm::mat4(1.0f), point1.coordinates);
        pointModel = glm::scale(pointModel, glm::vec3(0.1f));
        GLint pointModelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(pointModelLoc, 1, GL_FALSE, glm::value_ptr(pointModel));

        GLint pointColorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(pointColorLoc, isInside ? 1.0f : 0.0f, isInside ? 0.0f : 1.0f, 0.0f);

        glBindVertexArray(VAO); // Unbind the VAO
        glDrawArrays(GL_POINTS, 0, 1); // Draw the point

    
    

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void PointVsAABB(GLFWwindow* window, const glm::vec3& initialPointCoords, const glm::vec3& boxCenter, const glm::vec3& boxHalfExtents)
{
    // Generate AABB data
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

    Point point = { initialPointCoords };
    AABB box = { boxCenter - boxHalfExtents, boxCenter + boxHalfExtents, boxCenter, boxHalfExtents };

    
        // Input
        processInput(window);

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

        // Animate the point moving left and right
        float time = static_cast<float>(glfwGetTime());
        point.coordinates.x = sin(time) * 2.0f;
        box.center.x = -sin(time) * 2.0f;
        box.min = box.center - box.halfExtents;
        box.max = box.center + box.halfExtents;

        // Check for intersection
        bool isInside = checkIntersection(point, box);

        // Draw AABB
        glm::mat4 model = glm::translate(glm::mat4(1.0f), box.center);
        model = glm::scale(model, box.halfExtents * 2.0f);
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc, isInside ? 1.0f : 0.0f, isInside ? 0.0f : 1.0f, 0.0f);

        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, boxIndices.size(), GL_UNSIGNED_INT, 0);

        // Draw point
        glm::mat4 pointModel = glm::translate(glm::mat4(1.0f), point.coordinates);
        pointModel = glm::scale(pointModel, glm::vec3(0.1f));
        GLint pointModelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(pointModelLoc, 1, GL_FALSE, glm::value_ptr(pointModel));

        GLint pointColorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(pointColorLoc, isInside ? 1.0f : 0.0f, isInside ? 0.0f : 1.0f, 0.0f);

        glDrawArrays(GL_POINTS, 0, 1); // Draw the point

      
    

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void PointVsPlane(GLFWwindow* window, const glm::vec3& initialPointCoords, const glm::vec3& planeNormal, float planeOffset)
{
    // Define the vertices and indices for a plane
    std::vector<float> planeVertices = {
        // Positions         // Colors
        -5.0f, 0.0f, -20.0f,  0.0f, 1.0f, 0.0f,
         5.0f, 0.0f, -20.0f,  0.0f, 1.0f, 0.0f,
         5.0f, 0.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -5.0f, 0.0f,  0.0f,  0.0f, 1.0f, 0.0f
    };

    std::vector<unsigned int> planeIndices = {
        0, 1, 2,
        2, 3, 0
    };

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, planeVertices.size() * sizeof(float), planeVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, planeIndices.size() * sizeof(unsigned int), planeIndices.data(), GL_STATIC_DRAW);

    // Set the vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Projection matrix
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

    Point point = { initialPointCoords };
    Plane plane = { glm::vec4(planeNormal, planeOffset) };

    
        // Input
        processInput(window);

        // Clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);

        // View matrix
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, -10.0f));

        // Set the projection and view matrix uniforms
        GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Animate the point moving up and down
        float time = static_cast<float>(glfwGetTime());
        point.coordinates.y = sin(time) * 2.0f;

        // Check for intersection
        bool isIntersecting = checkIntersection(point, plane);

        // Draw plane
        glm::mat4 model = glm::mat4(1.0f);
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc, isIntersecting ? 1.0f : 0.0f, isIntersecting ? 0.0f : 1.0f, 0.0f); // Red if intersecting, green otherwise

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, planeIndices.size(), GL_UNSIGNED_INT, 0);

        // Draw point
        glm::mat4 pointModel = glm::translate(glm::mat4(1.0f), point.coordinates);
        pointModel = glm::scale(pointModel, glm::vec3(0.1f));
        GLint pointModelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(pointModelLoc, 1, GL_FALSE, glm::value_ptr(pointModel));

        GLint pointColorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(pointColorLoc, 1.0f, 1.0f, 1.0f);

        glDrawArrays(GL_POINTS, 0, 1); // Draw the point

      
    

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void PointVsTriangle(GLFWwindow* window, const glm::vec3& initialPointCoords, const Triangle& triangle)
{
    // Define the vertices and indices for a triangle
    std::vector<float> triangleVertices = {
        // Positions         // Colors
        triangle.v1.x, triangle.v1.y, triangle.v1.z,  0.0f, 1.0f, 0.0f,
        triangle.v2.x, triangle.v2.y, triangle.v2.z,  0.0f, 1.0f, 0.0f,
        triangle.v3.x, triangle.v3.y, triangle.v3.z,  0.0f, 1.0f, 0.0f
    };

    std::vector<unsigned int> triangleIndices = {
        0, 1, 2
    };

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, triangleVertices.size() * sizeof(float), triangleVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangleIndices.size() * sizeof(unsigned int), triangleIndices.data(), GL_STATIC_DRAW);

    // Set the vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Projection matrix
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.1f, 100.0f);

    Point point = { initialPointCoords };

    
        // Input
        processInput(window);

        // Clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);

        // View matrix
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));

        GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Animate the point moving up and down
        float time = static_cast<float>(glfwGetTime());
        point.coordinates.x = sin(time) * 2.0f;

        // Rotate the triangle slowly
        float rotationSpeed = 0.5f; // Adjust this value to control the speed
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), time * rotationSpeed, glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around y-axis
        Triangle rotatedTriangle;
        rotatedTriangle.v1 = glm::vec3(rotation * glm::vec4(triangle.v1, 1.0f));
        rotatedTriangle.v2 = glm::vec3(rotation * glm::vec4(triangle.v2, 1.0f));
        rotatedTriangle.v3 = glm::vec3(rotation * glm::vec4(triangle.v3, 1.0f));

        // Check for intersection
        bool isIntersecting = checkIntersection(point, rotatedTriangle);

        // Draw triangle
        glm::mat4 model = rotation;
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc, isIntersecting ? 1.0f : 0.0f, isIntersecting ? 0.0f : 1.0f, 0.0f); // Red if intersecting, green otherwise

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, triangleIndices.size(), GL_UNSIGNED_INT, 0);

        // Draw point
        glm::mat4 pointModel = glm::translate(glm::mat4(1.0f), point.coordinates);
        pointModel = glm::scale(pointModel, glm::vec3(0.1f));
        GLint pointModelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(pointModelLoc, 1, GL_FALSE, glm::value_ptr(pointModel));

        GLint pointColorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(pointColorLoc, 1.0f, 1.0f, 1.0f);

        glDrawArrays(GL_POINTS, 0, 1); // Draw the point

   
    

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void PlaneVsAABB(GLFWwindow* window, const glm::vec3& planeNormal, float planeOffset,const glm::vec3& initialBoxCenter, const glm::vec3& initialBoxHalfExtents)
{
    // Define the vertices and indices for an AABB
    std::vector<float> boxVertices = {
        // Positions        
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f
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

    // Set the vertex attribute pointers
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

    AABB box = { initialBoxCenter - initialBoxHalfExtents, initialBoxCenter + initialBoxHalfExtents, initialBoxCenter, initialBoxHalfExtents };
    Plane plane = { glm::vec4(planeNormal, planeOffset) };

    
        // Input
        processInput(window);

        // Clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);

        // View matrix
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, -10.0f));

        // Set the projection and view matrix uniforms
        GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Animate the box moving up and down
        float time = static_cast<float>(glfwGetTime());
        box.center.y = sin(time) * 2.0f;
        box.min = box.center - box.halfExtents;
        box.max = box.center + box.halfExtents;

        // Check for intersection
        bool isIntersecting = checkIntersection(plane, box);

        // Draw AABB
        glm::mat4 model = glm::translate(glm::mat4(1.0f), box.center);
        model = glm::scale(model, box.halfExtents * 2.0f);
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(colorLoc, 1.f, 1.f, 1.f); // Red if intersecting, green otherwise

        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, boxIndices.size(), GL_UNSIGNED_INT, 0);

        // Draw plane
        std::vector<float> planeVertices = {
            -5.0f, 0.0f, -20.0f,  0.0f, 1.0f, 0.0f,
             5.0f, 0.0f, -20.0f,  0.0f, 1.0f, 0.0f,
             5.0f, 0.0f,  0.0f,  0.0f, 1.0f, 0.0f,
            -5.0f, 0.0f,  0.0f,  0.0f, 1.0f, 0.0f
        };

        std::vector<unsigned int> planeIndices = {
            0, 1, 2,
            2, 3, 0
        };

        GLuint planeVBO, planeVAO, planeEBO;
        glGenVertexArrays(1, &planeVAO);
        glGenBuffers(1, &planeVBO);
        glGenBuffers(1, &planeEBO);

        glBindVertexArray(planeVAO);

        glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
        glBufferData(GL_ARRAY_BUFFER, planeVertices.size() * sizeof(float), planeVertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, planeIndices.size() * sizeof(unsigned int), planeIndices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glm::mat4 planeModel = glm::mat4(1.0f);
        GLint planeModelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(planeModelLoc, 1, GL_FALSE, glm::value_ptr(planeModel));

        GLint planeColorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(planeColorLoc, isIntersecting ? 1.0f : 0.0f, isIntersecting ? 0.0f : 1.0f, 0.0f); // Red if intersecting, green otherwise

        glBindVertexArray(planeVAO);
        glDrawElements(GL_TRIANGLES, planeIndices.size(), GL_UNSIGNED_INT, 0);

   

        // Cleanup plane
        glDeleteVertexArrays(1, &planeVAO);
        glDeleteBuffers(1, &planeVBO);
        glDeleteBuffers(1, &planeEBO);
    

    // Cleanup box
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void PlaneVsSphere(GLFWwindow* window, const glm::vec3& planeNormal, float planeOffset, float sphereRadius)
{
    // Define the vertices and indices for a plane
    std::vector<float> planeVertices = {
        -5.0f, 0.0f, -20.0f,  0.0f, 1.0f, 0.0f,
         5.0f, 0.0f, -20.0f,  0.0f, 1.0f, 0.0f,
         5.0f, 0.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -5.0f, 0.0f,  0.0f,  0.0f, 1.0f, 0.0f
    };

    std::vector<unsigned int> planeIndices = {
        0, 1, 2,
        2, 3, 0
    };

    GLuint planeVBO, planeVAO, planeEBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glGenBuffers(1, &planeEBO);

    glBindVertexArray(planeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, planeVertices.size() * sizeof(float), planeVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, planeIndices.size() * sizeof(unsigned int), planeIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Generate sphere data
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    generateSphere(sphereVertices, sphereIndices, sphereRadius, 36, 18);

    GLuint sphereVBO, sphereVAO, sphereEBO;
    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindVertexArray(sphereVAO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
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

    Plane plane = { glm::vec4(planeNormal, planeOffset) };
    Sphere sphere = { glm::vec3(0.0f, 0.0f, -5.0f), sphereRadius };

   
        // Input
        processInput(window);

        // Clear the color and depth buffer
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);

        // View matrix
        glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, -10.0f));

        // Set the projection and view matrix uniforms
        GLint projectionLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

        GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Animate the sphere moving up and down
        float time = static_cast<float>(glfwGetTime());
        sphere.position.y = sin(time) * 2.0f;

        // Check for intersection
        bool isIntersecting = checkIntersection(plane, sphere);

        // Draw plane
        glm::mat4 planeModel = glm::mat4(1.0f);
        GLint planeModelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(planeModelLoc, 1, GL_FALSE, glm::value_ptr(planeModel));

        GLint planeColorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(planeColorLoc, isIntersecting ? 1.0f : 0.0f, isIntersecting ? 0.0f : 1.0f, 0.0f); // Red if intersecting, green otherwise

        glBindVertexArray(planeVAO);
        glDrawElements(GL_TRIANGLES, planeIndices.size(), GL_UNSIGNED_INT, 0);

        // Draw sphere
        glm::mat4 sphereModel = glm::translate(glm::mat4(1.0f), sphere.position);
        sphereModel = glm::scale(sphereModel, glm::vec3(sphereRadius));
        GLint sphereModelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(sphereModelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel));

        GLint sphereColorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(sphereColorLoc, 1.f,1.f,1.f); // Red if intersecting, green otherwise

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_LINES, sphereIndices.size(), GL_UNSIGNED_INT, 0);


    

    // Cleanup plane
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteBuffers(1, &planeEBO);

    // Cleanup sphere
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
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

bool checkIntersection(const Point& point, const Sphere& sphere) {
    float distance = glm::distance(point.coordinates, sphere.position);
    return distance <= sphere.radius;
}

bool checkIntersection(const Point& point, const AABB& box) {
    return (point.coordinates.x >= box.min.x && point.coordinates.x <= box.max.x) &&
        (point.coordinates.y >= box.min.y && point.coordinates.y <= box.max.y) &&
        (point.coordinates.z >= box.min.z && point.coordinates.z <= box.max.z);
}

bool checkIntersection(const Point& point, const Plane& plane) {
    // Plane equation: Ax + By + Cz + D = 0
    // If point satisfies the plane equation (with a small epsilon tolerance), it's on the plane
    float distance = glm::dot(plane.normal, glm::vec4(point.coordinates, 1.0f));
    return glm::abs(distance) < 0.01f; // Epsilon tolerance
}

bool checkIntersection(const Point& point, const Triangle& triangle) {
    glm::vec3 v0 = triangle.v3 - triangle.v1;
    glm::vec3 v1 = triangle.v2 - triangle.v1;
    glm::vec3 v2 = point.coordinates - triangle.v1;

    float dot00 = glm::dot(v0, v0);
    float dot01 = glm::dot(v0, v1);
    float dot02 = glm::dot(v0, v2);
    float dot11 = glm::dot(v1, v1);
    float dot12 = glm::dot(v1, v2);

    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    return (u >= 0) && (v >= 0) && (u + v < 1);
}

bool checkIntersection(const Plane& plane, const AABB& box)
{
    // Calculate the projection interval radius of the AABB onto the plane normal
    float r = box.halfExtents.x * fabs(plane.normal.x) +
        box.halfExtents.y * fabs(plane.normal.y) +
        box.halfExtents.z * fabs(plane.normal.z);

    // Compute the distance of the box center from the plane
    float s = glm::dot(plane.normal, glm::vec4(box.center, 1.0f)) - plane.normal.w;

    // Intersection occurs if the absolute distance from the center is less than or equal to the projection interval radius
    return fabs(s) <= r;
}

bool checkIntersection(const Plane& plane, const Sphere& sphere)
{
    // Calculate the distance from the sphere's center to the plane
    float distance = glm::dot(plane.normal, glm::vec4(sphere.position, 1.0f)) - plane.normal.w;

    // Check if the distance is less than or equal to the sphere's radius
    return fabs(distance) <= sphere.radius;
}