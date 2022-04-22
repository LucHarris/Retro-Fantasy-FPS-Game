#include "TimerInterval.h"

class InfinitePowerup
{
public:
	InfinitePowerup();
	InfinitePowerup(float, float);
	~InfinitePowerup();
	float Consume();
	void Update(float);
	bool hasBeenConsumed = false;
	bool hasSpawnedIn = false;
private:
	float maxTimeActive = 30.f;
	TimerInterval timer;
};
