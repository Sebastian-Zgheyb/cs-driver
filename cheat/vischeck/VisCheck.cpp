#include "VisCheck.h"

bool VisCheck::IsPointVisible(const Vector3& point1, const Vector3& point2)
{
    Vector3 rayDir = { point2.x - point1.x, point2.y - point1.y, point2.z - point1.z };
    float distance = std::sqrt(rayDir.dot(rayDir));
    rayDir = { rayDir.x / distance, rayDir.y / distance, rayDir.z / distance };
    float t;

    bool isVisible = true;

    for (size_t i = 0; i < triangles.size(); ++i) {
        if (!isVisible) break;

        const auto& triangleList = triangles[i];
        const auto& aabbList = triangleAABBs[i];

        for (size_t j = 0; j < triangleList.size(); ++j) {
            if (!isVisible) break;

            const auto& triangleAABB = aabbList[j];
            const auto& triangle = triangleList[j];

            if (!triangleAABB.RayIntersects(point1, rayDir)) {
                continue;
            }

            if (RayIntersectsTriangle(point1, rayDir, triangle, t)) {
                if (t < distance) {
                    isVisible = false;
                    break;
                }
            }
        }
    }

    return isVisible;
}

bool VisCheck::RayIntersectsTriangle(const Vector3& rayOrigin, const Vector3& rayDir, const TriangleCombined& triangle, float& t)
{
    const float EPSILON = 0.0000001f;

    Vector3 edge1 = triangle.v1 - triangle.v0;
    Vector3 edge2 = triangle.v2 - triangle.v0;
    Vector3 h = rayDir.cross(edge2);
    float a = edge1.dot(h);

    if (a > -EPSILON && a < EPSILON)
        return false;

    float f = 1.0f / a;
    Vector3 s = rayOrigin - triangle.v0;
    float u = f * s.dot(h);

    if (u < 0.0f || u > 1.0f)
        return false;

    Vector3 q = s.cross(edge1);
    float v = f * rayDir.dot(q);

    if (v < 0.0f || u + v > 1.0f)
        return false;

    t = f * edge2.dot(q);

    return (t > EPSILON);
}