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
void spheremake();
void boxmake();
void SphereVsSphere(GLFWwindow* window, Sphere Sphere1, Sphere Sphere2);
void AABBVsSphere(GLFWwindow* window, AABB aabb, Sphere Sphere1);
void SphereVsAABB(GLFWwindow* window, Sphere Sphere1, AABB aabb);
void AABBvsAABB(GLFWwindow* window, AABB aabb1, AABB aabb2);
void PointVsSphere(GLFWwindow* window, Point point, Sphere sphere1);
void PointVsAABB(GLFWwindow* window, Point point1, AABB aabb);
void PointVsPlane(GLFWwindow* window, Point point1, Plane plane1);
void PointVsTriangle(GLFWwindow* window, Point point1, Triangle triangle);
void PlaneVsAABB(GLFWwindow* window, Plane plane1, AABB aabb);
void PlaneVsSphere(GLFWwindow* window, Plane plane1, Sphere sphere1);


void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

bool checkIntersection(const Sphere& sphere1, const Sphere& sphere2);
bool checkIntersection(const Sphere& sphere, const AABB& box);
bool checkIntersection(const AABB& box1, const AABB& box2);
bool checkIntersection(const Point& point, const Sphere& sphere);
bool checkIntersection(const Point& point, const AABB& box);
bool checkIntersection(const Point& point, const Plane& plane);
bool checkIntersection(const Point& point, const Triangle& triangle);
bool checkIntersection(const Plane& plane, const AABB& box);
bool checkIntersection(const Plane& plane, const Sphere& sphere);


void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, int sectorCount, int stackCount);
void rungenerateSphere();
