#pragma once
#include "Enemy.h"
class Point;

class Mob : public Enemy
{
public:
	Mob();
	~Mob();
	void Setup(Point* geo, Camera* player, BoundingBox* box);
	void Enemy::Movement(float dt);
	void Enemy::Update();
private:
	Point* pointObject = nullptr;
};