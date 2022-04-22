#pragma once
#include "SimpleMath.h"
#include "Constants.h"


class Animation
{
	// must be assigned before Update() call
	DirectX::XMFLOAT4* pDestination;
	int currentFrame = 0;
	float cdTimer = gc::ANIM_FRAME_TIME;
	const NoramlisedAnimData* pData = nullptr;
	float isAnimating = true;
public:
	Animation() = default;
	// pass in pAnimData from constant.h
	Animation(const NoramlisedAnimData* pAnimData, DirectX::XMFLOAT4* pDest = nullptr);

	// pass in pAnimData from constant.h
	// pDest must be valid on first call
	void SetAnimation(const NoramlisedAnimData* pAnimData, DirectX::XMFLOAT4* pDest = nullptr);
	void Update(float dt);
	
	DirectX::SimpleMath::Vector4 GetFrame();
};

