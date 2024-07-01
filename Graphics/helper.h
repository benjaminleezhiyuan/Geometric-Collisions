#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include "classes.h"

extern GLuint shaderProgram;
extern GLuint bvShaderProgram;

extern glm::mat4 view;
extern glm::mat4 projection;

extern GLFWwindow* window;

void shader();
void bvShader();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
