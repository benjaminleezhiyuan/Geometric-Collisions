#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include "classes.h"


void shader();
void SphereVsSphere(GLFWwindow* window, float radius1, float radius2);
void AABBvsSphere(GLFWwindow* window, float radius, const glm::vec3& initialBoxCenter, const glm::vec3& initialBoxHalfExtents);
void AABBvsAABB(GLFWwindow* window, const glm::vec3& initialBox1Center, const glm::vec3& initialBox1HalfExtents, const glm::vec3& initialBox2Center, const glm::vec3& initialBox2HalfExtents); void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
bool checkIntersection(const Sphere& sphere1, const Sphere& sphere2);
bool checkIntersection(const Sphere& sphere, const AABB& box);
bool checkIntersection(const AABB& box1, const AABB& box2);
void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, float radius, int sectorCount, int stackCount);