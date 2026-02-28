#pragma once
#include "atom.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace physics {

/// All forces and bonding decisions emerge from physical properties.
/// No predefined reaction tables — chemistry is computed from first principles.
class InteractionEngine {
public:
    /// Compute all pairwise forces and update atom.force vectors.
    void computeForces(std::vector<Atom>& atoms);

    /// Check for bond formation/breaking based on energy criteria.
    void updateBonds(std::vector<Atom>& atoms);

    // Simulation parameters
    float temperature      = 300.0f;   // Kelvin
    float pressure         = 1.0f;     // atm (future use)

    // Physical constants (eV, Å, amu unit system)
    static constexpr float kB     = 8.617e-5f;    // eV/K
    static constexpr float coulK  = 14.4f;        // eV·Å/e²

    // Tuning
    float bondingRange     = 5.0f;     // Å — max distance to attempt bonding
    float ionicThreshold   = 1.7f;     // Δχ above which → ionic bond
    float ljEpsilon        = 0.01f;    // eV (LJ well depth baseline)
    float cutoffDist       = 20.0f;    // Å — force cutoff
    float switchDist       = 15.0f;    // Å — start smoothing to zero

    // Statistics
    float totalKE = 0, totalPE = 0, totalBondE = 0;
    int bondFormedCount = 0, bondBrokenCount = 0;

    // Reaction log
    struct ReactionEvent {
        float time = 0;
        std::string description;
    };
    std::vector<ReactionEvent> reactionLog;
    float simTime = 0;

private:
    // ── Force models ──
    /// Morse potential force for bonded pairs: V = De*(1-exp(-α(r-re)))²
    glm::vec3 morseForce(const Atom& a, const Atom& b, const Bond& bond,
                         float dist, glm::vec3 dir) const;

    /// Coulomb force between charged ions
    glm::vec3 coulombForce(const Atom& a, const Atom& b,
                           float dist, glm::vec3 dir) const;

    /// Lennard-Jones 6-12 for non-bonded van der Waals
    glm::vec3 ljForce(const Atom& a, const Atom& b,
                      float dist, glm::vec3 dir) const;

    /// VSEPR bond-angle restoring force
    void applyAngleForces(std::vector<Atom>& atoms);

    /// Smooth switching function for force cutoff
    float switchingFunction(float dist) const;

    // ── Emergent bonding decisions ──
    /// Attempt ionic bonding (Born-Haber energy check)
    bool tryIonicBond(Atom& a, Atom& b, int idxA, int idxB);

    /// Attempt covalent bonding (overlap energy check)
    bool tryCovalentBond(Atom& a, Atom& b, int idxA, int idxB);

    /// Compute estimated bond dissociation energy
    float estimateBondEnergy(const Atom& a, const Atom& b,
                             Bond::Type type, int order) const;

    /// Get VSEPR ideal bond angle from steric number
    static float idealBondAngle(int stericNumber);

    /// Should this bond break? (energy + thermal check)
    bool shouldBreakBond(const Atom& a, const Atom& b,
                         const Bond& bond, float dist) const;
};

} // namespace physics
