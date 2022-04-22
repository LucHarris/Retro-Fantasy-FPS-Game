#include "ShieldPowerup.h"

ShieldPowerup::ShieldPowerup()
{

}

ShieldPowerup::ShieldPowerup(float t, float s)
{
	maxTimeActive = t;
	timer = TimerInterval(s);
	timer.Start();
}

ShieldPowerup::~ShieldPowerup()
{

}

float ShieldPowerup::Consume()
{
	if (hasBeenConsumed == false && hasSpawnedIn == true)
	{
		hasBeenConsumed = true;
		timer.Start();
		return maxTimeActive;
	}
	else return 0;
}

void ShieldPowerup::Update(float gt)
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


