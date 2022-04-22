#include "TimerInterval.h"

class SpeedPowerup
{
public:
	SpeedPowerup();
	SpeedPowerup(float, float, float);
	~SpeedPowerup();
	float Consume();
	void Update(float);
	bool hasBeenConsumed = false;
	bool hasSpawnedIn = false;
private:
	float maxTimeActive = 30.f;
	float speedMultiplier = 3.f;
	TimerInterval timer;
};
