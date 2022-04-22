#include "Animation.h"
#include "Constants.h"
Animation::Animation(const NoramlisedAnimData* pAnimData, DirectX::XMFLOAT4* pDest)
{
	assert(pAnimData);
	SetAnimation(pAnimData, pDest);
}
void Animation::SetAnimation(const NoramlisedAnimData* pAnimData, DirectX::XMFLOAT4* pDest)
{
	assert(pAnimData);
	pData = pAnimData;
	
	if (pDest)
	{
		pDestination = pDest;
	}

	if (pDestination)
	{
		*pDestination = pData->initFloatRect;
	}
	else
	{
		assert(false);
	}

	isAnimating = pData->animating;
	currentFrame = rand() % pData->numFrames;

}
void Animation::Update(float dt)
{

	if (pData && pDestination)
	{
		if (isAnimating)
		{
			// decrement timer for next frame
			if (currentFrame != pData->numFrames || pData->loop)
			{
				cdTimer -= dt;
			}

			// set next frame
			if (cdTimer <= 0.0f)
			{
				
				currentFrame = (currentFrame + 1) % pData->numFrames;
				cdTimer = gc::ANIM_FRAME_TIME;
				//float x = pData->initFloatRect.x + (currentFrame * pData->frameOffset);
				// update destination rect

				DirectX::XMStoreFloat4(pDestination, GetFrame());
				//*pDestination = GetFrame();
			}
		}
	}
	else
	{
		assert(false);
	}
}

DirectX::SimpleMath::Vector4 Animation::GetFrame()
{
	DirectX::SimpleMath::Vector4 temp = pData->initFloatRect;
	float offset = (float)currentFrame * pData->frameOffset;
	temp.x += offset;
	return temp;
}
