#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <string>
#include <functional>

namespace engine {

/// Compiles a shader from source, returns the GL handle.
GLuint compileShader(GLenum type, const char* source);

/// Links vertex + fragment shaders into a program.
GLuint linkProgram(GLuint vertexShader, GLuint fragmentShader);

/// Compiles + links a vertex/fragment pair in one call.
GLuint createShaderProgram(const char* vertSrc, const char* fragSrc);

// ─────────────────────────────────────────────────────────────
// Engine — owns the GLFW window and the top-level render loop.
// ─────────────────────────────────────────────────────────────
class Engine {
public:
    Engine(int width = 1280, int height = 720,
           const char* title = "Element Simulator");
    ~Engine();

    // Non‐copyable.
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    /// Returns true while the window should remain open.
    bool isRunning() const;

    /// Call at the start of each frame.
    void beginFrame();

    /// Call at the end of each frame (swaps buffers, polls events).
    void endFrame();

    GLFWwindow* getWindow() const { return window_; }
    int getWidth()  const { return width_;  }
    int getHeight() const { return height_; }
    float getAspectRatio() const;
    float getDeltaTime() const { return dt_; }

private:
    GLFWwindow* window_ = nullptr;
    int width_, height_;
    float dt_ = 0.0f;
    float lastFrameTime_ = 0.0f;
};

} // namespace engine
