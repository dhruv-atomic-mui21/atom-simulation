#include "hud.h"
#include <GL/glew.h>

namespace ui {

void HUD::render(int windowW, int windowH, int atomCount,
                 float fps, float temperature) {
    if (!visible) return;

    // Switch to 2D overlay
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    glOrtho(0, windowW, windowH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    // Draw info background
    float panelW = 200, panelH = 80;
    float x = 10, y = windowH - panelH - 10;
    glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x+panelW, y);
    glVertex2f(x+panelW, y+panelH); glVertex2f(x, y+panelH);
    glEnd();

    // Note: actual text rendering requires a font engine (stb_truetype, freetype, etc.)
    // For now we just draw colored indicator bars as placeholders.
    // Temperature bar
    float tempNorm = temperature / 5000.0f;
    glColor4f(1.0f, 0.3f, 0.1f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(x+10, y+50); glVertex2f(x+10 + 180*tempNorm, y+50);
    glVertex2f(x+10 + 180*tempNorm, y+65); glVertex2f(x+10, y+65);
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

} // namespace ui
