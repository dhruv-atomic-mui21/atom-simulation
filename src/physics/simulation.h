#pragma once
#include "atom.h"
#include "interaction.h"
#include "quantum.h"
#include <vector>

namespace physics {

class Simulation {
public:
    Simulation();

    /// Add an atom of the given element at a position.
    void spawnAtom(int atomicNumber, glm::vec3 pos);

    /// Remove all atoms.
    void clear();

    /// Advance the simulation by dt seconds.
    void step(float dt);

    // Public access to state
    std::vector<Atom>& atoms() { return atoms_; }
    const std::vector<Atom>& atoms() const { return atoms_; }
    InteractionEngine& interactions() { return interactions_; }

    // World bounds (reflective box)
    float worldSize = 50.0f;

private:
    std::vector<Atom> atoms_;
    InteractionEngine interactions_;
    QuantumSampler sampler_;

    void applyBoundary(Atom& a);
};

} // namespace physics
