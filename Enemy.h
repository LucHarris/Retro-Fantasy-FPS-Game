#pragma once
#include "RenderItemStruct.h"
#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "Common/Camera.h"
#include <random>
#include <time.h>
#include <cmath>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace std;



class Enemy
{
	public:
		Enemy();
		~Enemy();
		bool DealDamage(int damage);
		void FollowTarget(XMFLOAT3 targetPos, float dt);
		/*void Setup(int stage, int health, RenderItem* geo);*/
		virtual void Movement(float dt) = 0;
		virtual void Update() = 0;

	//private:
		int hp;
		int stage;
		//RenderItem* geoObject;
		Camera* playerObject;
		BoundingBox* hitbox;
		float tX = 0, tY, tZ, tt = 0;

		float posX = 0.0f;
		float posY = 5.0f;
		float posZ = 0.0f;
		bool isActive = false;
};
