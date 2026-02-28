#pragma once

namespace ui {

/// Simple HUD overlay for simulation info and controls.
class HUD {
public:
    void render(int windowW, int windowH, int atomCount,
                float fps, float temperature);

    bool visible = true;
};

} // namespace ui
