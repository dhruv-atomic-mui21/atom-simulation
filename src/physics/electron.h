#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace physics {

/// Quantum numbers for a single electron.
struct QuantumNumbers {
    int n = 1;   // principal
    int l = 0;   // angular momentum
    int m = 0;   // magnetic
    int s = 1;   // spin (+1 or -1, representing +1/2 or -1/2)
};

/// A single electron with quantum state and visual position.
struct Electron {
    QuantumNumbers qn;
    float zEff      = 1.0f;  // effective nuclear charge (Slater)
    glm::vec3 pos   = glm::vec3(0.0f);
    bool shared     = false;  // true if in a covalent bond
    int  sharedWith = -1;     // index of bonded atom (-1 = none)
};

/// Fill electron shells for a given atomic number using Aufbau.
/// Returns a vector of Electron structs with correct quantum numbers.
std::vector<Electron> fillElectronShells(int atomicNumber);

/// Count valence electrons from an electron configuration.
int countValenceElectrons(const std::vector<Electron>& electrons);

/// Get the outermost shell number.
int outermostShell(const std::vector<Electron>& electrons);

} // namespace physics
