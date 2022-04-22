#pragma once
#include "Enemy.h"

class Point;

class Boss : public Enemy
{
public: 
	Boss();
	~Boss();
	void Setup(Point* geo, Camera* player, BoundingBox* box);
	void Enemy::Movement(float dt);
	void Enemy::Update();
	void Pattern_1();
	void Pattern_2();
	void Pattern_3();
	int GetSpawnRate();
	bool SpawnReady();
private:
	bool spawn;
	int spawnRate = 8;
	Point* pointObject;
};