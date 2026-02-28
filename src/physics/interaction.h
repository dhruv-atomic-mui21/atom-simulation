#pragma once
#include "atom.h"
#include <glm/glm.hpp>
#include <vector>

namespace physics {

/// All forces and bonding decisions emerge from physical properties.
class InteractionEngine {
public:
    /// Compute all pairwise forces and update atom.force vectors.
    void computeForces(std::vector<Atom>& atoms);

    /// Check for bond formation/breaking based on energy criteria.
    void updateBonds(std::vector<Atom>& atoms);

    // Tuning parameters
    float coulombK         = 14.4f;    // eV·Å / e² (for Coulomb law)
    float ljEpsilon        = 0.01f;    // eV (Lennard-Jones well depth)
    float bondingRange     = 5.0f;     // Å — max distance to attempt bonding
    float ionicThreshold   = 1.7f;     // Δχ above which → ionic bond
    float temperature      = 300.0f;   // Kelvin

private:
    /// Lennard-Jones force between two neutral atoms.
    glm::vec3 ljForce(const Atom& a, const Atom& b, float dist,
                      glm::vec3 dir) const;

    /// Coulomb force between two charged ions.
    glm::vec3 coulombForce(const Atom& a, const Atom& b, float dist,
                           glm::vec3 dir) const;

    /// Covalent spring force for bonded atoms.
    glm::vec3 bondSpringForce(const Atom& a, const Atom& b,
                              float eqDist, float dist,
                              glm::vec3 dir) const;

    /// Attempt ionic bonding between two atoms.
    bool tryIonicBond(Atom& a, Atom& b, int idxA, int idxB);

    /// Attempt covalent bonding between two atoms.
    bool tryCovalentBond(Atom& a, Atom& b, int idxA, int idxB);

    /// Check if a bond should break (thermal energy vs bond strength).
    bool shouldBreakBond(const Bond& bond) const;
};

} // namespace physics
