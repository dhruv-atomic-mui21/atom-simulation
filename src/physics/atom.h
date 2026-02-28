#pragma once
#include "element.h"
#include "electron.h"
#include <glm/glm.hpp>
#include <vector>

namespace physics {

struct Bond {
    int otherAtomIdx = -1;
    enum Type { IONIC, COVALENT, METALLIC } type = COVALENT;
    int order = 1;           // single=1, double=2, triple=3
    float strength = 1.0f;   // eV
};

struct Atom {
    // Identity
    int elementZ = 1;
    const ElementData* element = nullptr;

    // Kinematics
    glm::vec3 pos  = glm::vec3(0.0f);
    glm::vec3 vel  = glm::vec3(0.0f);
    glm::vec3 force= glm::vec3(0.0f);

    // Electron state
    std::vector<Electron> electrons;
    int charge = 0;  // net ionic charge (0 = neutral, +1 = lost 1 e⁻, etc.)

    // Bonds
    std::vector<Bond> bonds;

    // Visual
    float visualRadius = 1.0f;

    /// Initialize atom from element data.
    void init(int atomicNumber);

    /// Number of unpaired valence electrons available for bonding.
    int availableValenceElectrons() const;

    /// Does this atom want to gain electrons? (based on electron affinity)
    bool wantsElectron() const;

    /// Does this atom want to lose electrons? (based on ionization energy)
    bool wantsToLoseElectron() const;

    /// Remove outermost electron (ionization).
    Electron removeOuterElectron();

    /// Add an electron.
    void addElectron(const Electron& e);
};

} // namespace physics
