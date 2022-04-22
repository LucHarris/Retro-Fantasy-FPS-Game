#pragma once


#include "SimpleMath.h"
#include <algorithm>
#include "Common/GeometryGenerator.h"

struct TerrainParams
{
  float noiseScale = 1.0f;
  float curveStrength = 1.0f;
  float seed = 43758.5453f;
  float heightMulti = 1.0f;
};


DirectX::SimpleMath::Vector3 FloorV3(const DirectX::SimpleMath::Vector3& v);

DirectX::SimpleMath::Vector2 FloorV2(const DirectX::SimpleMath::Vector2& v);

DirectX::SimpleMath::Vector3 FracV3(const DirectX::SimpleMath::Vector3& v);

// deterministic random function for noise using xz
DirectX::SimpleMath::Vector3 Hash(DirectX::SimpleMath::Vector3 p, float seed = 43758.5453f);
DirectX::SimpleMath::Vector2 Hash2(DirectX::SimpleMath::Vector2 v, float seed = 43758.5453f);
// std::clamp available since C++17
float Clamp(float a, float low, float high);

float Smoothstep(float a, float low, float high);

// noise function to grid y
float CalcTerrainHeight2(const DirectX::SimpleMath::Vector3& pos, const TerrainParams& tp);// float noiseScale = 1.0f, float curveStrength = 1.0f, float seed = 43758.5453f);

DirectX::SimpleMath::Vector3 ApplyTerrainHeight(const DirectX::SimpleMath::Vector3& pos, const TerrainParams& tp);


// normal for initialisig terrain
DirectX::SimpleMath::Vector3 CalcTerrianNormal(
  const DirectX::SimpleMath::Vector3& pos,
  const DirectX::SimpleMath::Vector3& res,
  float normalScale = 1.0f,
  float noiseScale = 1.0f,
  float voronoi = 1.0f,
  float smooth = 1.0f,
  float seed = 43758.5453f
);

void CalcTerrianNormal2(GeometryGenerator::MeshData& grid);