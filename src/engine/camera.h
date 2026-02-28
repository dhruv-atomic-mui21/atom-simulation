#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

namespace engine {

// ─────────────────────────────────────────────────────────────
// Camera — spherical orbit camera adapted from ref-repo.
// ─────────────────────────────────────────────────────────────
class Camera {
public:
    Camera();

    /// Matrices.
    glm::mat4 getViewMatrix()       const;
    glm::mat4 getProjectionMatrix(float aspect) const;
    glm::vec3 getPosition()         const;

    /// Attach GLFW callbacks to a window.
    void attachToWindow(GLFWwindow* window);

    /// Smoothly interpolate the target toward a new position.
    void setTarget(const glm::vec3& t);

    // Tuning
    float fov     = 45.0f;
    float nearZ   = 0.1f;
    float farZ    = 5000.0f;

private:
    // Spherical coords
    float radius_   = 80.0f;
    float azimuth_  = 0.0f;
    float elevation_= 1.2f;   // slightly above equator
    glm::vec3 target_ = glm::vec3(0.0f);

    // Interaction state
    bool  dragging_ = false;
    double lastX_ = 0, lastY_ = 0;

    // Sensitivity
    float orbitSpeed_ = 0.005f;
    float zoomSpeed_  = 5.0f;

    // GLFW callbacks (static wrappers → instance via user pointer)
    static void mouseButtonCB(GLFWwindow*, int, int, int);
    static void cursorPosCB(GLFWwindow*, double, double);
    static void scrollCB(GLFWwindow*, double, double);
};

} // namespace engine
