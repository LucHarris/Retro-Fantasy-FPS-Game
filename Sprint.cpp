#include "Sprint.h"


Sprint::Sprint()
{

}

Sprint::~Sprint()
{

}

void Sprint::Update(float stamina)
{
	if (isSprinting == true)
	{
		sprintScale = 2.0f;
		stamina--;
	}
	else if (isSprinting == false)
	{
		sprintScale = 1.0f;
		stamina++;
	}
}