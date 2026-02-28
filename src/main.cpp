#include "engine/engine.h"
#include "engine/camera.h"
#include "engine/renderer.h"
#include "physics/element.h"
#include "physics/simulation.h"
#include "physics/molecule.h"
#include "physics/quantum.h"
#include "ui/periodic_table.h"
#include "ui/hud.h"

#include <iostream>
#include <random>
#include <algorithm>
#include <iomanip>

// ═══════════════════════════════════════════════════════════
//  Global state for GLFW callbacks
// ═══════════════════════════════════════════════════════════
static physics::Simulation* g_sim       = nullptr;
static engine::Camera*      g_camera    = nullptr;
static ui::PeriodicTableUI* g_ptUI      = nullptr;
static int g_windowW = 1280, g_windowH = 720;

static void keyCallback(GLFWwindow* win, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    if (!g_sim) return;

    auto& inter = g_sim->interactions();
    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(win, GLFW_TRUE); break;
        case GLFW_KEY_TAB:    if (g_ptUI) g_ptUI->visible = !g_ptUI->visible; break;
        case GLFW_KEY_UP:     inter.temperature = std::min(inter.temperature + 100.0f, 10000.0f); 
                              std::cout << "[Temp] " << inter.temperature << "K\n"; break;
        case GLFW_KEY_DOWN:   inter.temperature = std::max(inter.temperature - 100.0f, 10.0f);
                              std::cout << "[Temp] " << inter.temperature << "K\n"; break;
        case GLFW_KEY_DELETE: g_sim->clear(); break;
        // Quick spawn shortcuts
        case GLFW_KEY_1: g_sim->spawnAtom(1,  glm::vec3(0)); break; // H
        case GLFW_KEY_2: g_sim->spawnAtom(2,  glm::vec3(0)); break; // He
        case GLFW_KEY_3: g_sim->spawnAtom(6,  glm::vec3(0)); break; // C
        case GLFW_KEY_4: g_sim->spawnAtom(8,  glm::vec3(0)); break; // O
        case GLFW_KEY_5: g_sim->spawnAtom(11, glm::vec3(0)); break; // Na
        case GLFW_KEY_6: g_sim->spawnAtom(17, glm::vec3(0)); break; // Cl
        case GLFW_KEY_7: g_sim->spawnAtom(26, glm::vec3(0)); break; // Fe
        case GLFW_KEY_8: g_sim->spawnAtom(79, glm::vec3(0)); break; // Au
        default: break;
    }
}

static void mouseButtonCallback(GLFWwindow* win, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mx, my;
        glfwGetCursorPos(win, &mx, &my);
        if (g_ptUI && g_ptUI->handleClick(static_cast<float>(mx), static_cast<float>(my), 
                                          g_windowW, g_windowH)) return;
    }
    if (g_camera) g_camera->attachToWindow(win);
}

// ═══════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════
int main() {
    engine::Engine eng(1280, 720, "Universal Simulator — Emergent Chemistry");
    engine::Camera camera;
    engine::Renderer renderer;
    renderer.init();

    // Physics
    auto& pt = physics::PeriodicTable::instance();
    if (!pt.loadFromFile("data/elements.json")) {
        std::cerr << "Could not load 118-element database!\n";
        return 1;
    }

    physics::Simulation sim;
    physics::QuantumSampler sampler;
    ui::PeriodicTableUI ptUI;
    ui::HUD hud;

    // Spawn callback: pt click -> random pos
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> posDist(-10.0f, 10.0f);
    ptUI.setSpawnCallback([&](int z) {
        glm::vec3 pos(posDist(rng), posDist(rng), posDist(rng));
        sim.spawnAtom(z, pos);
    });

    g_sim = &sim; g_camera = &camera; g_ptUI = &ptUI;
    camera.attachToWindow(eng.getWindow());
    glfwSetKeyCallback(eng.getWindow(), keyCallback);
    glfwSetMouseButtonCallback(eng.getWindow(), mouseButtonCallback);

    // Starter atoms (let's form a water molecule and some salt)
    sim.spawnAtom(8, glm::vec3(0, 0, 0));    // O
    sim.spawnAtom(1, glm::vec3(1.5, 1, 0));  // H
    sim.spawnAtom(1, glm::vec3(-1.5, 1, 0)); // H

    sim.spawnAtom(11, glm::vec3(5, -5, 0));  // Na
    sim.spawnAtom(17, glm::vec3(6, -5, 0));  // Cl

    std::cout << "\n=== Universal Simulator ===\n"
              << "Physics: Velocity Verlet (eV, Å, amu, fs)\n"
              << "Chemistry: Emergent (Morse bonds, Born-Haber ionic, VSEPR angles)\n"
              << "Controls: Tab to toggle PT, 1-8 for presets, Up/Down for temp.\n\n";

    float fpsTimer = 0, frameCount = 0, fps = 0;
    float physDt = 1.0f; // 1 fs integration step
    int lastLogCount = 0;
    std::string latestReaction = "";

    while (eng.isRunning()) {
        eng.beginFrame();
        float dt = eng.getDeltaTime();
        g_windowW = eng.getWidth(); g_windowH = eng.getHeight();

        fpsTimer += dt; frameCount++;
        if (fpsTimer > 1.0f) {
            fps = frameCount / fpsTimer;
            fpsTimer = 0; frameCount = 0;
            
            // Console print high-level stats every second
            const auto& mols = sim.molecules();
            if (!mols.empty() && mols.size() < sim.atoms().size()) {
                std::cout << "[Molecules] ";
                for (const auto& m : mols) if (m.atomIndices.size() > 1) std::cout << m.formula << " ";
                std::cout << "\n";
            }
        }

        // --- Physics step ---
        // Run physics at ~1000 steps per frame (1 ps per frame) for stability
        // To keep it smooth but interactive, we'll do 50 substeps.
        for(int i=0; i<50; ++i) {
            sim.step(physDt);
        }

        // Print new reactions
        const auto& logs = sim.reactionLog();
        if (logs.size() > lastLogCount) {
            for (size_t i = lastLogCount; i < logs.size(); ++i) {
                std::cout << "[Reaction] " << std::fixed << std::setprecision(1) 
                          << logs[i].time << "fs: " << logs[i].description << "\n";
                latestReaction = logs[i].description;
            }
            lastLogCount = logs.size();
        }

        // --- Render Data ---
        const auto& atoms = sim.atoms();
        std::vector<engine::SphereInstance> spheres;
        for (const auto& a : atoms) {
            engine::SphereInstance s;
            s.position = a.pos;
            s.radius = a.visualRadius;
            s.color = glm::vec4(a.element->color, 1.0f);
            spheres.push_back(s);
        }

        std::vector<engine::BondInstance> bondInstances;
        for (size_t i = 0; i < atoms.size(); ++i) {
            for (const auto& b : atoms[i].bonds) {
                if (b.otherAtomIdx > static_cast<int>(i)) {
                    engine::BondInstance bi;
                    bi.posA = atoms[i].pos;
                    bi.posB = atoms[b.otherAtomIdx].pos;
                    bi.thickness = 0.1f * b.order;
                    if (b.type == physics::Bond::IONIC)
                        bi.color = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f); // Gold = Ionic
                    else if (b.type == physics::Bond::COVALENT)
                        bi.color = glm::vec4(0.5f, 0.8f, 1.0f, 1.0f); // Blue = Covalent
                    else
                        bi.color = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
                    bondInstances.push_back(bi);
                }
            }
        }

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(eng.getAspectRatio());

        renderer.drawAtoms(spheres, view, proj);
        renderer.drawBonds(bondInstances, view, proj);

        ptUI.render(g_windowW, g_windowH);
        hud.render(g_windowW, g_windowH,
                   static_cast<int>(atoms.size()),
                   static_cast<int>(sim.molecules().size()),
                   fps, sim.interactions().temperature,
                   sim.interactions().totalKE,
                   sim.interactions().totalPE,
                   sim.interactions().totalBondE,
                   latestReaction);

        eng.endFrame();
    }

    return 0;
}
