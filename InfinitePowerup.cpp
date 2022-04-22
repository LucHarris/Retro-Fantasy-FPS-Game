#include "InfinitePowerup.h"

InfinitePowerup::InfinitePowerup()
{

}

InfinitePowerup::InfinitePowerup(float t, float s)
{
	maxTimeActive = t;
	timer = TimerInterval(s);
	timer.Start();
}

InfinitePowerup::~InfinitePowerup()
{

}

float InfinitePowerup::Consume()
{
	if (hasBeenConsumed == false && hasSpawnedIn == true)
	{
		hasBeenConsumed = true;
		timer.Start();
		return maxTimeActive;
	}
	else return 0;
}

void InfinitePowerup::Update(float gt)
{
	if (hasSpawnedIn == false)
	{
		if (timer.UpdateTimer(gt) == true)
		{
			hasSpawnedIn = true;
			timer.SetInterval(150.0f);
		}
	}

	if (hasBeenConsumed == true)
	{
		if (timer.UpdateTimer(gt) == true)
		{
			hasBeenConsumed = false;
		}
	}
}


