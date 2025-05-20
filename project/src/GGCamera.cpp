#include "GGCamera.h"
#include "Time.h"
#include <algorithm>
#include <cmath>


namespace GG {

    void Camera::Initialize(float fovAngle, glm::vec3 origin, float aspectRatio, GLFWwindow* window)
    {
        m_FOVAngle = fovAngle;
        m_FOV = tan(glm::radians(m_FOVAngle) / 2.0f);
        m_Origin = origin;
        m_AspectRatio = aspectRatio;
        m_Window = window;

        // Hide and lock cursor
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        glfwGetCursorPos(m_Window, &m_PrevMouseX, &m_PrevMouseY);
    }

    void Camera::CalculateViewMatrix()
    {
        m_Right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), m_Forward));
        m_Up = glm::normalize(glm::cross(m_Forward, m_Right));

        m_ViewMatrix = glm::lookAt(m_Origin, m_Origin + m_Forward, m_Up);
        m_InvViewMatrix = glm::inverse(m_ViewMatrix);
    }

    glm::mat4 Camera::GetViewMatrix()
    {
        return m_ViewMatrix;
    }

    void Camera::CalculateProjectionMatrix()
    {
        m_ProjectionMatrix = glm::perspectiveLH(glm::radians(m_FOVAngle), m_AspectRatio, m_NearPlane, m_FarPlane);
    }

    const float* Camera::GetOrigin() const
    {
        return glm::value_ptr(m_Origin);
    }

    void GG::Camera::Update()
    {
        if (m_FOVAngle != m_PreviousFOVAngle)
        {
            m_FOV = tan(glm::radians(m_FOVAngle) / 2.0f);
            m_PreviousFOVAngle = m_FOVAngle;
        }

        const float deltaTime = Time::GetDeltaTime();

        // Mouse input
        double mouseX, mouseY;
        glfwGetCursorPos(m_Window, &mouseX, &mouseY);

        int mouseXInt = static_cast<int>(mouseX);
        int mouseYInt = static_cast<int>(mouseY);
        int deltaX = mouseXInt - static_cast<int>(m_PrevMouseX);
        int deltaY = mouseYInt - static_cast<int>(m_PrevMouseY);

        m_PrevMouseX = mouseX;
        m_PrevMouseY = mouseY;

        int left = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_LEFT);
        int right = glfwGetMouseButton(m_Window, GLFW_MOUSE_BUTTON_RIGHT);

        float moveSpeed {m_MovSpeed};

        if (glfwGetKey(m_Window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            moveSpeed *= 2;

        glm::vec3 forwardVec = glm::normalize(m_Forward) * moveSpeed;
        glm::vec3 rightVec = m_Right * moveSpeed;
        glm::vec3 upVec = m_Up * moveSpeed;

        if (left && right)
        {
            if (deltaY > 0)
            {
                m_Origin -= upVec * 3.f * deltaTime;
            }

            if (deltaY < 0)
            {
                m_Origin += upVec * 3.f * deltaTime;
            }
        }
        else if (right)
        {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (deltaY > 0)
            {
                m_TotalPitch -= m_RotSpeed * deltaTime;
            }
            if (deltaY < 0)
            {
                m_TotalPitch += m_RotSpeed * deltaTime;
            }

            if (deltaX > 0)
            {
                m_TotalYaw -= m_RotSpeed * deltaTime;
            }
            if (deltaX < 0)
            {
                m_TotalYaw += m_RotSpeed * deltaTime;
            }
        }
        else if (left)
        {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            if (deltaY > 0)
            {
                m_Origin -= forwardVec * 3.f * deltaTime;
            }

            if (deltaY < 0)
            {
                m_Origin += forwardVec * 3.f * deltaTime;
            }

            if (deltaX > 0)
            {
                m_TotalYaw += m_RotSpeed * deltaTime;
            }
            if (deltaX < 0)
            {
                m_TotalYaw -= m_RotSpeed * deltaTime;
            }
        }

        // Keyboard movement
        if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS) m_Origin += forwardVec * deltaTime;
        if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS) m_Origin -= forwardVec * deltaTime;
        if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS) m_Origin -= rightVec * deltaTime;
        if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS) m_Origin += rightVec * deltaTime;
        if (glfwGetKey(m_Window, GLFW_KEY_SPACE) == GLFW_PRESS) m_Origin += upVec * deltaTime;
        if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        m_TotalPitch = std::clamp(m_TotalPitch, -89.9f, 89.9f);

        glm::mat4 rot = glm::yawPitchRoll(glm::radians(m_TotalYaw), glm::radians(-m_TotalPitch), 0.0f);
        m_Forward = glm::normalize(glm::vec3(rot * glm::vec4(0, 0, 1, 0)));

        CalculateViewMatrix();

        if (m_FOVAngle != m_PreviousFOVAngle || m_AspectRatio != m_PreviousAspectRatio)
        {
            CalculateProjectionMatrix();
            m_PreviousFOVAngle = m_FOVAngle;
            m_PreviousAspectRatio = m_AspectRatio;
        }
    }

} // namespace GG
