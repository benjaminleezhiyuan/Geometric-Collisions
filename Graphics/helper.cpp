#include "helper.h"

std::vector<float> sphereVertices;
std::vector<unsigned int> sphereIndices;
GLuint sphereVBO, sphereVAO, sphereEBO;
std::vector<float> boxVertices;
std::vector<unsigned int> boxIndices;
GLuint boxVBO, boxVAO, boxEBO;

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
        //gl_PointSize = pointSize;
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

void spheremake()
{
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
}

void boxmake()
{
    // Define the vertices and indices for an AABB
    boxVertices = {
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

    boxIndices = {
        0, 1, 1, 2, 2, 3, 3, 0, // Bottom face
        4, 5, 5, 6, 6, 7, 7, 4, // Top face
        0, 4, 1, 5, 2, 6, 3, 7  // Connecting edges
    };

    glGenVertexArrays(1, &boxVAO);
    glGenBuffers(1, &boxVBO);
    glGenBuffers(1, &boxEBO);

    glBindVertexArray(boxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
    glBufferData(GL_ARRAY_BUFFER, boxVertices.size() * sizeof(float), boxVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, boxIndices.size() * sizeof(unsigned int), boxIndices.data(), GL_STATIC_DRAW);

    // Set the vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void SphereVsSphere(Sphere Sphere1, Sphere Sphere2)
{
    spheremake();

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    // Projection matrix
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    Sphere sphere1 = { Sphere1 };
    Sphere sphere2 = { Sphere2 };

    // Clear the color and depth buffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // Animate the spheres moving left and right
    float time = static_cast<float>(glfwGetTime());
    if (animate)
    {
        sphere1.position.x = sin(time) * 2.0f;
        sphere2.position.x = -sin(time) * 2.0f;
    }
   
    // Check for intersection
    bool intersecting = checkIntersection(sphere1, sphere2);

    // Draw sphere 1
    glm::mat4 model1 = glm::translate(glm::mat4(1.0f), sphere1.position);
    glm::mat4 scale1 = glm::scale(glm::mat4(1.0f), glm::vec3(sphere1.radius));
    model1 = model1 * scale1;
    GLint modelLoc1 = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc1, 1, GL_FALSE, glm::value_ptr(model1));

    GLint colorLoc1 = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc1, intersecting ? 1.0f : 0.0f, intersecting ? 0.0f : 1.0f, 0.0f);

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_LINES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

    // Draw sphere 2
    glm::mat4 model2 = glm::translate(glm::mat4(1.0f), sphere2.position);
    glm::mat4 scale2 = glm::scale(glm::mat4(1.0f), glm::vec3(sphere2.radius));
    model2 = model2 * scale2;
    GLint modelLoc2 = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc2, 1, GL_FALSE, glm::value_ptr(model2));

    GLint colorLoc2 = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc2, intersecting ? 1.0f : 0.0f, intersecting ? 0.0f : 1.0f, 0.0f);

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_LINES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
}

void AABBVsSphere( AABB aabb, Sphere Sphere1)
{
    spheremake();

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    Sphere sphere = { Sphere1 };
    AABB box = { aabb };

    // Clear the color and depth buffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // Animate the sphere moving left and right
    float time = static_cast<float>(glfwGetTime());
    if (animate)
    {
        sphere.position.x = sin(time) * 2.0f;

        // Animate the box moving left and right (opposite direction of the sphere)
        float boxOffset = -sin(time) * 2.0f;
        box.center += glm::vec3(boxOffset, 0.0f, 0.0f);
        box.min = box.center - box.halfExtents;
        box.max = box.center + box.halfExtents;
    }
    else
    {
        float boxOffset = 0;
        box.center += glm::vec3(boxOffset, 0.0f, 0.0f);
        box.min = box.center - box.halfExtents;
        box.max = box.center + box.halfExtents;
    }

    // Check for intersection
    bool intersecting = checkIntersection(sphere, box);

    // Draw sphere
    glm::mat4 model1 = glm::translate(glm::mat4(1.0f), sphere.position);
    glm::mat4 scale1 = glm::scale(glm::mat4(1.0f), glm::vec3(sphere.radius));
    model1 = model1 * scale1;
    GLint modelLoc1 = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc1, 1, GL_FALSE, glm::value_ptr(model1));

    GLint colorLoc1 = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc1, intersecting ? 1.0f : 0.0f, intersecting ? 0.0f : 1.0f, 0.0f);

    glBindVertexArray(sphereVAO);
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
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
}

void SphereVsAABB( Sphere Sphere1, AABB aabb)
{
    spheremake();
    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    Sphere sphere = { Sphere1 };
    AABB box = { aabb };

    // Clear the color and depth buffer
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Animate the sphere moving left and right
    float time = static_cast<float>(glfwGetTime());
    if (animate)
    {
        sphere.position.x = -sin(time) * 2.0f;

        // Animate the box moving left and right (opposite direction of the sphere)
        float boxOffset = sin(time) * 2.0f;
        box.center += glm::vec3(boxOffset, 0.0f, 0.0f);
        box.min = box.center - box.halfExtents;
        box.max = box.center + box.halfExtents;
    }
    else
    {
        float boxOffset = 0;
        box.center += glm::vec3(boxOffset, 0.0f, 0.0f);
        box.min = box.center - box.halfExtents;
        box.max = box.center + box.halfExtents;
    }


    // Check for intersection
    bool intersecting = checkIntersection(sphere, box);

    // Draw sphere
    glm::mat4 model1 = glm::translate(glm::mat4(1.0f), sphere.position);
    glm::mat4 scale1 = glm::scale(glm::mat4(1.0f), glm::vec3(sphere.radius));
    model1 = model1 * scale1;
    GLint modelLoc1 = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc1, 1, GL_FALSE, glm::value_ptr(model1));

    GLint colorLoc1 = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc1, intersecting ? 1.0f : 0.0f, intersecting ? 0.0f : 1.0f, 0.0f);

    glBindVertexArray(sphereVAO);
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
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
}

void AABBvsAABB( AABB aabb1, AABB aabb2)   {

    boxmake();

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    AABB box1 = { aabb1 };
    AABB box2 = { aabb2 };

    // Animate the boxes moving left and right
    float time = static_cast<float>(glfwGetTime());
    if (animate)
    {
        float box1Offset = sin(time) * 2.0f;
        float box2Offset = -sin(time) * 2.0f;

        box1.center += glm::vec3(box1Offset, 0.0f, 0.0f);
        box1.min = box1.center - box1.halfExtents;
        box1.max = box1.center + box1.halfExtents;

        box2.center += glm::vec3(box2Offset, 0.0f, 0.0f);
        box2.min = box2.center - box2.halfExtents;
        box2.max = box2.center + box2.halfExtents;
    }


    // Check for intersection
    bool intersecting = checkIntersection(box1, box2);

    // Draw box 1
    glm::mat4 model1 = glm::translate(glm::mat4(1.0f), box1.center);
    model1 = glm::scale(model1, box1.halfExtents * 2.0f);
    GLint modelLoc1 = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc1, 1, GL_FALSE, glm::value_ptr(model1));

    GLint colorLoc1 = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc1, intersecting ? 1.0f : 0.0f, intersecting ? 0.0f : 1.0f, 0.0f);

    glBindVertexArray(boxVAO);
    glDrawElements(GL_LINES, boxIndices.size(), GL_UNSIGNED_INT, 0);

    // Draw box 2
    glm::mat4 model2 = glm::translate(glm::mat4(1.0f), box2.center);
    model2 = glm::scale(model2, box2.halfExtents * 2.0f);
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
}

void PointVsSphere( Point point, Sphere sphere1)
{
    spheremake();

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    Sphere sphere = { sphere1 };
    Point point1 = { point };

    // Animate the point and sphere moving left and right
    float time = static_cast<float>(glfwGetTime());
    if (animate)
    {
        point1.coordinates.x = sin(time) * 2.0f;
        sphere.position.x = -sin(time) * 2.0f;
    }


    // Check for intersection
    bool isInside = checkIntersection(point1, sphere);

    // Draw sphere
    glm::mat4 model = glm::translate(glm::mat4(1.0f), sphere.position);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(sphere.radius));
    model = model * scale;
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc, isInside ? 1.0f : 0.0f, isInside ? 0.0f : 1.0f, 0.0f);

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_LINES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

    // Draw point
    glm::mat4 pointModel = glm::translate(glm::mat4(1.0f), point1.coordinates);
    pointModel = glm::scale(pointModel, glm::vec3(0.1f));
    GLint pointModelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(pointModelLoc, 1, GL_FALSE, glm::value_ptr(pointModel));

    GLint pointColorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(pointColorLoc, isInside ? 1.0f : 0.0f, isInside ? 0.0f : 1.0f, 0.0f);

    glBindVertexArray(sphereVAO); // Unbind the VAO
    glDrawArrays(GL_POINTS, 0, 1); // Draw the point

    // Cleanup
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
}

void PointVsAABB( Point point1, AABB aabb)
{
    boxmake();
    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    Point point = { point1 };
    AABB box = { aabb };

    // Animate the point moving left and right
    float time = static_cast<float>(glfwGetTime());
    if (animate)
    {
        point.coordinates.x = sin(time) * 2.0f;
        box.center.x = -sin(time) * 2.0f;
        box.min = box.center - box.halfExtents;
        box.max = box.center + box.halfExtents;
    }


    // Check for intersection
    bool isInside = checkIntersection(point, box);

    // Draw AABB
    glm::mat4 model = glm::translate(glm::mat4(1.0f), box.center);
    model = glm::scale(model, box.halfExtents * 2.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc, isInside ? 1.0f : 0.0f, isInside ? 0.0f : 1.0f, 0.0f);

    glBindVertexArray(boxVAO);
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
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteBuffers(1, &boxVBO);
    glDeleteBuffers(1, &boxEBO);
}

void PointVsPlane( Point point1, Plane plane1)
{
    // Define the vertices and indices for a plane
    glm::vec3 normal = glm::vec3(plane1.normal);
    float d = plane1.normal.w;
    glm::vec3 right = glm::normalize(glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, normal));

    float planeSize = 1.0f;
    std::vector<float> planeVertices = {
        // Positions                        // Colors
        (-right.x + up.x) * planeSize, (-right.y + up.y) * planeSize, (-right.z + up.z) * planeSize, 0.0f, 1.0f, 0.0f,
        (right.x + up.x) * planeSize,  (right.y + up.y) * planeSize,  (right.z + up.z) * planeSize,  0.0f, 1.0f, 0.0f,
        (right.x - up.x) * planeSize,  (right.y - up.y) * planeSize,  (right.z - up.z) * planeSize,  0.0f, 1.0f, 0.0f,
        (-right.x - up.x) * planeSize, (-right.y - up.y) * planeSize, (-right.z - up.z) * planeSize, 0.0f, 1.0f, 0.0f
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

    Point point = { point1 };
    Plane plane = { plane1 };

    // Animate the point moving up and down
    float time = static_cast<float>(glfwGetTime());
    if (animate)
    {
        point.coordinates.y = sin(time) * 2.0f;
    }

    // Check for intersection
    bool isIntersecting = checkIntersection(point, plane);

    // Draw plane
    glm::mat4 model = glm::translate(glm::mat4(1.0f), -normal * d); // Translate plane based on normal and d value
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

void PointVsTriangle( Point point1, Triangle triangle)
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

    Point point = { point1 };
  
    // Animate the point moving up and down
    float time = static_cast<float>(glfwGetTime());
    float rotationSpeed;
    if (animate)
    {
        point.coordinates.x = sin(time) * 2.0f;
        // Rotate the triangle slowly
        rotationSpeed = 0.5f; // Adjust this value to control the speed
    }
    else
    {
        rotationSpeed = 0.f;
    }

    
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), time * rotationSpeed, glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around y-axis
    Triangle rotatedTriangle{};
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

void PlaneVsAABB( Plane plane1, AABB aabb)
{
    boxmake();

    // Define the vertices and indices for a plane
    glm::vec3 normal = glm::vec3(plane1.normal);
    float d = plane1.normal.w;
    glm::vec3 right = glm::normalize(glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, normal));

    float planeSize = 1.0f;
    std::vector<float> planeVertices = {
        // Positions                        // Colors
        (-right.x + up.x) * planeSize, (-right.y + up.y) * planeSize, (-right.z + up.z) * planeSize, 0.0f, 1.0f, 0.0f,
        (right.x + up.x) * planeSize,  (right.y + up.y) * planeSize,  (right.z + up.z) * planeSize,  0.0f, 1.0f, 0.0f,
        (right.x - up.x) * planeSize,  (right.y - up.y) * planeSize,  (right.z - up.z) * planeSize,  0.0f, 1.0f, 0.0f,
        (-right.x - up.x) * planeSize, (-right.y - up.y) * planeSize, (-right.z - up.z) * planeSize, 0.0f, 1.0f, 0.0f
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

    // Set the vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    AABB box = { aabb};
    Plane plane = { plane1 };

    // Animate the box moving up and down
    float time = static_cast<float>(glfwGetTime());
    if (animate)
    {
        box.center.y = sin(time) * 2.0f;
        box.min = box.center - box.halfExtents;
        box.max = box.center + box.halfExtents;
    }


    // Check for intersection
    bool isIntersecting = checkIntersection(plane, box);

    // Draw AABB
    glm::mat4 model = glm::translate(glm::mat4(1.0f), box.center);
    model = glm::scale(model, box.halfExtents * 2.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc, 1.f, 1.f, 1.f); // Red if intersecting, green otherwise

    glBindVertexArray(boxVAO);
    glDrawElements(GL_LINES, boxIndices.size(), GL_UNSIGNED_INT, 0);

    // Draw plane
    glm::mat4 planemodel = glm::translate(glm::mat4(1.0f), -normal * d); // Translate plane based on normal and d value
    GLint planemodelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(planemodelLoc, 1, GL_FALSE, glm::value_ptr(planemodel));

    GLint planecolorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(planecolorLoc, isIntersecting ? 1.0f : 0.0f, isIntersecting ? 0.0f : 1.0f, 0.0f); // Red if intersecting, green otherwise

    glBindVertexArray(planeVAO);
    glDrawElements(GL_TRIANGLES, planeIndices.size(), GL_UNSIGNED_INT, 0);

    // Cleanup plane
    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO);
    glDeleteBuffers(1, &planeEBO);
    
    // Cleanup box
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteBuffers(1, &boxVBO);
    glDeleteBuffers(1, &boxEBO);
}

void PlaneVsSphere( Plane plane1, Sphere sphere1)
{
    spheremake();

    // Define the vertices and indices for a plane
    glm::vec3 normal = glm::vec3(plane1.normal);
    float d = plane1.normal.w;
    glm::vec3 right = glm::normalize(glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, normal));

    float planeSize = 1.0f;
    std::vector<float> planeVertices = {
        // Positions                                                                                 // Colors
        (-right.x + up.x) * planeSize, (-right.y + up.y) * planeSize, (-right.z + up.z) * planeSize, 0.0f, 1.0f, 0.0f,
        (right.x + up.x) * planeSize,  (right.y + up.y) * planeSize,  (right.z + up.z) * planeSize,  0.0f, 1.0f, 0.0f,
        (right.x - up.x) * planeSize,  (right.y - up.y) * planeSize,  (right.z - up.z) * planeSize,  0.0f, 1.0f, 0.0f,
        (-right.x - up.x) * planeSize, (-right.y - up.y) * planeSize, (-right.z - up.z) * planeSize, 0.0f, 1.0f, 0.0f
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

    // Set the vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    Plane plane = {plane1};
    Sphere sphere = { sphere1 };

    // Animate the sphere moving up and down
    float time = static_cast<float>(glfwGetTime());
    if (animate)
    {
        sphere.position.y = sin(time) * 2.f;
    }

    // Check for intersection
    bool isIntersecting = checkIntersection(plane, sphere);

    // Draw plane
    glm::mat4 planemodel = glm::translate(glm::mat4(1.0f), -normal * d); // Translate plane based on normal and d value
    GLint planemodelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(planemodelLoc, 1, GL_FALSE, glm::value_ptr(planemodel));

    GLint planecolorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(planecolorLoc, isIntersecting ? 1.0f : 0.0f, isIntersecting ? 0.0f : 1.0f, 0.0f); // Red if intersecting, green otherwise

    glBindVertexArray(planeVAO);
    glDrawElements(GL_TRIANGLES, planeIndices.size(), GL_UNSIGNED_INT, 0);

    // Draw sphere
    glm::mat4 sphereModel = glm::translate(glm::mat4(1.0f), sphere.position);
    glm::mat4 sphereModelScale = glm::scale(glm::mat4(1.0f), glm::vec3(sphere.radius));
    sphereModel = sphereModel * sphereModelScale;
    GLint sphereModelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(sphereModelLoc, 1, GL_FALSE, glm::value_ptr(sphereModel));

    GLint sphereColorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(sphereColorLoc, 1.f,1.f,1.f);

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

void RayVsPlane( Ray ray, Plane plane)
{
    spheremake();
    // Define the vertices and indices for a plane
    glm::vec3 normal = glm::vec3(plane.normal);
    float d = plane.normal.w;
    glm::vec3 right = glm::normalize(glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, normal));

    float planeSize = 1.0f;
    std::vector<float> planeVertices = {
        // Positions
        (-right.x + up.x) * planeSize, (-right.y + up.y) * planeSize, (-right.z + up.z) * planeSize,
        (right.x + up.x) * planeSize,  (right.y + up.y) * planeSize,  (right.z + up.z) * planeSize,
        (right.x - up.x) * planeSize,  (right.y - up.y) * planeSize,  (right.z - up.z) * planeSize,
        (-right.x - up.x) * planeSize, (-right.y - up.y) * planeSize, (-right.z - up.z) * planeSize
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    Ray ray1 = ray;
    Plane plane1 = plane;

    // Check for intersection
    glm::vec3 intersectionPoint;
    bool isIntersecting = checkIntersection(ray1, plane1, intersectionPoint);

    // Draw plane
    glm::mat4 model = glm::translate(glm::mat4(1.0f), -normal * d); // Translate plane based on normal and d value
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc, isIntersecting ? 1.0f : 0.0f, isIntersecting ? 0.0f : 1.0f, 0.0f); // Red if intersecting, green otherwise

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, planeIndices.size(), GL_UNSIGNED_INT, 0);

    // Draw ray
    glm::vec3 rayEnd = ray1.start + ray1.direction * rayLength; // Extend the ray for visualization
    std::vector<float> rayVertices = {
        ray1.start.x, ray1.start.y, ray1.start.z,
        rayEnd.x, rayEnd.y, rayEnd.z
    };

    GLuint rayVBO, rayVAO;
    glGenVertexArrays(1, &rayVAO);
    glGenBuffers(1, &rayVBO);

    glBindVertexArray(rayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
    glBufferData(GL_ARRAY_BUFFER, rayVertices.size() * sizeof(float), rayVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glm::mat4 rayModel = glm::mat4(1.0f);
    GLint rayModelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(rayModelLoc, 1, GL_FALSE, glm::value_ptr(rayModel));

    GLint rayColorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(rayColorLoc, 1.0f, 1.0f, 1.0f); // White color for the ray

    glBindVertexArray(rayVAO);
    glDrawArrays(GL_LINES, 0, 2); // Draw the ray

    // Draw intersection point if it exists
    if (isIntersecting)
    {
        glm::mat4 intersectionModel = glm::translate(glm::mat4(1.0f), intersectionPoint);
        intersectionModel = glm::scale(intersectionModel, glm::vec3(0.01f)); // Adjust scale for intersection sphere
        GLint intersectionModelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(intersectionModelLoc, 1, GL_FALSE, glm::value_ptr(intersectionModel));

        GLint intersectionColorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(intersectionColorLoc, 0.0f, 1.0f, 0.0f); // Green color for the intersection point

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &rayVAO);
    glDeleteBuffers(1, &rayVBO);
}

void RayVsTriangle( Ray ray, Triangle triangle)
{   
    spheremake();
    // Define the vertices and indices for a triangle
    std::vector<float> triangleVertices = {
        // Positions
        triangle.v1.x, triangle.v1.y, triangle.v1.z,
        triangle.v2.x, triangle.v2.y, triangle.v2.z,
        triangle.v3.x, triangle.v3.y, triangle.v3.z
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
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    Ray ray1 = ray;


    // Animate the ray moving up and down
    float time = static_cast<float>(glfwGetTime());

    // Rotate the triangle slowly
    float rotationSpeed = 0.5f; // Adjust this value to control the speed
    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), time * rotationSpeed, glm::vec3(0.0f, 0.0f, 1.0f)); // Rotate around z-axis
    Triangle rotatedTriangle{};
    rotatedTriangle.v1 = glm::vec3(rotation * glm::vec4(triangle.v1, 1.0f));
    rotatedTriangle.v2 = glm::vec3(rotation * glm::vec4(triangle.v2, 1.0f));
    rotatedTriangle.v3 = glm::vec3(rotation * glm::vec4(triangle.v3, 1.0f));

    // Check for intersection
    glm::vec3 intersectionPoint;
    bool isIntersecting = checkIntersection(ray1, rotatedTriangle, intersectionPoint);

    // Draw triangle
    glm::mat4 model = rotation;
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glPointSize(10.f);
    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc, isIntersecting ? 1.0f : 1.0f, isIntersecting ? 0.0f : 1.0f, isIntersecting ? 0.0f : 1.0f);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, triangleIndices.size(), GL_UNSIGNED_INT, 0);

    // Draw ray
    glm::vec3 rayEnd = ray1.start + ray1.direction * rayLength; // Extend the ray for visualization
    std::vector<float> rayVertices = {
        ray1.start.x, ray1.start.y, ray1.start.z,
        rayEnd.x, rayEnd.y, rayEnd.z
    };

    GLuint rayVBO, rayVAO;
    glGenVertexArrays(1, &rayVAO);
    glGenBuffers(1, &rayVBO);

    glBindVertexArray(rayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
    glBufferData(GL_ARRAY_BUFFER, rayVertices.size() * sizeof(float), rayVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glm::mat4 rayModel = glm::mat4(1.0f);
    GLint rayModelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(rayModelLoc, 1, GL_FALSE, glm::value_ptr(rayModel));

    GLint rayColorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(rayColorLoc, 1.0f, 1.0f, 1.0f); // White color for the ray

    glBindVertexArray(rayVAO);
    glDrawArrays(GL_LINES, 0, 2); // Draw the ray

    // Draw intersection point if it exists
    if (isIntersecting)
    {
        glm::mat4 intersectionModel = glm::translate(glm::mat4(1.0f), intersectionPoint);
        intersectionModel = glm::scale(intersectionModel, glm::vec3(0.01f)); // Adjust scale for intersection sphere
        GLint intersectionModelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(intersectionModelLoc, 1, GL_FALSE, glm::value_ptr(intersectionModel));

        GLint intersectionColorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(intersectionColorLoc, 0.0f, 1.0f, 0.0f); // Green color for the intersection point

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &rayVAO);
    glDeleteBuffers(1, &rayVBO);
}

void RayVsAABB( Ray ray, AABB aabb)
{
    boxmake();
    spheremake();  // Ensure the sphere for intersection point is created

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    Ray ray1 = ray;
    AABB box = aabb;

    // Animate the sphere moving left and right
    float time = static_cast<float>(glfwGetTime());
    if (animate)
    {
        // Animate the box moving left and right (opposite direction of the sphere)
        float boxOffset = sin(time) * 2.0f;
        box.center += glm::vec3(boxOffset, 0.0f, 0.0f);
        box.min = box.center - box.halfExtents;
        box.max = box.center + box.halfExtents;
    }
   

    // Check for intersection
    glm::vec3 intersectionPoint;
    bool isIntersecting = checkIntersection(ray1, box, intersectionPoint);

    // Draw AABB
    glm::mat4 model = glm::translate(glm::mat4(1.0f), box.center);
    model = glm::scale(model, box.halfExtents * 2.0f);
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc, isIntersecting ? 1.0f : 0.0f, isIntersecting ? 0.0f : 1.0f, 0.0f);

    glBindVertexArray(boxVAO);
    glDrawElements(GL_LINES, boxIndices.size(), GL_UNSIGNED_INT, 0);

    // Draw ray
    glm::vec3 rayEnd = ray1.start + ray1.direction * rayLength; // Extend the ray for visualization
    std::vector<float> rayVertices = {
        ray1.start.x, ray1.start.y, ray1.start.z,
        rayEnd.x, rayEnd.y, rayEnd.z
    };

    GLuint rayVBO, rayVAO;
    glGenVertexArrays(1, &rayVAO);
    glGenBuffers(1, &rayVBO);

    glBindVertexArray(rayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
    glBufferData(GL_ARRAY_BUFFER, rayVertices.size() * sizeof(float), rayVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glm::mat4 rayModel = glm::mat4(1.0f);
    GLint rayModelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(rayModelLoc, 1, GL_FALSE, glm::value_ptr(rayModel));

    GLint rayColorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(rayColorLoc, 1.0f, 1.0f, 1.0f); // White color for the ray

    glBindVertexArray(rayVAO);
    glDrawArrays(GL_LINES, 0, 2); // Draw the ray

    // Draw intersection point if it exists
    if (isIntersecting)
    {
        glm::mat4 intersectionModel = glm::translate(glm::mat4(1.0f), intersectionPoint);
        intersectionModel = glm::scale(intersectionModel, glm::vec3(0.01f)); // Adjust scale for intersection sphere
        GLint intersectionModelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(intersectionModelLoc, 1, GL_FALSE, glm::value_ptr(intersectionModel));

        GLint intersectionColorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(intersectionColorLoc, 0.0f, 1.0f, 0.0f); // Green color for the intersection point

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
    }

    // Cleanup
    glDeleteVertexArrays(1, &boxVAO);
    glDeleteBuffers(1, &boxVBO);
    glDeleteBuffers(1, &boxEBO);
    glDeleteVertexArrays(1, &rayVAO);
    glDeleteBuffers(1, &rayVBO);
}

void RayVsSphere( Ray ray, Sphere sphere1)
{
    spheremake();

    // Enable depth test
    glEnable(GL_DEPTH_TEST);

    Ray ray1 = ray;
    Sphere sphere = sphere1;

    // Animate the sphere moving left and right
    float time = static_cast<float>(glfwGetTime());
    if (animate)
    {
        sphere.position.x = -sin(time) * 2.0f;
    }
   
    // Check for intersection
    glm::vec3 intersectionPoint;
    bool isIntersecting = checkIntersection(ray1, sphere, intersectionPoint);

    // Draw sphere
    glm::mat4 model = glm::translate(glm::mat4(1.0f), sphere.position);
    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(sphere.radius));
    model = model * scale;
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    GLint colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(colorLoc, isIntersecting ? 1.0f : 0.0f, isIntersecting ? 0.0f : 1.0f, 0.0f);

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_LINES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

    // Draw ray
    glm::vec3 rayEnd = ray1.start + ray1.direction * rayLength; // Extend the ray for visualization
    std::vector<float> rayVertices = {
        ray1.start.x, ray1.start.y, ray1.start.z,
        rayEnd.x, rayEnd.y, rayEnd.z
    };

    GLuint rayVBO, rayVAO;
    glGenVertexArrays(1, &rayVAO);
    glGenBuffers(1, &rayVBO);

    glBindVertexArray(rayVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rayVBO);
    glBufferData(GL_ARRAY_BUFFER, rayVertices.size() * sizeof(float), rayVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glm::mat4 rayModel = glm::mat4(1.0f);
    GLint rayModelLoc = glGetUniformLocation(shaderProgram, "model");
    glUniformMatrix4fv(rayModelLoc, 1, GL_FALSE, glm::value_ptr(rayModel));

    GLint rayColorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3f(rayColorLoc, 1.0f, 1.0f, 1.0f); // White color for the ray

    glBindVertexArray(rayVAO);
    glDrawArrays(GL_LINES, 0, 2); // Draw the ray

    // Draw intersection point if it exists
    if (isIntersecting)
    {
        glm::mat4 intersectionModel = glm::translate(glm::mat4(1.0f), intersectionPoint);
        intersectionModel = glm::scale(intersectionModel, glm::vec3(0.01f)); // Adjust scale for intersection sphere
        GLint intersectionModelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(intersectionModelLoc, 1, GL_FALSE, glm::value_ptr(intersectionModel));

        GLint intersectionColorLoc = glGetUniformLocation(shaderProgram, "color");
        glUniform3f(intersectionColorLoc, 0.0f, 1.0f, 0.0f); // Green color for the intersection point

        glBindVertexArray(sphereVAO);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
    }

    // Cleanup
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteVertexArrays(1, &rayVAO);
    glDeleteBuffers(1, &rayVBO);
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

void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, int sectorCount, int stackCount)
{
    const float PI = 3.14159265359f;
    float x, y, z, xy;                            // vertex position
    float radius = 1.0f; // unit sphere

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

void rungenerateSphere()
{
    generateSphere(sphereVertices, sphereIndices, 36, 18);
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
    return glm::abs(distance) < 0.1f; // Epsilon tolerance
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
    float distance = glm::dot(plane.normal, glm::vec4(sphere.position, 1.f) - plane.normal.w);

    // Check if the absolute distance is less than or equal to the sphere's radius
    return abs(distance) <= sphere.radius;

}

bool checkIntersection(const Ray& ray, const Plane& plane, glm::vec3& intersectionPoint)
{
    float denom = glm::dot(glm::vec3(plane.normal), ray.direction);
    if (fabs(denom) > 1e-6) {
        float t = -(glm::dot(glm::vec3(plane.normal), ray.start) + plane.normal.w) / denom;
        if (t >= 0) {
            intersectionPoint = ray.start + t * ray.direction;
            return true;
        }
    }
    return false;
}

bool checkIntersection(const Ray& ray, const Triangle& triangle, glm::vec3& intersectionPoint)
{
    glm::vec3 v0v1 = triangle.v2 - triangle.v1;
    glm::vec3 v0v2 = triangle.v3 - triangle.v1;
    glm::vec3 pvec = glm::cross(ray.direction, v0v2);
    float det = glm::dot(v0v1, pvec);

    if (fabs(det) < 1e-8) return false; // Ray and triangle are parallel if det is close to 0

    float invDet = 1 / det;

    glm::vec3 tvec = ray.start - triangle.v1;
    float u = glm::dot(tvec, pvec) * invDet;
    if (u < 0 || u > 1) return false;

    glm::vec3 qvec = glm::cross(tvec, v0v1);
    float v = glm::dot(ray.direction, qvec) * invDet;
    if (v < 0 || u + v > 1) return false;

    float t = glm::dot(v0v2, qvec) * invDet;

    if (t < 0) return false; // Intersection point is behind the ray start

    intersectionPoint = ray.start + t * ray.direction;
    return true;
}

bool checkIntersection(const Ray& ray, const AABB& box, glm::vec3& intersectionPoint)
{
    // Calculate min and max using center and halfExtents
    glm::vec3 boxMin = box.center - box.halfExtents;
    glm::vec3 boxMax = box.center + box.halfExtents;

    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t0s = (boxMin - ray.start) * invDir;
    glm::vec3 t1s = (boxMax - ray.start) * invDir;
    glm::vec3 tmins = glm::min(t0s, t1s);
    glm::vec3 tmaxs = glm::max(t0s, t1s);
    float tmin = glm::max(glm::max(tmins.x, tmins.y), tmins.z);
    float tmax = glm::min(glm::min(tmaxs.x, tmaxs.y), tmaxs.z);
    if (tmax < 0 || tmin > tmax)
    {
        return false;
    }
    intersectionPoint = ray.start + tmin * ray.direction;
    return true;
}

bool checkIntersection(const Ray& ray, const Sphere& sphere, glm::vec3& intersectionPoint)
{
    glm::vec3 oc = ray.start - sphere.position;
    float a = glm::dot(ray.direction, ray.direction);
    float b = 2.0f * glm::dot(oc, ray.direction);
    float c = glm::dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0)
    {
        return false;
    }
    else
    {
        float t = (-b - sqrt(discriminant)) / (2.0f * a);
        if (t < 0)
        {
            t = (-b + sqrt(discriminant)) / (2.0f * a);
        }
        if (t >= 0)
        {
            intersectionPoint = ray.start + t * ray.direction;
            return true;
        }
        return false;
    }
}