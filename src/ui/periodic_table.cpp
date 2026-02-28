#include "periodic_table.h"
#include "../physics/element.h"
#include <GL/glew.h>
#include <cstring>

namespace ui {

// Standard periodic table layout: [row][col] = atomic number (0 = empty)
static const int layout[10][18] = {
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2},
    { 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 6, 7, 8, 9,10},
    {11,12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,13,14,15,16,17,18},
    {19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36},
    {37,38, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {55,56, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0,47, 0, 0, 0, 0,53,54, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0,79, 0,82, 0, 0, 0,86, 0, 0,92, 0, 0, 0, 0, 0}
};
static const int ROWS = 10, COLS = 18;

// Category → color
static void categoryColor(const char* cat, float& r, float& g, float& b) {
    if (!cat) { r=0.3f; g=0.3f; b=0.3f; return; }
    if (strcmp(cat,"nonmetal")==0)              { r=0.2f; g=0.8f; b=0.4f; }
    else if (strcmp(cat,"noble_gas")==0)        { r=0.4f; g=0.6f; b=0.9f; }
    else if (strcmp(cat,"alkali_metal")==0)     { r=0.9f; g=0.3f; b=0.5f; }
    else if (strcmp(cat,"alkaline_earth")==0)   { r=0.9f; g=0.6f; b=0.2f; }
    else if (strcmp(cat,"transition_metal")==0) { r=0.6f; g=0.5f; b=0.7f; }
    else if (strcmp(cat,"metalloid")==0)        { r=0.5f; g=0.7f; b=0.5f; }
    else if (strcmp(cat,"halogen")==0)          { r=0.3f; g=0.9f; b=0.7f; }
    else if (strcmp(cat,"post_transition_metal")==0) { r=0.6f; g=0.6f; b=0.5f; }
    else if (strcmp(cat,"actinide")==0)         { r=0.5f; g=0.3f; b=0.8f; }
    else                                       { r=0.4f; g=0.4f; b=0.4f; }
}

void PeriodicTableUI::drawCell(float x, float y, float w, float h,
                                int, const char*, float r, float g, float b) {
    // Background quad
    glColor4f(r, g, b, 0.75f);
    glBegin(GL_QUADS);
    glVertex2f(x, y); glVertex2f(x+w, y);
    glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
    // Border
    glColor4f(0.1f, 0.1f, 0.1f, 1.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x+w, y);
    glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();
}

void PeriodicTableUI::render(int windowW, int windowH) {
    if (!visible) return;

    // Switch to 2D orthographic for overlay
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    glOrtho(0, windowW, windowH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    float cellW = 28.0f, cellH = 22.0f;
    float startX = windowW - COLS * cellW - 10;
    float startY = 10.0f;

    auto& pt = physics::PeriodicTable::instance();
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            int z = layout[row][col];
            if (z == 0) continue;
            const auto& el = pt.get(z);
            if (el.atomicNumber == 0) continue;

            float r, g, b;
            categoryColor(el.category.c_str(), r, g, b);
            float x = startX + col * cellW;
            float y = startY + row * cellH;
            drawCell(x, y, cellW - 1, cellH - 1, z, el.symbol.c_str(), r, g, b);
        }
    }

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

bool PeriodicTableUI::handleClick(float mx, float my, int windowW, int) {
    if (!visible) return false;

    float cellW = 28.0f, cellH = 22.0f;
    float startX = windowW - COLS * cellW - 10;
    float startY = 10.0f;

    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            int z = layout[row][col];
            if (z == 0) continue;
            float x = startX + col * cellW;
            float y = startY + row * cellH;
            if (mx >= x && mx <= x + cellW && my >= y && my <= y + cellH) {
                if (callback_) callback_(z);
                return true;
            }
        }
    }
    return false;
}

} // namespace ui
