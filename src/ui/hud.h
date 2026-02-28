#pragma once
#include <string>

namespace ui {

/// Simple HUD overlay for simulation info and controls.
class HUD {
public:
    void render(int windowW, int windowH,
                int atomCount, int molCount,
                float fps, float temperature,
                float totalKE, float totalPE, float bondE,
                const std::string& recentLog);

    bool visible = true;
};

} // namespace ui
