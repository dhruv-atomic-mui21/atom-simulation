#pragma once
#include <functional>

namespace ui {

/// Minimal periodic table overlay rendered with OpenGL.
/// Click an element cell → fires a callback with its atomic number.
class PeriodicTableUI {
public:
    using SpawnCallback = std::function<void(int atomicNumber)>;

    void setSpawnCallback(SpawnCallback cb) { callback_ = cb; }

    /// Render the periodic table overlay (call during frame).
    void render(int windowW, int windowH);

    /// Handle mouse click — returns true if click was consumed.
    bool handleClick(float mouseX, float mouseY, int windowW, int windowH);

    bool visible = true;

private:
    SpawnCallback callback_;
    void drawCell(float x, float y, float w, float h,
                  int atomicNumber, const char* symbol,
                  float r, float g, float b);
};

} // namespace ui
