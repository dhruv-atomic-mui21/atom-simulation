#include "simulation.h"
#include <algorithm>

namespace physics {

Simulation::Simulation() = default;

void Simulation::spawnAtom(int atomicNumber, glm::vec3 pos) {
    Atom a;
    a.init(atomicNumber);
    a.pos = pos;

    // Thermal velocity distribution using Maxwell-Boltzmann
    float kT = InteractionEngine::kB * interactions_.temperature;
    float vrms = std::sqrt(3.0f * kT / a.mass); // simple 1D RMS velocity approximation
    std::uniform_real_distribution<float> vdist(-vrms, vrms);
    std::mt19937 rng(std::random_device{}());
    a.vel = glm::vec3(vdist(rng), vdist(rng), vdist(rng));

    atoms_.push_back(a);

    // Update bonds since we added a new atom
    interactions_.updateBonds(atoms_);
    tracker_.update(atoms_.data(), static_cast<int>(atoms_.size()));
}

void Simulation::clear() {
    atoms_.clear();
    interactions_.reactionLog.clear();
    simTime = 0.0f;
    stepCount = 0;
    tracker_.update(atoms_.data(), 0);
}

void Simulation::berendsenThermostat(float dt, float targetT, float tau) {
    if (atoms_.empty() || targetT < 1.0f) return;

    // Calculate current temperature
    float totalKE = 0;
    for (const auto& a : atoms_) {
        totalKE += 0.5f * a.mass * glm::dot(a.vel, a.vel);
    }
    // T = (2/3) * (KE / N) / kB
    float currentT = (2.0f / 3.0f) * (totalKE / atoms_.size()) / InteractionEngine::kB;
    if (currentT < 1.0f) currentT = 1.0f;

    // Scale factor
    float lambda = std::sqrt(1.0f + (dt / tau) * ((targetT / currentT) - 1.0f));

    // Prevent extreme scaling in a single step
    lambda = std::clamp(lambda, 0.9f, 1.1f);

    for (auto& a : atoms_) {
        a.vel *= lambda;
    }
}

void Simulation::step(float dt) {
    if (atoms_.empty()) return;

    // ── Velocity Verlet Integration ──

    // 1. Half-kick v(t + dt/2) = v(t) + 0.5*a(t)*dt
    for (auto& a : atoms_) {
        if (a.mass > 0) {
            float invMass = 1.0f / a.mass;
            a.vel += 0.5f * dt * a.force * invMass;
        }
    }

    // 2. Drift r(t + dt) = r(t) + v(t + dt/2)*dt
    for (auto& a : atoms_) {
        a.pos += dt * a.vel;
        applyBoundary(a);
    }

    // 3. Update Forces a(t + dt)
    interactions_.simTime = simTime;
    interactions_.computeForces(atoms_);

    // 4. Half-kick v(t + dt) = v(t + dt/2) + 0.5*a(t+dt)*dt
    for (auto& a : atoms_) {
        if (a.mass > 0) {
            float invMass = 1.0f / a.mass;
            a.vel += 0.5f * dt * a.force * invMass;
        }
    }

    // ── Thermostat ──
    // Apply temperature control (tau = 100.0f is a typical relaxation time)
    berendsenThermostat(dt, interactions_.temperature, 100.0f);

    // ── Emergent Chemistry (Bond updates) ──
    // We don't need to check bonds every single integration step.
    // Checking every 10 steps saves massive CPU time and allows
    // atoms to vibrate naturally before forming/breaking bonds.
    if (stepCount % 10 == 0) {
        int oldCount = interactions_.bondFormedCount - interactions_.bondBrokenCount;
        interactions_.updateBonds(atoms_);
        int newCount = interactions_.bondFormedCount - interactions_.bondBrokenCount;

        // If bonds changed, update molecules
        if (oldCount != newCount || stepCount == 0) {
            tracker_.update(atoms_.data(), static_cast<int>(atoms_.size()));
        }
    }

    simTime += dt;
    stepCount++;
}

void Simulation::applyBoundary(Atom& a) {
    float hw = worldSize;
    // Reflective box boundary
    for (int axis = 0; axis < 3; ++axis) {
        if (a.pos[axis] > hw) {
            a.pos[axis] = hw;
            a.vel[axis] *= -0.5f; // lose some energy on bounce
        } else if (a.pos[axis] < -hw) {
            a.pos[axis] = -hw;
            a.vel[axis] *= -0.5f;
        }
    }
}

} // namespace physics
