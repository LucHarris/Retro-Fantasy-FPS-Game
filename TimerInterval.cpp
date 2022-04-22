#include "TimerInterval.h"

TimerInterval::TimerInterval()
{

}

TimerInterval::TimerInterval(float t)
{
	interval = t;
}

TimerInterval::~TimerInterval()
{

}

void TimerInterval::Start()
{
	hasStarted = true;
}

void TimerInterval::SetInterval(float t)
{
	interval = t;
}

bool TimerInterval::UpdateTimer(float gt)
{
	if (hasStarted == true)
	{
		timer += gt;
		if (timer >= interval)
		{

			timer = 0;
			hasStarted = false;
			return true;
		}
	}

	return false;
}
