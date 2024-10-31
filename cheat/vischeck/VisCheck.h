#pragma once
#include <future>

#include "../Parser/Parser.h"

class VisCheck
{
public:
    Parser parser;

    VisCheck() {
        triangles = parser.GetCombinedList();
        PrecomputeAABBs();
    }

    bool IsPointVisible(const Vector3& point1, const Vector3& point2);

private:
    std::vector<std::vector<AABB>> triangleAABBs;
    std::vector<std::vector<TriangleCombined>> triangles;

    void PrecomputeAABBs() {
        for (const auto& triangleList : triangles) {
            std::vector<AABB> aabbList;
            for (const auto& triangle : triangleList) {
                aabbList.push_back(triangle.ComputeAABB());
            }
            triangleAABBs.push_back(aabbList);
        }
    }

    bool RayIntersectsTriangle(const Vector3& rayOrigin, const Vector3& rayDir, const TriangleCombined& triangle, float& t);
};

inline VisCheck vischeck;