#include "Time.h"

Time& Time::GetInstance()
{
	static Time instance;
	return instance;
}

void Time::Update()
{
	auto& t = GetInstance();
	t.m_CurrentTime = std::chrono::high_resolution_clock::now();
	t.m_DeltaTime = std::chrono::duration<float>(t.m_CurrentTime - t.m_LastTime).count();
	t.m_LastTime = t.m_CurrentTime;
	t.m_TotalTime += t.m_DeltaTime;
}

float Time::GetDeltaTime()
{
	return GetInstance().m_DeltaTime;
}

float Time::GetTotalTime()
{
	return GetInstance().m_TotalTime;
}

float Time::GetFixedDeltaTime()
{
	return m_FixedTimeStep;
}
