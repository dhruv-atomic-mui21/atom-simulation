#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace engine {

Camera::Camera() = default;

glm::vec3 Camera::getPosition() const {
    float ce = std::clamp(elevation_, 0.01f, static_cast<float>(M_PI) - 0.01f);
    return target_ + glm::vec3(
        radius_ * std::sin(ce) * std::cos(azimuth_),
        radius_ * std::cos(ce),
        radius_ * std::sin(ce) * std::sin(azimuth_)
    );
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(getPosition(), target_, glm::vec3(0, 1, 0));
}

glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, nearZ, farZ);
}

void Camera::setTarget(const glm::vec3& t) {
    target_ = t;
}

void Camera::attachToWindow(GLFWwindow* win) {
    glfwSetWindowUserPointer(win, this);
    glfwSetMouseButtonCallback(win, mouseButtonCB);
    glfwSetCursorPosCallback(win,   cursorPosCB);
    glfwSetScrollCallback(win,      scrollCB);
}

// ── Static GLFW callbacks ────────────────────────────────────
void Camera::mouseButtonCB(GLFWwindow* win, int button, int action, int) {
    auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(win));
    if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (action == GLFW_PRESS) {
            cam->dragging_ = true;
            glfwGetCursorPos(win, &cam->lastX_, &cam->lastY_);
        } else if (action == GLFW_RELEASE) {
            cam->dragging_ = false;
        }
    }
}

void Camera::cursorPosCB(GLFWwindow* win, double x, double y) {
    auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(win));
    if (!cam->dragging_) { cam->lastX_ = x; cam->lastY_ = y; return; }
    float dx = static_cast<float>(x - cam->lastX_);
    float dy = static_cast<float>(y - cam->lastY_);
    cam->azimuth_   += dx * cam->orbitSpeed_;
    cam->elevation_ -= dy * cam->orbitSpeed_;
    cam->elevation_  = std::clamp(cam->elevation_, 0.01f, static_cast<float>(M_PI) - 0.01f);
    cam->lastX_ = x;
    cam->lastY_ = y;
}

void Camera::scrollCB(GLFWwindow* win, double, double yoffset) {
    auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(win));
    cam->radius_ -= static_cast<float>(yoffset) * cam->zoomSpeed_;
    if (cam->radius_ < 1.0f) cam->radius_ = 1.0f;
}

} // namespace engine
