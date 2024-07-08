#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include <iostream>
#include "helper.h"
#include "OBJ_Loader.h"
#include <limits>
#include <cmath>
#include <glm/gtx/norm.hpp> // for distance2



#define MIN_OBJECTS_AT_LEAF 1

GLFWwindow* window;

GLuint shaderProgram;
GLuint bvShaderProgram;

std::vector<glm::vec3> levelColors = {
    {1.0f, 0.0f, 0.0f}, // Red
    {0.0f, 1.0f, 0.0f}, // Green
    {0.0f, 0.0f, 1.0f}, // Blue
    {1.0f, 1.0f, 0.0f}, // Yellow
    {0.0f, 1.0f, 1.0f}, // Cyan
    {1.0f, 0.0f, 1.0f}, // Magenta
    {1.0f, 0.5f, 0.0f}, // Orange
    {0.5f, 0.0f, 1.0f}, // Purple
    {0.0f, 0.5f, 0.5f}  // Teal
};

objl::Loader loader;

// Camera parameters
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 20.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
bool firstMouse = true;     
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 400, lastY = 300;
float fov = 45.0f;
bool rightMouseButtonPressed = false;
float cameraSpeed = 2.5f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Containers for multiple models
std::vector<GLuint> VAOs, VBOs, EBOs;
std::vector<objl::Mesh> meshes;
std::vector<float> scales;

// Define the AABB structure
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

struct BoundingSphere {
    glm::vec3 center;
    float radius;
};

struct Object {
    AABB boundingBox;
    BoundingSphere ritterSphere;
    BoundingSphere larssonSphere;
    BoundingSphere pcaSphere;
    objl::Mesh mesh; // Store the mesh for access to vertices
};

std::vector<Object> objects;
enum NodeType { INTERNAL, LEAF };

AABB MergeAABB(const AABB& a, const AABB& b) {
    AABB result;
    result.min = glm::min(a.min, b.min);
    result.max = glm::max(a.max, b.max);
    return result;
}

BoundingSphere MergeBoundingSpheres(const BoundingSphere& a, const BoundingSphere& b) {
    glm::vec3 d = b.center - a.center;
    float dist = glm::length(d);

    if (dist + b.radius <= a.radius) {
        // Sphere b is entirely within sphere a
        return a;
    }

    if (dist + a.radius <= b.radius) {
        // Sphere a is entirely within sphere b
        return b;
    }

    // Otherwise, compute the new sphere that minimally bounds both spheres
    float newRadius = (dist + a.radius + b.radius) * 0.5f;
    glm::vec3 newCenter = a.center;

    if (dist > 0.0f) {
        newCenter += d * ((newRadius - a.radius) / dist);
    }

    return { newCenter, newRadius };
}

struct TreeNode {
    NodeType type;
    AABB aabbVolume;
    BoundingSphere ritterVolume;
    BoundingSphere larssonVolume;
    BoundingSphere pcaVolume;

    Object* objects; // pointer to objects/BVs that the node represents
    int numObjects; // How many objects in this subtree?
    TreeNode* lChild;
    TreeNode* rChild;

    // Constructor for leaf nodes
    TreeNode() : type(LEAF), objects(nullptr), numObjects(0), lChild(nullptr), rChild(nullptr) {}

    // Constructor for internal nodes
    TreeNode(TreeNode* left, TreeNode* right) : type(INTERNAL), lChild(left), rChild(right) {
        // Merge the volumes of left and right children
        aabbVolume = MergeAABB(left->aabbVolume, right->aabbVolume);
        ritterVolume = MergeBoundingSpheres(left->ritterVolume, right->ritterVolume);
        larssonVolume = MergeBoundingSpheres(left->larssonVolume, right->larssonVolume);
        pcaVolume = MergeBoundingSpheres(left->pcaVolume, right->pcaVolume);
    }
};

enum ConstructionMethod {
    CM_TOP_DOWN,
    CM_BOTTOM_UP
};

// Add this enumeration to your global scope
enum BoundingVolumeType {
    BVT_NONE,
    BVT_AABB,
    BVT_RITTER_SPHERE,
    BVT_LARSSON_SPHERE,
    BVT_PCA_SPHERE
};

struct BoundingVolumeCost {
    float distance;
    float combinedVolume;
    float relativeVolumeIncrease;
};

enum SplitMethod {
    SM_MEDIAN_CENTER,
    SM_MEDIAN_EXTENT,
    SM_K_EVEN_SPLITS
};

SplitMethod currentSplitMethod = SM_MEDIAN_CENTER; // Default split method
int kSplits = 2; // Default number of even splits

ConstructionMethod currentMethod = CM_TOP_DOWN;
BoundingVolumeType currentBVType = BVT_NONE;
bool displayAllLevels = false;
int currentLevel = 0;
bool maxHeight = true;
int maxHeightValue = 7;
bool rebuildTree = true;
bool prevMaxHeight = maxHeight; // Track the previous state of maxHeight checkbox

BoundingSphere ComputeRitterSphere(const std::vector<Object>& objects);
BoundingSphere ComputeLarssonSphere(const std::vector<Object>& objects);
BoundingSphere ComputePCASphere(const std::vector<Object>& objects);

float Volume(const AABB& aabb) {
    glm::vec3 size = aabb.max - aabb.min;
    return size.x * size.y * size.z;
}

BoundingVolumeCost CalculateBoundingVolumeCost(const TreeNode* a, const TreeNode* b) {
    glm::vec3 centerA = (a->aabbVolume.min + a->aabbVolume.max) * 0.5f;
    glm::vec3 centerB = (b->aabbVolume.min + b->aabbVolume.max) * 0.5f;
    float distance = glm::distance(centerA, centerB);

    AABB mergedAABB = MergeAABB(a->aabbVolume, b->aabbVolume);
    float combinedVolume = Volume(mergedAABB);

    float volumeA = Volume(a->aabbVolume);
    float volumeB = Volume(b->aabbVolume);
    float relativeVolumeIncrease = (combinedVolume - (volumeA + volumeB)) / (volumeA + volumeB);

    return { distance, combinedVolume, relativeVolumeIncrease };
}

void FindNodesToMerge(std::vector<TreeNode*>& nodes, TreeNode*& first, TreeNode*& second) {
    float minCost = std::numeric_limits<float>::max();
    int firstIndex = -1;
    int secondIndex = -1;

    for (size_t i = 0; i < nodes.size(); ++i) {
        for (size_t j = i + 1; j < nodes.size(); ++j) {
            BoundingVolumeCost cost = CalculateBoundingVolumeCost(nodes[i], nodes[j]);

            // Combine the costs into a single heuristic value
            float combinedCost = cost.distance + cost.combinedVolume + cost.relativeVolumeIncrease;

            if (combinedCost < minCost) {
                minCost = combinedCost;
                firstIndex = i;
                secondIndex = j;
            }
        }
    }

    first = nodes[firstIndex];
    second = nodes[secondIndex];
    nodes.erase(nodes.begin() + secondIndex); // Erase the second node first
    nodes.erase(nodes.begin() + firstIndex);  // Then erase the first node
}

TreeNode* BottomUpTree(std::vector<TreeNode*>& nodes) {
    while (nodes.size() > 1) {
        TreeNode* first, * second;
        FindNodesToMerge(nodes, first, second);
        TreeNode* parent = new TreeNode(first, second);
        nodes.push_back(parent);
    }
    return nodes[0]; // return the root node
}

AABB ComputeAABB(const std::vector<Object>& objects) {
    AABB bv;
    bv.min = glm::vec3(std::numeric_limits<float>::max());
    bv.max = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& obj : objects) {
        bv.min = glm::min(bv.min, obj.boundingBox.min);
        bv.max = glm::max(bv.max, obj.boundingBox.max);
    }

    return bv;
}

BoundingSphere ComputeBV(const std::vector<Object>& objects, BoundingVolumeType bvType) {
    switch (bvType) {
    case BVT_RITTER_SPHERE:
        return ComputeRitterSphere(objects);
    case BVT_LARSSON_SPHERE:
        return ComputeLarssonSphere(objects);
    case BVT_PCA_SPHERE:
        return ComputePCASphere(objects);
    default:
        throw std::runtime_error("Unknown Bounding Volume Type");
    }
}

std::vector<TreeNode*> InitializeLeafNodes(const std::vector<Object>& objects) {
    std::vector<TreeNode*> nodes;
    for (const auto& obj : objects) {
        TreeNode* node = new TreeNode();
        node->aabbVolume = obj.boundingBox;
        node->ritterVolume = obj.ritterSphere;
        node->larssonVolume = obj.larssonSphere;
        node->pcaVolume = obj.pcaSphere;
        node->objects = const_cast<Object*>(&obj); // Leaf nodes contain the actual objects
        node->numObjects = 1;
        nodes.push_back(node);
    }
    return nodes;
}

int PartitionObjects(std::vector<Object>& objects, int axis, SplitMethod splitMethod, int k = 2) {
    int numObjects = objects.size();
    if (numObjects <= 1) {
        return 0;
    }

    if (splitMethod == SM_MEDIAN_CENTER) {
        // Sort objects based on the center of their bounding volumes along the given axis
        if (axis == 0) {
            std::nth_element(objects.begin(), objects.begin() + numObjects / 2, objects.end(),
                [](const Object& a, const Object& b) {
                    return (a.boundingBox.min.x + a.boundingBox.max.x) < (b.boundingBox.min.x + b.boundingBox.max.x);
                });
        }
        else if (axis == 1) {
            std::nth_element(objects.begin(), objects.begin() + numObjects / 2, objects.end(),
                [](const Object& a, const Object& b) {
                    return (a.boundingBox.min.y + a.boundingBox.max.y) < (b.boundingBox.min.y + b.boundingBox.max.y);
                });
        }
        else {
            std::nth_element(objects.begin(), objects.begin() + numObjects / 2, objects.end(),
                [](const Object& a, const Object& b) {
                    return (a.boundingBox.min.z + a.boundingBox.max.z) < (b.boundingBox.min.z + b.boundingBox.max.z);
                });
        }
        return numObjects / 2;
    }
    else if (splitMethod == SM_MEDIAN_EXTENT) {
        // Sort objects based on the extents of their bounding volumes along the given axis
        if (axis == 0) {
            std::nth_element(objects.begin(), objects.begin() + numObjects / 2, objects.end(),
                [](const Object& a, const Object& b) {
                    return a.boundingBox.max.x < b.boundingBox.max.x;
                });
        }
        else if (axis == 1) {
            std::nth_element(objects.begin(), objects.begin() + numObjects / 2, objects.end(),
                [](const Object& a, const Object& b) {
                    return a.boundingBox.max.y < b.boundingBox.max.y;
                });
        }
        else {
            std::nth_element(objects.begin(), objects.begin() + numObjects / 2, objects.end(),
                [](const Object& a, const Object& b) {
                    return a.boundingBox.max.z < b.boundingBox.max.z;
                });
        }
        return numObjects / 2;
    }
    else if (splitMethod == SM_K_EVEN_SPLITS) {
        int k = static_cast<int>(numObjects * 0.7f); // Default is splitRatio = 0.5 for balanced split

        if (axis == 0) {
            std::nth_element(objects.begin(), objects.begin() + k, objects.end(), [](const Object& a, const Object& b) {
                return a.boundingBox.min.x < b.boundingBox.min.x;
                });
        }
        else if (axis == 1) {
            std::nth_element(objects.begin(), objects.begin() + k, objects.end(), [](const Object& a, const Object& b) {
                return a.boundingBox.min.y < b.boundingBox.min.y;
                });
        }
        else {
            std::nth_element(objects.begin(), objects.begin() + k, objects.end(), [](const Object& a, const Object& b) {
                return a.boundingBox.min.z < b.boundingBox.min.z;
                });
        }

        return k;
    }

    return numObjects / 2; // Default split point
}

void TopDownTree(TreeNode* node, std::vector<Object>& objects, int depth, int maxheightV) {
    node->aabbVolume = ComputeAABB(objects);
    node->ritterVolume = ComputeBV(objects, BVT_RITTER_SPHERE);
    node->larssonVolume = ComputeBV(objects, BVT_LARSSON_SPHERE);
    node->pcaVolume = ComputeBV(objects, BVT_PCA_SPHERE);

    if (objects.size() <= MIN_OBJECTS_AT_LEAF || (depth >= maxheightV && maxHeight)) {
        node->type = LEAF;
        node->objects = objects.data();
        node->numObjects = objects.size();
        node->lChild = nullptr;
        node->rChild = nullptr;
    }
    else {
        node->type = INTERNAL;
        int axis = depth % 3; // Alternate between x, y, and z axes
        int medianIndex = PartitionObjects(objects, axis, currentSplitMethod, kSplits);

        node->lChild = new TreeNode();
        node->rChild = new TreeNode();

        std::vector<Object> leftObjects(objects.begin(), objects.begin() + medianIndex);
        std::vector<Object> rightObjects(objects.begin() + medianIndex, objects.end());

        TopDownTree(node->lChild, leftObjects, depth + 1, maxheightV);
        TopDownTree(node->rChild, rightObjects, depth + 1, maxheightV);
    }
}

AABB ComputeAABB(const objl::Mesh& mesh) {
    AABB aabb;
    aabb.min = glm::vec3(std::numeric_limits<float>::max());
    aabb.max = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& vertex : mesh.Vertices) {
        aabb.min = glm::min(aabb.min, glm::vec3(vertex.Position.X, vertex.Position.Y, vertex.Position.Z));
        aabb.max = glm::max(aabb.max, glm::vec3(vertex.Position.X, vertex.Position.Y, vertex.Position.Z));
    }

    return aabb;
}

void CreateAABBVertices(const AABB& aabb, std::vector<glm::vec3>& vertices, std::vector<GLuint>& indices) {
    glm::vec3 min = aabb.min;
    glm::vec3 max = aabb.max;

    vertices = {
        min,
        glm::vec3(max.x, min.y, min.z),
        glm::vec3(max.x, max.y, min.z),
        glm::vec3(min.x, max.y, min.z),
        glm::vec3(min.x, min.y, max.z),
        glm::vec3(max.x, min.y, max.z),
        glm::vec3(max.x, max.y, max.z),
        glm::vec3(min.x, max.y, max.z)
    };

    indices = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7
    };
}

void CreateSphereVertices(BoundingSphere sphere, std::vector<glm::vec3>& vertices, std::vector<GLuint>& indices) {
    const unsigned int X_SEGMENTS = 16;
    const unsigned int Y_SEGMENTS = 16;

    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;
            float xPos = sphere.center.x + sphere.radius * std::cos(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());
            float yPos = sphere.center.y + sphere.radius * std::cos(ySegment * glm::pi<float>());
            float zPos = sphere.center.z + sphere.radius * std::sin(xSegment * 2.0f * glm::pi<float>()) * std::sin(ySegment * glm::pi<float>());

            vertices.push_back(glm::vec3(xPos, yPos, zPos));
        }
    }

    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);

            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
            indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
        }
    }
}

BoundingSphere ComputeRitterSphere(const std::vector<Object>& objects) {
    std::vector<glm::vec3> points;
    for (const auto& obj : objects) {
        for (const auto& vertex : obj.mesh.Vertices) {
            points.emplace_back(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
        }
    }

    if (points.empty()) return { glm::vec3(0.0f), 0.0f };

    glm::vec3 x = points[0];
    glm::vec3 y = points[0];
    float maxDistSq = 0;

    for (const auto& p : points) {
        float distSq = glm::distance2(p, x);
        if (distSq > maxDistSq) {
            y = p;
            maxDistSq = distSq;
        }
    }

    glm::vec3 z = y;
    maxDistSq = 0;
    for (const auto& p : points) {
        float distSq = glm::distance2(p, y);
        if (distSq > maxDistSq) {
            z = p;
            maxDistSq = distSq;
        }
    }

    glm::vec3 center = (y + z) * 0.5f;
    float radius = glm::distance(y, z) * 0.5f;

    for (const auto& p : points) {
        float dist = glm::distance(center, p);
        if (dist > radius) {
            float newRadius = (radius + dist) * 0.5f;
            float k = (newRadius - radius) / dist;
            radius = newRadius;
            center += k * (p - center);
        }
    }

    return { center, radius };
}

BoundingSphere ComputeLarssonSphere(const std::vector<Object>& objects) {
    std::vector<glm::vec3> points;
    for (const auto& obj : objects) {
        for (const auto& vertex : obj.mesh.Vertices) {
            points.emplace_back(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
        }
    }

    if (points.empty()) return { glm::vec3(0.0f), 0.0f };

    glm::vec3 minPoint = points[0];
    glm::vec3 maxPoint = points[0];

    for (const auto& point : points) {
        minPoint = glm::min(minPoint, point);
        maxPoint = glm::max(maxPoint, point);
    }

    glm::vec3 center = (minPoint + maxPoint) * 0.5f;
    float radius = 0.0f;

    for (const auto& point : points) {
        float dist = glm::distance(center, point);
        if (dist > radius) {
            radius = dist;
        }
    }

    for (const auto& point : points) {
        float dist = glm::distance(center, point);
        if (dist > radius) {
            float newRadius = (radius + dist) * 0.5f;
            float k = (newRadius - radius) / dist;
            radius = newRadius;
            center += k * (point - center);
        }
    }

    return { center, radius };
}

BoundingSphere ComputePCASphere(const std::vector<Object>& objects) {
    std::vector<glm::vec3> points;
    for (const auto& obj : objects) {
        for (const auto& vertex : obj.mesh.Vertices) {
            points.emplace_back(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
        }
    }

    if (points.empty()) return { glm::vec3(0.0f), 0.0f };

    glm::vec3 mean(0.0f);
    for (const auto& p : points) {
        mean += p;
    }
    mean /= static_cast<float>(points.size());

    glm::mat3 covariance(0.0f);
    for (const auto& p : points) {
        glm::vec3 centered = p - mean;
        covariance[0][0] += centered.x * centered.x;
        covariance[0][1] += centered.x * centered.y;
        covariance[0][2] += centered.x * centered.z;
        covariance[1][0] += centered.y * centered.x;
        covariance[1][1] += centered.y * centered.y;
        covariance[1][2] += centered.y * centered.z;
        covariance[2][0] += centered.z * centered.x;
        covariance[2][1] += centered.z * centered.y;
        covariance[2][2] += centered.z * centered.z;
    }
    covariance /= static_cast<float>(points.size());

    glm::vec3 eigenvalues;
    glm::mat3 eigenvectors;
    glm::vec3 v(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < 10; ++i) {
        v = covariance * v;
        v = glm::normalize(v);
    }
    glm::vec3 principalComponent = v;

    float minProj = std::numeric_limits<float>::max();
    float maxProj = std::numeric_limits<float>::lowest();
    for (const auto& p : points) {
        float proj = glm::dot(p - mean, principalComponent);
        minProj = std::min(minProj, proj);
        maxProj = std::max(maxProj, proj);
    }

    glm::vec3 center = mean + principalComponent * (minProj + maxProj) * 0.5f;
    float radius = (maxProj - minProj) * 0.5f;

    return { center, radius };
}

BoundingSphere ComputeRitterSphere(const objl::Mesh& mesh) {
    std::vector<glm::vec3> points;
    for (const auto& vertex : mesh.Vertices) {
        points.emplace_back(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
    }

    if (points.empty()) return { glm::vec3(0.0f), 0.0f };

    glm::vec3 x = points[0];
    glm::vec3 y = points[0];
    float maxDistSq = 0;

    for (const auto& p : points) {
        float distSq = glm::distance2(p, x);
        if (distSq > maxDistSq) {
            y = p;
            maxDistSq = distSq;
        }
    }

    glm::vec3 z = y;
    maxDistSq = 0;
    for (const auto& p : points) {
        float distSq = glm::distance2(p, y);
        if (distSq > maxDistSq) {
            z = p;
            maxDistSq = distSq;
        }
    }

    glm::vec3 center = (y + z) * 0.5f;
    float radius = glm::distance(y, z) * 0.5f;

    for (const auto& p : points) {
        float dist = glm::distance(center, p);
        if (dist > radius) {
            float newRadius = (radius + dist) * 0.5f;
            float k = (newRadius - radius) / dist;
            radius = newRadius;
            center += k * (p - center);
        }
    }

    return { center, radius };
}

BoundingSphere ComputeLarssonSphere(const objl::Mesh& mesh) {
    std::vector<glm::vec3> points;
    for (const auto& vertex : mesh.Vertices) {
        points.emplace_back(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
    }

    if (points.empty()) return { glm::vec3(0.0f), 0.0f };

    glm::vec3 minPoint = points[0];
    glm::vec3 maxPoint = points[0];

    for (const auto& point : points) {
        minPoint = glm::min(minPoint, point);
        maxPoint = glm::max(maxPoint, point);
    }

    glm::vec3 center = (minPoint + maxPoint) * 0.5f;
    float radius = 0.0f;

    for (const auto& point : points) {
        float dist = glm::distance(center, point);
        if (dist > radius) {
            radius = dist;
        }
    }

    for (const auto& point : points) {
        float dist = glm::distance(center, point);
        if (dist > radius) {
            float newRadius = (radius + dist) * 0.5f;
            float k = (newRadius - radius) / dist;
            radius = newRadius;
            center += k * (point - center);
        }
    }

    return { center, radius };
}

BoundingSphere ComputePCASphere(const objl::Mesh& mesh) {
    std::vector<glm::vec3> points;
    for (const auto& vertex : mesh.Vertices) {
        points.emplace_back(vertex.Position.X, vertex.Position.Y, vertex.Position.Z);
    }

    if (points.empty()) return { glm::vec3(0.0f), 0.0f };

    glm::vec3 mean(0.0f);
    for (const auto& p : points) {
        mean += p;
    }
    mean /= static_cast<float>(points.size());

    glm::mat3 covariance(0.0f);
    for (const auto& p : points) {
        glm::vec3 centered = p - mean;
        covariance[0][0] += centered.x * centered.x;
        covariance[0][1] += centered.x * centered.y;
        covariance[0][2] += centered.x * centered.z;
        covariance[1][0] += centered.y * centered.x;
        covariance[1][1] += centered.y * centered.y;
        covariance[1][2] += centered.y * centered.z;
        covariance[2][0] += centered.z * centered.x;
        covariance[2][1] += centered.z * centered.y;
        covariance[2][2] += centered.z * centered.z;
    }
    covariance /= static_cast<float>(points.size());

    glm::vec3 eigenvalues;
    glm::mat3 eigenvectors;
    glm::vec3 v(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < 10; ++i) {
        v = covariance * v;
        v = glm::normalize(v);
    }
    glm::vec3 principalComponent = v;

    float minProj = std::numeric_limits<float>::max();
    float maxProj = std::numeric_limits<float>::lowest();
    for (const auto& p : points) {
        float proj = glm::dot(p - mean, principalComponent);
        minProj = std::min(minProj, proj);
        maxProj = std::max(maxProj, proj);
    }

    glm::vec3 center = mean + principalComponent * (minProj + maxProj) * 0.5f;
    float radius = (maxProj - minProj) * 0.5f;

    return { center, radius };
}

void loadModel(const std::string& path, float scale) {
    objl::Loader loader;
    if (loader.LoadFile(path)) {
        for (auto& mesh : loader.LoadedMeshes) {
            // Create a new Object for each mesh
            Object obj;
            obj.boundingBox = ComputeAABB(mesh);
            
            // Calculate bounding spheres using all three methods
            obj.mesh = mesh;
            obj.ritterSphere = ComputeRitterSphere(mesh);
            obj.larssonSphere = ComputeLarssonSphere(mesh);
            obj.pcaSphere = ComputePCASphere(mesh);
            // Add the object to the vector
            objects.push_back(obj);

            meshes.push_back(mesh);
            unsigned int VAO, VBO, EBO;

            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, mesh.Vertices.size() * sizeof(objl::Vertex), &mesh.Vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.Indices.size() * sizeof(unsigned int), &mesh.Indices[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);

            glBindVertexArray(0);

            VAOs.push_back(VAO);
            VBOs.push_back(VBO);
            EBOs.push_back(EBO);
            scales.push_back(scale);
        }
    }
}

void loadModelsFromDirectory(const std::string& directory, float scale) {
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.path().extension() == ".obj") {
            std::cout << "Loading " << entry.path().string() << std::endl;
            loadModel(entry.path().string(), scale);
        }
    }
}

// Mouse callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (rightMouseButtonPressed) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        float sensitivity = 0.1f;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);
    }
}

// Mouse button callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            rightMouseButtonPressed = true;
        }
        else if (action == GLFW_RELEASE) {
            rightMouseButtonPressed = false;
            firstMouse = true;
        }
    }
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    float speed = cameraSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += speed * cameraUp; // Move up
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        cameraPos -= speed * cameraUp; // Move down
}

int TreeDepth(TreeNode* node) {
    if (!node) {
        return 0;
    }

    int leftDepth = TreeDepth(node->lChild);
    int rightDepth = TreeDepth(node->rChild);

    return std::max(leftDepth, rightDepth) + 1;
}

void DrawBoundingVolumes(TreeNode* node, GLuint bvShaderProgram, bool drawAllLevels, int targetLevel, int currentLevel = 0) {
    if (!node) return;

    glm::vec3 color = levelColors[currentLevel % levelColors.size()];

    if (drawAllLevels || currentLevel == targetLevel) {
        if (currentBVType == BVT_AABB) {
            std::vector<glm::vec3> vertices;
            std::vector<GLuint> indices;
            CreateAABBVertices(node->aabbVolume, vertices, indices);

            GLuint bboxVAO, bboxVBO, bboxEBO;
            glGenVertexArrays(1, &bboxVAO);
            glGenBuffers(1, &bboxVBO);
            glGenBuffers(1, &bboxEBO);

            glBindVertexArray(bboxVAO);

            glBindBuffer(GL_ARRAY_BUFFER, bboxVBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bboxEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glUniform3f(glGetUniformLocation(bvShaderProgram, "boundingVolumeColor"), color.r, color.g, color.b); // Set AABB color
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(0.0001f, 0.0001f, 0.0001f));
            int modelLoc = glGetUniformLocation(bvShaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(bboxVAO);
            glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glDeleteVertexArrays(1, &bboxVAO);
            glDeleteBuffers(1, &bboxVBO);
            glDeleteBuffers(1, &bboxEBO);
        }
        else if (currentBVType == BVT_RITTER_SPHERE) {
            std::vector<glm::vec3> vertices;
            std::vector<GLuint> indices;

            CreateSphereVertices(node->ritterVolume, vertices, indices);

            GLuint sphereVAO, sphereVBO, sphereEBO;
            glGenVertexArrays(1, &sphereVAO);
            glGenBuffers(1, &sphereVBO);
            glGenBuffers(1, &sphereEBO);

            glBindVertexArray(sphereVAO);

            glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glUniform3f(glGetUniformLocation(bvShaderProgram, "boundingVolumeColor"), color.r, color.g, color.b); // Set sphere color
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(0.0001f, 0.0001f, 0.0001f));
            int modelLoc = glGetUniformLocation(bvShaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(sphereVAO);
            glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glDeleteVertexArrays(1, &sphereVAO);
            glDeleteBuffers(1, &sphereVBO);
            glDeleteBuffers(1, &sphereEBO);
        }
        else if (currentBVType == BVT_LARSSON_SPHERE) {
            std::vector<glm::vec3> vertices;
            std::vector<GLuint> indices;

            CreateSphereVertices(node->larssonVolume, vertices, indices);

            GLuint sphereVAO, sphereVBO, sphereEBO;
            glGenVertexArrays(1, &sphereVAO);
            glGenBuffers(1, &sphereVBO);
            glGenBuffers(1, &sphereEBO);

            glBindVertexArray(sphereVAO);

            glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glUniform3f(glGetUniformLocation(bvShaderProgram, "boundingVolumeColor"), color.r, color.g, color.b); // Set sphere color
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(0.0001f, 0.0001f, 0.0001f));
            int modelLoc = glGetUniformLocation(bvShaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(sphereVAO);
            glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glDeleteVertexArrays(1, &sphereVAO);
            glDeleteBuffers(1, &sphereVBO);
            glDeleteBuffers(1, &sphereEBO);
        }
        else if (currentBVType == BVT_PCA_SPHERE) {
            std::vector<glm::vec3> vertices;
            std::vector<GLuint> indices;

            CreateSphereVertices(node->pcaVolume, vertices, indices);

            GLuint sphereVAO, sphereVBO, sphereEBO;
            glGenVertexArrays(1, &sphereVAO);
            glGenBuffers(1, &sphereVBO);
            glGenBuffers(1, &sphereEBO);

            glBindVertexArray(sphereVAO);

            glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            glUniform3f(glGetUniformLocation(bvShaderProgram, "boundingVolumeColor"), color.r, color.g, color.b); // Set sphere color
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(0.0001f, 0.0001f, 0.0001f));
            int modelLoc = glGetUniformLocation(bvShaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(sphereVAO);
            glDrawElements(GL_LINES, indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            glDeleteVertexArrays(1, &sphereVAO);
            glDeleteBuffers(1, &sphereVBO);
            glDeleteBuffers(1, &sphereEBO);
        }
    }

    // Continue with children if not at target level or drawing all levels
    if (drawAllLevels || currentLevel != targetLevel) {
        DrawBoundingVolumes(node->lChild, bvShaderProgram, drawAllLevels, targetLevel, currentLevel + 1);
        DrawBoundingVolumes(node->rChild, bvShaderProgram, drawAllLevels, targetLevel, currentLevel + 1);
    }
}

int main() {
    const unsigned int SCR_WIDTH = 1920;
    const unsigned int SCR_HEIGHT = 1080;

    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create window
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL OBJ Renderer", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // Set viewport
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

    // Load all models from the specified directory with a specific scale
    loadModelsFromDirectory("../Assets/power4/part_a", 0.0001f);
    loadModelsFromDirectory("../Assets/power4/part_b", 0.0001f);
    loadModelsFromDirectory("../Assets/power5/part_a", 0.0001f);
    loadModelsFromDirectory("../Assets/power5/part_b", 0.0001f);
    loadModelsFromDirectory("../Assets/power5/part_c", 0.0001f);
    loadModelsFromDirectory("../Assets/power6/part_a", 0.0001f);
    loadModelsFromDirectory("../Assets/power6/part_b", 0.0001f);

    // Root nodes of the BVH
    TreeNode* topRoot = new TreeNode();
    TreeNode* botRoot = nullptr;

    // Build the Top-Down BVH
    TopDownTree(topRoot, objects, 0, maxHeightValue);

    // Build the Bottom-Up BVH
    std::vector<TreeNode*> leafNodes = InitializeLeafNodes(objects);
    botRoot = BottomUpTree(leafNodes);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    shader();
    bvShader();

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
     
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        // Process input
        processInput(window);

        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int maxD = (currentMethod == CM_TOP_DOWN) ? TreeDepth(topRoot) : TreeDepth(botRoot);

        // ImGui interface
        ImGui::Begin("Bounding Volume");

        // Radio buttons for root selection
        ImGui::Text("Choose Root:");
        if (ImGui::RadioButton("Top-Down", currentMethod == CM_TOP_DOWN)) {
            currentMethod = CM_TOP_DOWN;
        }
        if (ImGui::RadioButton("Bottom-Up", currentMethod == CM_BOTTOM_UP)) {
            currentMethod = CM_BOTTOM_UP;
        }

        if (currentMethod == CM_TOP_DOWN) {
            if (ImGui::Checkbox("Restrict Height to 7", &maxHeight)) {
                rebuildTree = true; // Set rebuild flag if checkbox state changes
                maxHeightValue = maxHeight ? 7 : INT_MAX;
            }
            ImGui::Text("Split Method:");
            const char* splitItems[] = { "Median of Centers", "Median of Extents", "K Even Splits" };
            static int splitItem = 0; // Default to "Median of Centers"
            if (ImGui::Combo("##SplitMethod", &splitItem, splitItems, IM_ARRAYSIZE(splitItems))) {
                currentSplitMethod = static_cast<SplitMethod>(splitItem);
                rebuildTree = true; // Set rebuild flag
            }
        }

        ImGui::Text("Bounding Volume Type:");
        const char* bvItems[] = { "None", "AABB", "Ritter Sphere", "Larsson Sphere", "PCA Sphere" };
        static int bvItem = 0; // default to "None"
        if (ImGui::Combo("##BVType", &bvItem, bvItems, IM_ARRAYSIZE(bvItems))) {
            currentBVType = static_cast<BoundingVolumeType>(bvItem);
        }

        // Checkbox for displaying all levels
        ImGui::Checkbox("Display All Levels", &displayAllLevels);

        // Slider for selecting tree level
        ImGui::SliderInt("Tree Level", &currentLevel, 0, maxD - 1);

        ImGui::End();

        glClearColor(0.4f, 0.4f, 0.4f, 1.f);
        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Rebuild the tree if needed
        if (rebuildTree) {
            delete topRoot;
            topRoot = new TreeNode();
            TopDownTree(topRoot, objects, 0, maxHeightValue);
            rebuildTree = false; // Reset the rebuild flag
        }

        // Use shader program
        glUseProgram(shaderProgram);

        // Set view and projection matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        int projLoc = glGetUniformLocation(shaderProgram, "projection");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Set light and view positions
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), -50.f, 50.0f, 20.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.537, 0.098, 0.898);

        // Draw each loaded model with its corresponding scale
        for (size_t i = 0; i < VAOs.size(); ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(scales[i], scales[i], scales[i]));
            int modelLoc = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(VAOs[i]);
            glDrawElements(GL_TRIANGLES, meshes[i].Indices.size(), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }

        // Use shader program
        glUseProgram(bvShaderProgram);

        // Set view and projection matrices
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        projection = glm::perspective(glm::radians(fov), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

        viewLoc = glGetUniformLocation(bvShaderProgram, "view");
        projLoc = glGetUniformLocation(bvShaderProgram, "projection");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw bounding volumes based on the selected type and level
        TreeNode* rootToDraw = (currentMethod == CM_TOP_DOWN) ? topRoot : botRoot;
        DrawBoundingVolumes(rootToDraw, bvShaderProgram, displayAllLevels, currentLevel);

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup VAOs, VBOs, and EBOs
    for (size_t i = 0; i < VAOs.size(); ++i) {
        glDeleteVertexArrays(1, &VAOs[i]);
        glDeleteBuffers(1, &VBOs[i]);
        glDeleteBuffers(1, &EBOs[i]);
    }

    glDeleteProgram(shaderProgram);
    glDeleteProgram(bvShaderProgram);

    // Cleanup ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Terminate GLFW
    glfwTerminate();
    return 0;
}