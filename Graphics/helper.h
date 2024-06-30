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
extern GLuint boundingshaderProgram;

extern glm::mat4 view;
extern glm::mat4 projection;
extern float rayLength;
extern GLFWwindow* window;
extern bool animate;

void shader();
void boundingshader();
void spheremake();
void boxmake();
void SphereVsSphere(Sphere Sphere1, Sphere Sphere2);
void AABBVsSphere( AABB aabb, Sphere Sphere1);
void SphereVsAABB( Sphere Sphere1, AABB aabb);
void AABBvsAABB( AABB aabb1, AABB aabb2);
void PointVsSphere( Point point, Sphere sphere1);
void PointVsAABB( Point point1, AABB aabb);
void PointVsPlane( Point point1, Plane plane1);
void PointVsTriangle( Point point1, Triangle triangle);
void PlaneVsAABB( Plane plane1, AABB aabb);
void PlaneVsSphere( Plane plane1, Sphere sphere1);
void RayVsPlane( Ray ray, Plane plane);
void RayVsTriangle( Ray ray, Triangle triangle);
void RayVsAABB( Ray ray, AABB aabb);
void RayVsSphere( Ray ray, Sphere sphere1);


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
bool checkIntersection(const Ray& ray, const Plane& plane, glm::vec3& intersectionPoint);
bool checkIntersection(const Ray& ray, const Triangle& triangle, glm::vec3& intersectionPoint);
bool checkIntersection(const Ray& ray, const AABB& box, glm::vec3& intersectionPoint);
bool checkIntersection(const Ray& ray, const Sphere& sphere, glm::vec3& intersectionPoint);


void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices, int sectorCount, int stackCount);
void rungenerateSphere();
