#include "Terrain.h"

DirectX::SimpleMath::Vector3 FloorV3(const DirectX::SimpleMath::Vector3& v)
{
  return  { floorf(v.x), floorf(v.y), floorf(v.z) };
}

DirectX::SimpleMath::Vector2 FloorV2(const DirectX::SimpleMath::Vector2& v)
{
  return  { floorf(v.x), floorf(v.y)};
}


DirectX::SimpleMath::Vector3 FracV3(const DirectX::SimpleMath::Vector3& v)
{
  return v - FloorV3(v);
}

DirectX::SimpleMath::Vector3 Hash(DirectX::SimpleMath::Vector3 p, float seed)
{

  float x = sinf(p.Dot(DirectX::SimpleMath::Vector3(127.1f, 0.0f, 311.7f)));
  float y = sinf(p.Dot(DirectX::SimpleMath::Vector3(269.5f, 0.0f, 183.3f)));
  float z = sinf(p.Dot(DirectX::SimpleMath::Vector3(419.2f, 0.0f, 371.9f)));

  DirectX::SimpleMath::Vector3 a = DirectX::SimpleMath::Vector3(x, y, z) * seed;
  DirectX::SimpleMath::Vector3 b = FloorV3(a);

  // between 0-1
  return a - b;
}

DirectX::SimpleMath::Vector2 Hash2(DirectX::SimpleMath::Vector2 v, float seed)
{

  DirectX::SimpleMath::Vector2 d = {
    sinf(v.Dot(DirectX::SimpleMath::Vector2(127.1f, 311.7f))),
    sinf(v.Dot(DirectX::SimpleMath::Vector2(269.5f, 183.3f)))};

  d *= 43758.5453;
  // floor
  DirectX::SimpleMath::Vector2 f = { floorf(d.x),floorf(d.y) };

  // fract
  return d - f;
}


float Clamp(float a, float low, float high)
{
  float r = a;
  if (a < low)
  {
    r = low;
  }
  else
  {
    if (high < a)
    {
      r = high;
    }
  }
  return r;
}

float Smoothstep(float a, float low, float high)
{
  return Clamp(a * a * (3.0f - 2.0f * a), 0.0f, 1.0f);
}






// params:
//  noiseScale - size of noise. smaller number for smoother terrain
//  curveStrength - stretches terrain and smoothstep clamp between 0-1 
float CalcTerrainHeight2(const DirectX::SimpleMath::Vector3& pos, const TerrainParams& tp)// float noiseScale, float curveStrength, float seed)
{


  const DirectX::SimpleMath::Vector2 scaledPos = DirectX::SimpleMath::Vector2{ pos.x,pos.z } * tp.noiseScale;
  // grid position
  const DirectX::SimpleMath::Vector2 ipos = FloorV2(scaledPos);
  // fractional position within grid
  const DirectX::SimpleMath::Vector2 fpos = scaledPos - ipos;

  float curve = max(tp.curveStrength, 1.0f);
  float curveOffset = tp.curveStrength * 0.5f - 0.5f;


  const float smooth = 1.0f;
  const float k = 1.0f + 63.0f * pow(1.0 - smooth, 4.0);

  float minDist = 1.0;
  DirectX::SimpleMath::Vector2 minPoint;
  float res = minDist;

  for (int x = -1; x <= 1; ++x)
  {
    for (int y = -1; y <= 1; ++y)
    {
      const DirectX::SimpleMath::Vector2 ineighbour = DirectX::SimpleMath::Vector2(float(x), float(y));
      const DirectX::SimpleMath::Vector2 point = Hash2(ipos + ineighbour, tp.seed);
      const DirectX::SimpleMath::Vector2 diff = point + ineighbour - fpos;

      const float dist = diff.Length();

      if (dist < minDist)
      {
        minDist = dist;
        res = minDist * curve - curveOffset;
        minPoint = point;
      }
    }
  }

  return  (1.0f - Smoothstep(res, 0.0f, 1.0f)) * tp.heightMulti;
}

DirectX::SimpleMath::Vector3 ApplyTerrainHeight(const DirectX::SimpleMath::Vector3& pos, const TerrainParams& tp)
{
  return { pos.x,CalcTerrainHeight2(pos,tp),pos.z };
}

DirectX::SimpleMath::Vector3 CalcTerrianNormal(const DirectX::SimpleMath::Vector3& pos, const DirectX::SimpleMath::Vector3& res, float normalScale, float noiseScale, float voronoi, float smooth, float seed)
{
  const DirectX::SimpleMath::Vector3& step = DirectX::SimpleMath::Vector3(1.0f / res.x, 1.0f / res.y, 1.0f / res.z);
  // todo normal amend
  float height = 0.0f;// CalcTerrainHeight2(pos, noiseScale, voronoi, smooth, seed);

  DirectX::SimpleMath::Vector2 dxy =
    DirectX::SimpleMath::Vector2(height) - DirectX::SimpleMath::Vector3(
      1.0f,//CalcTerrainHeight(pos + DirectX::SimpleMath::Vector2(step.x, 0.0f), noiseScale, voronoi, smooth, seed),
      1.0f,
      1.0f//CalcTerrainHeight(pos + DirectX::SimpleMath::Vector2(0.0f, step.z), noiseScale, voronoi, smooth, seed)
    );

  DirectX::SimpleMath::Vector3 normal = {
    dxy.x * normalScale / step.x,
    dxy.y * normalScale / step.z,
    1.0f
  };
  normal.Normalize();
  return normal;
}


void CalcTerrianNormal2(GeometryGenerator::MeshData& grid)
{
  // vertex index 
  size_t i0, i1, i2;

  for (size_t i = 0; i < grid.Indices32.size(); i += 3)
  {
    assert(i + 2 < grid.Indices32.size());
    // 
    i0 = grid.Indices32[i];
    i1 = grid.Indices32[i+1];
    i2 = grid.Indices32[i+2];

    const DirectX::SimpleMath::Vector3 v0 = grid.Vertices.at(i0).Position;
    const DirectX::SimpleMath::Vector3 v1 = grid.Vertices.at(i1).Position;
    const DirectX::SimpleMath::Vector3 v2 = grid.Vertices.at(i2).Position;
    // edge
    const DirectX::SimpleMath::Vector3 e1 = v0 - v1;
    const DirectX::SimpleMath::Vector3 e2 = v0 - v2;
    // normal of face
    const DirectX::SimpleMath::Vector3 c = e1.Cross(e2);

    // accumulate normals
    grid.Vertices.at(i0).Normal = c + grid.Vertices.at(i0).Normal;
    grid.Vertices.at(i1).Normal = c + grid.Vertices.at(i1).Normal;
    grid.Vertices.at(i2).Normal = c + grid.Vertices.at(i2).Normal;

  }

  // normalise
  for (size_t i = 0; i < grid.Vertices.size(); ++i)
  {
    DirectX::SimpleMath::Vector3 n = grid.Vertices.at(i).Normal;
    n.Normalize();
    grid.Vertices.at(i).Normal = n;
  }
}