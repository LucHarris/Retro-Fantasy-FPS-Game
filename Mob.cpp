#include "Mob.h"

#include "Mob.h"

#include "FrameResource.h"
using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

Mob::Mob()
{
	hp = 5;
}

Mob::~Mob()
{

}

void Mob::Setup(Point* geo, Camera* player, BoundingBox* box)
{
	assert(box);
	pointObject = geo;
	playerObject = player;
	hitbox = box;

	hitbox->Center = XMFLOAT3(posX, posY, posZ);
	pointObject->Pos = { posX, posY, posZ };
}

void Mob::Movement(float dt) 
{
	if (isActive)
	{
		assert(hitbox);
		assert(playerObject);
		FollowTarget(playerObject->GetPosition3f(), dt);
		pointObject->Pos = { posX, posY, posZ };
		hitbox->Center = XMFLOAT3(posX, posY, posZ);
	}
}

void Mob::Update()
{
	
}

