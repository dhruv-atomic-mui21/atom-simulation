#include "simulation.h"
#include <algorithm>

namespace physics {

Simulation::Simulation() = default;

void Simulation::spawnAtom(int atomicNumber, glm::vec3 pos) {
    Atom a;
    a.init(atomicNumber);
    a.pos = pos;
    // Small random velocity (thermal)
    float kT = 8.617e-5f * interactions_.temperature;
    std::uniform_real_distribution<float> vdist(-kT * 10.0f, kT * 10.0f);
    std::mt19937 rng(std::random_device{}());
    a.vel = glm::vec3(vdist(rng), vdist(rng), vdist(rng));
    atoms_.push_back(a);
}

void Simulation::clear() {
    atoms_.clear();
}

void Simulation::step(float dt) {
    if (atoms_.empty()) return;

    // Velocity Verlet integration
    // Step 1: half-kick
    for (auto& a : atoms_) {
        float invMass = 1.0f / a.element->atomicMass;
        a.vel += 0.5f * dt * a.force * invMass;
    }

    // Step 2: drift
    for (auto& a : atoms_) {
        a.pos += dt * a.vel;
        applyBoundary(a);
    }

    // Step 3: compute new forces
    interactions_.computeForces(atoms_);

    // Step 4: half-kick with new forces
    for (auto& a : atoms_) {
        float invMass = 1.0f / a.element->atomicMass;
        a.vel += 0.5f * dt * a.force * invMass;
    }

    // Step 5: damping (simple thermostat)
    for (auto& a : atoms_) {
        a.vel *= 0.999f;
    }

    // Step 6: bond updates (less frequent — every few frames is fine)
    interactions_.updateBonds(atoms_);
}

void Simulation::applyBoundary(Atom& a) {
    float hw = worldSize;
    for (int axis = 0; axis < 3; ++axis) {
        if (a.pos[axis] > hw)  { a.pos[axis] = hw;  a.vel[axis] *= -0.5f; }
        if (a.pos[axis] < -hw) { a.pos[axis] = -hw; a.vel[axis] *= -0.5f; }
    }
}

} // namespace physics
