#include "Enemy.h"

Enemy::Enemy()
{
	stage = 0;
	std::mt19937 random_number_engine(time(0));
	std::uniform_int_distribution<int> distribution(-5, 5);

	posX = distribution(random_number_engine);
	posZ = distribution(random_number_engine);
}

Enemy::~Enemy()
{

}

bool Enemy::DealDamage(int damage)
{
	hp -= damage;
	isActive = hp > 0;
	return hp <= 0;
}

void Enemy::FollowTarget(XMFLOAT3 targetPos, float dt)
{
	if (targetPos.x > posX) posX += (dt * 2);
	else if (targetPos.x < posX) posX += -(dt * 2);
	else posX += 0;

	if (targetPos.y > posY) posY += (dt * 2);
	else if (targetPos.y < posY) posY += -(dt * 2);
	else posY += 0;

	if (targetPos.z > posZ) posZ += (dt * 2);
	else if (targetPos.z < posZ) posZ += -(dt * 2);
	else posZ += 0;

}