#include "interaction.h"
#include <cmath>
#include <algorithm>
#include <sstream>

namespace physics {

// ═══════════════════════════════════════════════════════════
//  Switching function: smooth force cutoff between switchDist and cutoffDist
// ═══════════════════════════════════════════════════════════
float InteractionEngine::switchingFunction(float dist) const {
    if (dist < switchDist) return 1.0f;
    if (dist > cutoffDist) return 0.0f;
    float t = (dist - switchDist) / (cutoffDist - switchDist);
    // Smooth cubic: 1 - 3t² + 2t³
    return 1.0f - 3.0f * t * t + 2.0f * t * t * t;
}

// ═══════════════════════════════════════════════════════════
//  Morse potential: V(r) = De * (1 - exp(-α(r-re)))²
//  F(r) = 2*De*α * (1 - exp(-α(r-re))) * exp(-α(r-re)) * r_hat
// ═══════════════════════════════════════════════════════════
glm::vec3 InteractionEngine::morseForce(const Atom& a, const Atom& b,
                                         const Bond& bond, float dist,
                                         glm::vec3 dir) const {
    float De = bond.strength;
    float alpha = bond.morseAlpha;
    float re = bond.equilibriumDist;
    if (De < 1e-6f || dist < 0.1f) return glm::vec3(0);

    float expt = std::exp(-alpha * (dist - re));
    float magnitude = 2.0f * De * alpha * (1.0f - expt) * expt;
    return -magnitude * dir; // attractive toward equilibrium
}

// ═══════════════════════════════════════════════════════════
//  Coulomb force: F = k * q1 * q2 / r² with soft core
// ═══════════════════════════════════════════════════════════
glm::vec3 InteractionEngine::coulombForce(const Atom& a, const Atom& b,
                                           float dist, glm::vec3 dir) const {
    if (a.charge == 0 && b.charge == 0) return glm::vec3(0);
    float softDist = std::max(dist, 0.5f);
    float magnitude = coulK * a.charge * b.charge / (softDist * softDist);
    return magnitude * switchingFunction(dist) * dir;
}

// ═══════════════════════════════════════════════════════════
//  Lennard-Jones 6-12: V = 4ε[(σ/r)¹² - (σ/r)⁶]
// ═══════════════════════════════════════════════════════════
glm::vec3 InteractionEngine::ljForce(const Atom& a, const Atom& b,
                                      float dist, glm::vec3 dir) const {
    float sigma = (a.element->vdwRadius + b.element->vdwRadius) / 200.0f; // pm→Å
    float softDist = std::max(dist, 0.5f);
    float sr6 = std::pow(sigma / softDist, 6.0f);
    float magnitude = 24.0f * ljEpsilon * (2.0f * sr6 * sr6 - sr6) / softDist;
    return magnitude * switchingFunction(dist) * dir;
}

// ═══════════════════════════════════════════════════════════
//  VSEPR bond angle forces
// ═══════════════════════════════════════════════════════════
float InteractionEngine::idealBondAngle(int stericNumber) {
    switch (stericNumber) {
        case 2: return 180.0f;   // linear
        case 3: return 120.0f;   // trigonal planar
        case 4: return 109.47f;  // tetrahedral
        case 5: return 90.0f;    // trigonal bipyramidal (equatorial-axial)
        case 6: return 90.0f;    // octahedral
        default: return 109.47f;
    }
}

void InteractionEngine::applyAngleForces(std::vector<Atom>& atoms) {
    const float kAngle = 2.0f; // eV/rad² — angle spring constant

    for (size_t i = 0; i < atoms.size(); ++i) {
        auto& center = atoms[i];
        int nBonds = static_cast<int>(center.bonds.size());
        if (nBonds < 2) continue;

        // Steric number = bonds + lone pairs
        int lonePairs = std::max(0, (center.element->valenceElectrons
                                     - center.totalBondOrder()) / 2);
        int stericNumber = nBonds + lonePairs;
        float idealDeg = idealBondAngle(stericNumber);
        float idealRad = idealDeg * 3.14159265f / 180.0f;

        // Apply angle restoring force to each pair of bonded neighbors
        for (int bi = 0; bi < nBonds; ++bi) {
            for (int bj = bi + 1; bj < nBonds; ++bj) {
                int idxA = center.bonds[bi].otherAtomIdx;
                int idxB = center.bonds[bj].otherAtomIdx;
                if (idxA < 0 || idxB < 0) continue;
                if (idxA >= (int)atoms.size() || idxB >= (int)atoms.size()) continue;

                glm::vec3 rA = atoms[idxA].pos - center.pos;
                glm::vec3 rB = atoms[idxB].pos - center.pos;
                float lenA = glm::length(rA);
                float lenB = glm::length(rB);
                if (lenA < 0.01f || lenB < 0.01f) continue;

                rA /= lenA;
                rB /= lenB;
                float cosAngle = glm::clamp(glm::dot(rA, rB), -1.0f, 1.0f);
                float angle = std::acos(cosAngle);
                float dAngle = angle - idealRad;

                // Torque → force on outer atoms
                // Force perpendicular to bond direction
                float forceMag = kAngle * dAngle;

                // Perpendicular components
                glm::vec3 perpA = rB - cosAngle * rA;
                float perpAlen = glm::length(perpA);
                if (perpAlen > 0.001f) {
                    perpA /= perpAlen;
                    atoms[idxA].force += forceMag / lenA * perpA;
                    center.force -= forceMag / lenA * perpA;
                }

                glm::vec3 perpB = rA - cosAngle * rB;
                float perpBlen = glm::length(perpB);
                if (perpBlen > 0.001f) {
                    perpB /= perpBlen;
                    atoms[idxB].force += forceMag / lenB * perpB;
                    center.force -= forceMag / lenB * perpB;
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════
//  Main force computation
// ═══════════════════════════════════════════════════════════
void InteractionEngine::computeForces(std::vector<Atom>& atoms) {
    for (auto& a : atoms) a.force = glm::vec3(0.0f);
    totalPE = 0;
    totalKE = 0;

    int n = static_cast<int>(atoms.size());
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            glm::vec3 diff = atoms[i].pos - atoms[j].pos;
            float dist = glm::length(diff);
            if (dist < 0.01f || dist > cutoffDist) continue;
            glm::vec3 dir = diff / dist;

            // Check if bonded
            const Bond* bond = nullptr;
            for (const auto& b : atoms[i].bonds) {
                if (b.otherAtomIdx == j) { bond = &b; break; }
            }

            glm::vec3 f(0.0f);
            if (bond) {
                // Bonded: Morse potential
                f += morseForce(atoms[i], atoms[j], *bond, dist, dir);
            } else {
                // Non-bonded: LJ van der Waals
                f += ljForce(atoms[i], atoms[j], dist, dir);
            }
            // Coulomb always (for charged species)
            f += coulombForce(atoms[i], atoms[j], dist, dir);

            atoms[i].force += f;
            atoms[j].force -= f; // Newton's third law
        }

        // Kinetic energy
        float v2 = glm::dot(atoms[i].vel, atoms[i].vel);
        atoms[i].kineticEnergy = 0.5f * atoms[i].mass * v2;
        totalKE += atoms[i].kineticEnergy;
    }

    // VSEPR angle forces
    applyAngleForces(atoms);
}

// ═══════════════════════════════════════════════════════════
//  Emergent bond energy estimation
// ═══════════════════════════════════════════════════════════
float InteractionEngine::estimateBondEnergy(const Atom& a, const Atom& b,
                                             Bond::Type type, int order) const {
    if (type == Bond::IONIC) {
        // Born-Haber: lattice energy approximation
        float dist = glm::length(a.pos - b.pos);
        return std::abs(coulK / std::max(dist, 1.0f));
    }
    // Covalent: geometric mean of ionization energies scaled by order
    float IE_A = a.element->ionizationEnergy;
    float IE_B = b.element->ionizationEnergy;
    float EA_A = std::max(a.element->electronAffinity, 0.1f);
    float EA_B = std::max(b.element->electronAffinity, 0.1f);
    float baseBondE = std::sqrt(EA_A * EA_B) + std::sqrt(IE_A * IE_B) * 0.1f;
    return baseBondE * order;
}

// ═══════════════════════════════════════════════════════════
//  Ionic bonding — Born-Haber cycle energy check
// ═══════════════════════════════════════════════════════════
bool InteractionEngine::tryIonicBond(Atom& a, Atom& b, int idxA, int idxB) {
    // Determine donor (low χ) and acceptor (high χ)
    Atom* donor   = (a.element->electronegativity < b.element->electronegativity) ? &a : &b;
    Atom* acceptor= (donor == &a) ? &b : &a;
    int donorIdx  = (donor == &a) ? idxA : idxB;
    int accIdx    = (donor == &a) ? idxB : idxA;

    if (!donor->wantsToLoseElectron()) return false;
    if (!acceptor->wantsElectron())    return false;

    // Born-Haber cycle energy check:
    // ΔE = IE(donor) - EA(acceptor) - Coulomb_stabilization
    float dist = glm::length(a.pos - b.pos);
    float coulombStab = coulK / std::max(dist, 0.5f);
    float deltaE = donor->element->ionizationEnergy -
                   acceptor->element->electronAffinity - coulombStab;

    if (deltaE > 0) return false; // endothermic — no bond

    // Thermal stability check: bond must survive thermal fluctuations
    float thermalE = kB * temperature;
    if (std::abs(deltaE) < thermalE * 2.0f) return false;

    // Transfer electron!
    Electron e = donor->removeOuterElectron();
    acceptor->addElectron(e);

    float bondE = std::abs(deltaE);
    float eqDist = (donor->element->covalentRadius + acceptor->element->covalentRadius) / 100.0f;
    float alpha = std::sqrt(5.0f / (2.0f * std::max(bondE, 0.1f)));

    Bond bondA; bondA.otherAtomIdx = accIdx; bondA.type = Bond::IONIC;
    bondA.order = 1; bondA.strength = bondE;
    bondA.equilibriumDist = eqDist; bondA.morseAlpha = alpha;

    Bond bondB; bondB.otherAtomIdx = donorIdx; bondB.type = Bond::IONIC;
    bondB.order = 1; bondB.strength = bondE;
    bondB.equilibriumDist = eqDist; bondB.morseAlpha = alpha;

    donor->bonds.push_back(bondA);
    acceptor->bonds.push_back(bondB);
    totalBondE += bondE;
    bondFormedCount++;

    // Log reaction
    std::ostringstream oss;
    oss << donor->element->symbol << " + " << acceptor->element->symbol
        << " -> ionic bond (dE=" << -deltaE << "eV)";
    reactionLog.push_back({simTime, oss.str()});

    return true;
}

// ═══════════════════════════════════════════════════════════
//  Covalent bonding — orbital overlap energy check
// ═══════════════════════════════════════════════════════════
bool InteractionEngine::tryCovalentBond(Atom& a, Atom& b, int idxA, int idxB) {
    int availA = a.availableValenceElectrons();
    int availB = b.availableValenceElectrons();
    if (availA <= 0 || availB <= 0) return false;

    // Bond order = min available, capped at 3
    int order = std::min({availA, availB, 3});

    float dist = glm::length(a.pos - b.pos);
    float eqDist = (a.element->covalentRadius + b.element->covalentRadius) / 100.0f;

    // Overlap factor: bonds more favorable near equilibrium distance
    float overlapSigma = eqDist * 0.5f;
    float overlapFactor = std::exp(-(dist - eqDist) * (dist - eqDist) /
                                    (overlapSigma * overlapSigma));

    float bondE = estimateBondEnergy(a, b, Bond::COVALENT, order) * overlapFactor;

    // Must be energetically favorable and thermally stable
    float thermalE = kB * temperature;
    if (bondE < thermalE * 3.0f) return false;

    float alpha = std::sqrt(5.0f / (2.0f * std::max(bondE, 0.1f)));

    Bond bondA; bondA.otherAtomIdx = idxB; bondA.type = Bond::COVALENT;
    bondA.order = order; bondA.strength = bondE;
    bondA.equilibriumDist = eqDist; bondA.morseAlpha = alpha;

    Bond bondB; bondB.otherAtomIdx = idxA; bondB.type = Bond::COVALENT;
    bondB.order = order; bondB.strength = bondE;
    bondB.equilibriumDist = eqDist; bondB.morseAlpha = alpha;

    a.bonds.push_back(bondA);
    b.bonds.push_back(bondB);
    a.updateEffectiveValence();
    b.updateEffectiveValence();
    totalBondE += bondE;
    bondFormedCount++;

    // Log
    std::ostringstream oss;
    std::string orderStr = (order == 1) ? "single" : (order == 2) ? "double" : "triple";
    oss << a.element->symbol << " + " << b.element->symbol
        << " -> " << orderStr << " covalent bond (E=" << bondE << "eV)";
    reactionLog.push_back({simTime, oss.str()});

    return true;
}

// ═══════════════════════════════════════════════════════════
//  Bond breaking — energy-based + thermal
// ═══════════════════════════════════════════════════════════
bool InteractionEngine::shouldBreakBond(const Atom& a, const Atom& b,
                                         const Bond& bond, float dist) const {
    // Morse potential energy at current distance
    float De = bond.strength;
    float alpha = bond.morseAlpha;
    float re = bond.equilibriumDist;
    float expt = std::exp(-alpha * (dist - re));
    float morseE = De * (1.0f - expt) * (1.0f - expt);

    // Break if stretched past 90% of dissociation energy
    if (morseE > De * 0.9f) return true;

    // Probabilistic thermal breaking: Boltzmann factor
    float thermalE = kB * temperature;
    float probability = std::exp(-De / (thermalE * 3.0f));
    // Use a simple threshold (deterministic for consistency)
    if (probability > 0.5f) return true;  // kT >> bond energy

    // Distance-based: break if beyond 2x equilibrium
    if (dist > re * 2.5f) return true;

    return false;
}

// ═══════════════════════════════════════════════════════════
//  Bond update loop
// ═══════════════════════════════════════════════════════════
void InteractionEngine::updateBonds(std::vector<Atom>& atoms) {
    int n = static_cast<int>(atoms.size());

    // ── Phase 1: Break bonds ──
    for (int i = 0; i < n; ++i) {
        auto& bondList = atoms[i].bonds;
        for (auto it = bondList.begin(); it != bondList.end(); ) {
            int j = it->otherAtomIdx;
            if (j < 0 || j >= n) { it = bondList.erase(it); continue; }
            float dist = glm::length(atoms[i].pos - atoms[j].pos);

            if (shouldBreakBond(atoms[i], atoms[j], *it, dist)) {
                // Remove from partner
                auto& otherBonds = atoms[j].bonds;
                otherBonds.erase(
                    std::remove_if(otherBonds.begin(), otherBonds.end(),
                        [i](const Bond& b){ return b.otherAtomIdx == i; }),
                    otherBonds.end());

                // If ionic, return electron
                if (it->type == Bond::IONIC && atoms[i].charge > 0) {
                    if (!atoms[j].electrons.empty()) {
                        Electron e = atoms[j].removeOuterElectron();
                        atoms[i].addElectron(e);
                    }
                }

                // Log
                std::ostringstream oss;
                oss << atoms[i].element->symbol << "-" << atoms[j].element->symbol
                    << " bond broken (T=" << temperature << "K)";
                reactionLog.push_back({simTime, oss.str()});

                totalBondE -= it->strength;
                bondBrokenCount++;
                it = bondList.erase(it);
            } else {
                ++it;
            }
        }
    }

    // ── Phase 2: Form new bonds ──
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            float dist = glm::length(atoms[i].pos - atoms[j].pos);
            if (dist > bondingRange) continue;

            // Skip if already bonded
            bool alreadyBonded = false;
            for (const auto& b : atoms[i].bonds)
                if (b.otherAtomIdx == j) { alreadyBonded = true; break; }
            if (alreadyBonded) continue;

            // Skip noble gases (octet complete, no bonding tendency)
            if (atoms[i].element->category == "noble_gas" ||
                atoms[j].element->category == "noble_gas") continue;

            float chiA = atoms[i].element->electronegativity;
            float chiB = atoms[j].element->electronegativity;
            if (chiA < 0.01f || chiB < 0.01f) continue;

            float deltaChi = std::abs(chiA - chiB);

            // Electronegativity difference determines bond type
            if (deltaChi > ionicThreshold) {
                tryIonicBond(atoms[i], atoms[j], i, j);
            } else {
                tryCovalentBond(atoms[i], atoms[j], i, j);
            }
        }
    }

    // Update effective valences
    for (auto& a : atoms) a.updateEffectiveValence();
}

} // namespace physics
