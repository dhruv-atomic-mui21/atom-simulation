#pragma once
#include "atom.h"
#include "interaction.h"
#include "quantum.h"
#include "molecule.h"
#include <vector>

namespace physics {

/// The main simulation container and integrator.
class Simulation {
public:
    Simulation();

    /// Advance the simulation by dt seconds.
    void step(float dt);

    /// Add an atom of the given element at a position.
    void spawnAtom(int atomicNumber, glm::vec3 pos);

    /// Remove all atoms and molecules.
    void clear();

    // Public access to state
    std::vector<Atom>& atoms() { return atoms_; }
    const std::vector<Atom>& atoms() const { return atoms_; }

    const std::vector<Molecule>& molecules() const { return tracker_.molecules(); }

    InteractionEngine& interactions() { return interactions_; }
    const std::vector<InteractionEngine::ReactionEvent>& reactionLog() const {
        return interactions_.reactionLog;
    }

    // World state
    float worldSize = 50.0f;
    float simTime = 0.0f;
    int stepCount = 0;

private:
    std::vector<Atom> atoms_;
    InteractionEngine interactions_;
    MoleculeTracker   tracker_;
    QuantumSampler    sampler_;

    /// Keep atoms inside the simulation box.
    void applyBoundary(Atom& a);

    /// Berendsen thermostat for temperature control.
    void berendsenThermostat(float dt, float targetT, float tau = 100.0f);
};

} // namespace physics
