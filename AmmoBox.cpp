#include "AmmoBox.h"

AmmoBox::AmmoBox()
{

}

AmmoBox::AmmoBox(float t, float s)
{
	ammoPercentageToRestore = t;
	timer = TimerInterval(s);
	timer.Start();
}

AmmoBox::~AmmoBox()
{

}

float AmmoBox::Consume()
{
	if (hasBeenConsumed == false && hasSpawnedIn == true)
	{
		hasBeenConsumed = true;
		timer.Start();
		return ammoPercentageToRestore;
	}
	else return 0;
}

void AmmoBox::Update(float gt)
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


