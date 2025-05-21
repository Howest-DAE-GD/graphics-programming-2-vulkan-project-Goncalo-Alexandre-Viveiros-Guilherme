#define GLM_ENABLE_EXPERIMENTAL
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <GLFW/glfw3.h>

namespace GG
{
	class Camera
	{
	public:
		Camera() = default;
		void Initialize(float _fovAngle, glm::vec3 _origin, float AspectRatio, GLFWwindow* window);
		void CalculateViewMatrix();
		glm::mat4 GetViewMatrix();
		void CalculateProjectionMatrix();
		const float* GetOrigin() const;
		void Update();

		glm::mat4 GetProjectionMatrix() const { return m_ProjectionMatrix; }

	private:
		glm::vec3 m_Origin{};
		float m_AspectRatio{};
		float m_PreviousAspectRatio{m_AspectRatio};
		float m_FOVAngle{ 45.f };
		float m_PreviousFOVAngle{ m_FOVAngle };
		float m_FOV{};
		float m_RotSpeed{ 100 };
		float m_MovSpeed{ 10 };
		float m_NearPlane{ .1f };
		float m_FarPlane{ 500.f };

		glm::vec3 m_Forward{ 0.f, 0.f, 1.f };
		glm::vec3 m_Up{ 0.f, 1.f, 0.f };
		glm::vec3 m_Right{ 1.f, 0.f, 0.f };

		float m_TotalPitch{};
		float m_TotalYaw{};

		glm::mat4 m_InvViewMatrix{};
		glm::mat4 m_ViewMatrix{};
		glm::mat4 m_ProjectionMatrix{};

		GLFWwindow* m_Window;
		double m_PrevMouseX;
		double m_PrevMouseY;
	};
}
