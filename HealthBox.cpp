#include "HealthBox.h"

HealthBox::HealthBox()
{

}

HealthBox::HealthBox(float t, float s)
{
	healthPercentageToRestore = t;
	timer = TimerInterval(s);
	timer.Start();
}

HealthBox::~HealthBox()
{

}

float HealthBox::Consume()
{
	if (hasBeenConsumed == false && hasSpawnedIn == true)
	{
		hasBeenConsumed = true;
		timer.Start();
		return healthPercentageToRestore;
	}
	else return 0;
}

void HealthBox::Update(float gt)
{
	if (hasSpawnedIn == false)
	{
		if (timer.UpdateTimer(gt) == true)
		{
			hasSpawnedIn = true;
			timer.SetInterval(30.0f);
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


