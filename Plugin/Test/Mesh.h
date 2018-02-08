#pragma once

#include <vector>
#define usdiOverrideFloat4
#include "usdi/Foundation/aiMath.h"
#include "usdi/usdi.h"

#define DegToRad (3.1415926535897932384626433832795f / 180.0f)

void GenerateOffsets(std::vector<int>& dst, const std::vector<int>& counts);

void GenerateWaveMesh(
    std::vector<int>& counts,
    std::vector<int>& indices,
    std::vector<float3> &points,
    std::vector<float2> &uv,
    float size, float height,
    const int resolution,
    float angle);

void GenerateCylinderMesh(
    std::vector<int>& counts,
    std::vector<int>& indices,
    std::vector<float3> &points,
    std::vector<float2> &uv,
    float radius, float height,
    int cseg, int hseg);

void GenerateIcoSphereMesh(
    std::vector<int>& counts,
    std::vector<int>& indices,
    std::vector<float3> &points,
    std::vector<float2> &uv,
    float radius,
    int iteration = 3);
