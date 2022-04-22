#include "QuadPowerup.h"

QuadPowerup::QuadPowerup()
{

}

QuadPowerup::QuadPowerup(float t, float m, float s)
{
	maxTimeActive = t;
	damageMultiplier = m;
	timer = TimerInterval(s);
	timer.Start();
}

QuadPowerup::~QuadPowerup()
{

}

float QuadPowerup::Consume() //Change this to a vec2 or an array of floats as we need to send two int values through
{
	if (hasBeenConsumed == false && hasSpawnedIn == true)
	{
		hasBeenConsumed = true;
		timer.Start();
		return maxTimeActive;
		//return speedMultiplier;
	}
	else return 0;
}

void QuadPowerup::Update(float gt)
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


