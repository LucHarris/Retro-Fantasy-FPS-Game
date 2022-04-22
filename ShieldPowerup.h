#include "TimerInterval.h"

class ShieldPowerup
{
public:
	ShieldPowerup();
	ShieldPowerup(float, float);
	~ShieldPowerup();
	float Consume();
	void Update(float);
	bool hasBeenConsumed = false;
	bool hasSpawnedIn = false;
private:
	float maxTimeActive = 30.f;
	TimerInterval timer;
};
