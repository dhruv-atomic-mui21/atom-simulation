#include "engine/engine.h"
#include "engine/camera.h"
#include "engine/renderer.h"
#include "physics/element.h"
#include "physics/simulation.h"
#include "physics/quantum.h"
#include "ui/periodic_table.h"
#include "ui/hud.h"

#include <iostream>
#include <random>
#include <algorithm>

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
        case GLFW_KEY_UP:     inter.temperature = std::min(inter.temperature + 100.0f, 10000.0f); break;
        case GLFW_KEY_DOWN:   inter.temperature = std::max(inter.temperature - 100.0f, 10.0f); break;
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
        // Try periodic table first
        if (g_ptUI && g_ptUI->handleClick(
                static_cast<float>(mx), static_cast<float>(my),
                g_windowW, g_windowH)) {
            return; // consumed by UI
        }
    }
    // Otherwise pass to camera
    if (g_camera) {
        g_camera->attachToWindow(win); // re-attach is harmless
    }
}

// ═══════════════════════════════════════════════════════════
//  Main
// ═══════════════════════════════════════════════════════════
int main() {
    // --- Engine ---
    engine::Engine eng(1280, 720, "Element Simulator — Emergent Chemistry");
    engine::Camera camera;
    engine::Renderer renderer;
    renderer.init();

    // --- Physics ---
    auto& pt = physics::PeriodicTable::instance();
    if (!pt.loadFromFile("data/elements.json")) {
        std::cerr << "Could not load elements database!\n";
        return 1;
    }

    physics::Simulation sim;
    physics::QuantumSampler sampler;

    // --- UI ---
    ui::PeriodicTableUI ptUI;
    ui::HUD hud;

    // Wire spawn callback: periodic table click → spawn atom at random pos
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> posDist(-15.0f, 15.0f);
    ptUI.setSpawnCallback([&](int z) {
        glm::vec3 pos(posDist(rng), posDist(rng), posDist(rng));
        sim.spawnAtom(z, pos);
        const auto& el = pt.get(z);
        std::cout << "Spawned " << el.name << " (" << el.symbol
                  << ") at (" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
    });

    // --- Set globals for callbacks ---
    g_sim    = &sim;
    g_camera = &camera;
    g_ptUI   = &ptUI;

    camera.attachToWindow(eng.getWindow());
    glfwSetKeyCallback(eng.getWindow(), keyCallback);

    // Spawn a few starter atoms
    sim.spawnAtom(1, glm::vec3(-5, 0, 0));  // H
    sim.spawnAtom(1, glm::vec3( 5, 0, 0));  // H
    sim.spawnAtom(8, glm::vec3( 0, 5, 0));  // O

    std::cout << "\n=== Element Simulator ===\n"
              << "Controls:\n"
              << "  Tab       — toggle periodic table\n"
              << "  1-8       — quick spawn (H, He, C, O, Na, Cl, Fe, Au)\n"
              << "  Up/Down   — adjust temperature\n"
              << "  Delete    — clear all atoms\n"
              << "  Mouse     — orbit camera\n"
              << "  Scroll    — zoom\n\n";

    // --- Main loop ---
    float fpsTimer = 0, frameCount = 0, fps = 0;
    float physDt = 0.016f; // fixed physics timestep

    while (eng.isRunning()) {
        eng.beginFrame();
        float dt = eng.getDeltaTime();
        g_windowW = eng.getWidth();
        g_windowH = eng.getHeight();

        // FPS counter
        fpsTimer += dt; frameCount++;
        if (fpsTimer > 1.0f) {
            fps = frameCount / fpsTimer;
            fpsTimer = 0; frameCount = 0;
        }

        // --- Physics step ---
        sim.step(physDt);

        // --- Build render data ---
        auto& atoms = sim.atoms();

        // Atom spheres
        std::vector<engine::SphereInstance> spheres;
        for (const auto& a : atoms) {
            engine::SphereInstance s;
            s.position = a.pos;
            s.radius = a.visualRadius;
            s.color = glm::vec4(a.element->color, 1.0f);
            spheres.push_back(s);
        }

        // Bonds
        std::vector<engine::BondInstance> bondInstances;
        for (size_t i = 0; i < atoms.size(); ++i) {
            for (const auto& b : atoms[i].bonds) {
                if (b.otherAtomIdx > static_cast<int>(i)) { // avoid duplicates
                    engine::BondInstance bi;
                    bi.posA = atoms[i].pos;
                    bi.posB = atoms[b.otherAtomIdx].pos;
                    bi.thickness = 0.1f * b.order;
                    // Color by bond type
                    if (b.type == physics::Bond::IONIC)
                        bi.color = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);
                    else if (b.type == physics::Bond::COVALENT)
                        bi.color = glm::vec4(0.5f, 0.8f, 1.0f, 1.0f);
                    else
                        bi.color = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
                    bondInstances.push_back(bi);
                }
            }
        }

        // Electron clouds (sample a few points per atom)
        std::vector<engine::CloudPoint> cloud;
        const int samplesPerAtom = 200;
        for (const auto& a : atoms) {
            // Build (n,l) list for Slater shielding
            std::vector<std::pair<int,int>> nlList;
            for (const auto& e : a.electrons)
                nlList.push_back({e.qn.n, e.qn.l});

            // Only visualize valence shell
            int maxN = physics::outermostShell(a.electrons);
            for (const auto& e : a.electrons) {
                if (e.qn.n != maxN) continue;
                float zEff = physics::QuantumSampler::computeZeff(
                    a.elementZ, e.qn.n, e.qn.l, nlList);
                for (int s = 0; s < samplesPerAtom / std::max(1, a.element->valenceElectrons); ++s) {
                    glm::vec3 localPos = sampler.samplePosition(e.qn.n, e.qn.l, e.qn.m, zEff);
                    float r = glm::length(localPos);
                    float theta = (r > 1e-6f) ? std::acos(localPos.y / r) : 0;
                    float phi   = std::atan2(localPos.z, localPos.x);
                    float prob  = physics::QuantumSampler::probabilityDensity(
                        e.qn.n, e.qn.l, e.qn.m, zEff, r, theta, phi);
                    glm::vec4 col = physics::QuantumSampler::heatmapColor(
                        prob * 500.0f * std::pow(3.0f, e.qn.n));
                    col.a = 0.4f;
                    cloud.push_back({a.pos + localPos, col});
                }
            }
        }

        // --- Render ---
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix(eng.getAspectRatio());

        renderer.drawElectronCloud(cloud, view, proj);
        renderer.drawAtoms(spheres, view, proj);
        renderer.drawBonds(bondInstances, view, proj);

        // UI overlays
        ptUI.render(g_windowW, g_windowH);
        hud.render(g_windowW, g_windowH,
                   static_cast<int>(atoms.size()),
                   fps, sim.interactions().temperature);

        eng.endFrame();
    }

    return 0;
}
