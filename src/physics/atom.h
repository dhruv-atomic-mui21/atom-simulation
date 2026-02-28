#pragma once
#include "element.h"
#include "electron.h"
#include <glm/glm.hpp>
#include <vector>

namespace physics {

struct Bond {
    int otherAtomIdx = -1;
    enum Type { IONIC, COVALENT, METALLIC, HYDROGEN, VDW } type = COVALENT;
    int order = 1;               // single=1, double=2, triple=3
    float strength = 1.0f;       // eV (bond dissociation energy)
    float equilibriumDist = 0;   // Å
    float morseAlpha = 1.0f;     // Morse potential width parameter
};

struct Atom {
    // Identity
    int elementZ = 1;
    const ElementData* element = nullptr;

    // Kinematics (Newtonian: F=ma, Velocity Verlet)
    glm::vec3 pos   = glm::vec3(0.0f);
    glm::vec3 vel   = glm::vec3(0.0f);
    glm::vec3 force = glm::vec3(0.0f);
    float mass = 1.0f;  // amu

    // Electron state
    std::vector<Electron> electrons;
    int charge = 0;              // net ionic charge
    int effectiveValence = 0;    // dynamically computed unpaired valence e-

    // Bonds
    std::vector<Bond> bonds;
    int moleculeId = -1;         // which molecule cluster this belongs to

    // Energy tracking
    float kineticEnergy = 0;
    float potentialEnergy = 0;

    // Visual
    float visualRadius = 1.0f;

    /// Initialize atom from element data.
    void init(int atomicNumber);

    /// Number of unpaired valence electrons available for bonding.
    int availableValenceElectrons() const;

    /// Total bonds currently formed (sum of bond orders).
    int totalBondOrder() const;

    /// Does this atom want to gain electrons? (based on electron affinity)
    bool wantsElectron() const;

    /// Does this atom want to lose electrons? (based on ionization energy)
    bool wantsToLoseElectron() const;

    /// Remove outermost electron (ionization).
    Electron removeOuterElectron();

    /// Add an electron.
    void addElectron(const Electron& e);

    /// Recompute effectiveValence from current state.
    void updateEffectiveValence();
};

} // namespace physics
