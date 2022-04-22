#include "Boss.h"

#include "FrameResource.h"
#include "Constants.h"
using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

Boss::Boss()
{
	hp = gc::BOSS_MAX_HEALTH;
	posX = 0;
	posZ = 0;
	spawn = false;
}

Boss::~Boss()
{

}

void Boss::Setup(Point* geo, Camera* player, BoundingBox* box)
{
	assert(box);
	pointObject = geo;
	playerObject = player;
	hitbox = box;
}

void Boss::Movement(float dt) 
{
	assert(hitbox);
	// updates point
	pointObject->Pos = { posX, posY + tt, posZ };

	XMFLOAT3 playerPos = playerObject->GetPosition3f();
	XMFLOAT3 bossPos = pointObject->Pos;
	float playerToBoss = ((pow((bossPos.x - playerPos.x), 2)) + (pow((bossPos.y - playerPos.y), 2)) + (pow((bossPos.z - playerPos.z), 2))) * 0.5f;

	if (playerToBoss <= 10)
	{
		playerObject->SetPosition(XMFLOAT3(playerObject->GetPosition3f().x * 4, playerObject->GetPosition3f().y, playerObject->GetPosition3f().z * 4));
	}
}

void Boss::Update()
{
	Movement(0);
	spawn = hp == 100;
}

bool Boss::SpawnReady()
{
	return spawn;
}

int Boss::GetSpawnRate()
{
	return spawnRate;
}

void Boss::Pattern_1()
{
	spawnRate = 8;
	spawn = true;
}

void Boss::Pattern_2()
{
	spawnRate = 5;
	spawn = true;
}
void Boss::Pattern_3()
{
	spawnRate = 2;
	spawn = true;
}

