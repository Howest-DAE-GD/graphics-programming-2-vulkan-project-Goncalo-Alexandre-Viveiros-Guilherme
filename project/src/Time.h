#pragma once
#include <chrono>

class Time
{
public:
	static void Update();
	static float GetDeltaTime();
	static float GetTotalTime();
	static float GetFixedDeltaTime();

private:
	Time() = default;
	static Time& GetInstance();

	float m_DeltaTime{};
	float m_TotalTime{};
	std::chrono::high_resolution_clock::time_point m_CurrentTime{};
	std::chrono::high_resolution_clock::time_point m_LastTime{ std::chrono::high_resolution_clock::now() };
	float m_SleepTime{};
	static constexpr float m_FixedTimeStep{ 1 / 120.f };
	static constexpr double ms_per_frame{ 1000.f / 120.f };
};