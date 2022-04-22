#include "TimerInterval.h"

class HealthBox
{
public:
	HealthBox();
	HealthBox(float, float);
	~HealthBox();
	float Consume();
	void Update(float);
	bool hasBeenConsumed = false;
	bool hasSpawnedIn = false;
private:
	float healthPercentageToRestore = 50.0f;
	TimerInterval timer;
};