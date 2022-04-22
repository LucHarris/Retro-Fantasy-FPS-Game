#include "ParticleCtrl.h"
#include "FrameResource.h"
void ParticleData::Update(float dt)
{
	if (size > 0.0f)
	{
		lifetime += dt;
		vel.y += -9.81 * dt * lifetime;
		size -= sizeMulti * dt;
		pos += vel * dt;
	}
	else
	{
		
	}

}

void ParticleSys::Init( size_t vertexCount, size_t burstCount)
{
	numParticlesPerBurst = burstCount;
	vertCount = vertexCount;

	data.resize(vertexCount);
}

void ParticleSys::Update(std::vector<Point>* const geoPoints, float dt)
{
	// todo update ritem
	
	assert(geoPoints->size() == vertCount);

	for (size_t i = 0; i < data.size(); i++)
	{
		data.at(i).Update(dt);
		geoPoints->at(i).Pos = data.at(i).pos;
		geoPoints->at(i).Size.x = data.at(i).size;
		geoPoints->at(i).Size.y = data.at(i).size;
		geoPoints->at(i).Billboard = BillboardType::AXIS_ORIENTATION;
		if (geoPoints->at(i).Size.x <= 0.0f || geoPoints->at(i).Size.y <= 0.0f)
		{
			geoPoints->at(i).Billboard = BillboardType::NONE;
		}
	}

}

void ParticleSys::Explode(std::vector<Point>* const geoPoints, const DirectX::SimpleMath::Vector3& pos, float sizeMin, float sizeMax , float speedMin ,float speedMax )
{

	size_t start = burstOffset;
	size_t end = min(burstOffset + numParticlesPerBurst, vertCount);

	for (size_t i = start; i < end; i++)
	{
		data.at(i).pos = pos;
		data.at(i).size = RandFl(sizeMin,sizeMax);

		DirectX::SimpleMath::Vector3 vel = {
			RandFl(-1.0f,1.0f),
			RandFl(-1.0f,1.0f),
			RandFl(-1.0f,1.0f)
		};

		vel.Normalize();

		float speed = RandFl(speedMin, speedMax);
		vel *= speed;
		data.at(i).vel = vel;
		data.at(i).sizeMulti = RandFl(1.0f,2.0f);
		data.at(i).lifetime = 0.0f;

		geoPoints->at(i).Pos = data.at(i).pos;
		geoPoints->at(i).Size.x = data.at(i).size;
		geoPoints->at(i).Size.y = data.at(i).size;
	}

	

	// loop back to start of vertex cache
	burstOffset += numParticlesPerBurst;
	if (burstOffset >= vertCount)
	{
		burstOffset = 0;
	}
}

float RandFl(float low, float high)
{
	if (low > high)
	{
		std::swap(low, high);
	}

	return low + ((float)(rand() / (float)(RAND_MAX / (high - low))));
}
