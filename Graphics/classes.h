#pragma once
#include <glm/glm.hpp>
struct Point
{
    glm::vec3 coordinates;
};

struct Plane
{
    glm::vec4 normal;
};

struct Triangle
{
    glm::vec3 v1;
    glm::vec3 v2;
    glm::vec3 v3;
};

struct Sphere
{
    glm::vec3 position;
    float radius;
};

struct AABB
{
    glm::vec3 center;
    glm::vec3 halfExtents;
    glm::vec3 min;
    glm::vec3 max;
};

struct Ray
{
    glm::vec3 start;
    glm::vec3 direction;
};