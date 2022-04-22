#pragma once

class TimerInterval
{
public: 
	TimerInterval();
	TimerInterval(float);
	~TimerInterval();
	bool UpdateTimer(float gt);
	void Start();
	void SetInterval(float);
private:
	bool hasStarted = false;
	float timer = 0.0f;
	float interval = 0.0f;
};