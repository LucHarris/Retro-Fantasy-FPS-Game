#pragma once

#include "SimpleMath.h"
//#include "FrameResource.h"
#include <deque>
#include <vector>
#include "Animation.h"


struct Point;

struct ParticleData
{
	DirectX::SimpleMath::Vector3 pos = DirectX::SimpleMath::Vector3::Zero;
	DirectX::SimpleMath::Vector3 vel = DirectX::SimpleMath::Vector3::Down;
	float size = 0.0f;
	float sizeMulti = 1.0f;
	float lifetime = 0.0f;
	float speed = 1.0f;
	
	void Update(float dt);
};
class Application;
class ParticleSys
{
	size_t particleRitemIndex;
	size_t numParticlesPerBurst;
	size_t burstOffset = 0;
	size_t vertCount;
	std::deque<ParticleData> data;
public:
	void Init( size_t cacheSize, size_t burstCount);
	void Update(std::vector<Point>* const, float dt);

	void Explode(std::vector<Point>* const geoPoints, const DirectX::SimpleMath::Vector3& pos, float sizeMin = 0.5f, float sizeMax = 2.0f, float speedMin = 0.5f, float speedMax = 10.0f);
};

float RandFl(float low = 0.0f, float high = 1.0f);

