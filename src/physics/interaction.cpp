#include "interaction.h"
#include <cmath>
#include <algorithm>

namespace physics {

// Boltzmann constant in eV/K
static constexpr float kB = 8.617e-5f;

// ═══════════════════════════════════════════════════════════
//  Force calculations
// ═══════════════════════════════════════════════════════════

glm::vec3 InteractionEngine::ljForce(const Atom& a, const Atom& b,
                                      float dist, glm::vec3 dir) const {
    float sigma = (a.element->vdwRadius + b.element->vdwRadius) / 200.0f; // pm→Å
    if (dist < 0.5f) dist = 0.5f; // avoid singularity
    float sr6 = std::pow(sigma / dist, 6.0f);
    float magnitude = 24.0f * ljEpsilon * (2.0f * sr6 * sr6 - sr6) / dist;
    return magnitude * dir;
}

glm::vec3 InteractionEngine::coulombForce(const Atom& a, const Atom& b,
                                           float dist, glm::vec3 dir) const {
    if (a.charge == 0 && b.charge == 0) return glm::vec3(0.0f);
    if (dist < 0.5f) dist = 0.5f;
    float magnitude = coulombK * a.charge * b.charge / (dist * dist);
    return magnitude * dir; // positive = repulsion for same-sign charges
}

glm::vec3 InteractionEngine::bondSpringForce(const Atom& a, const Atom& b,
                                              float eqDist, float dist,
                                              glm::vec3 dir) const {
    float k = 5.0f; // spring constant (eV/Å²)
    float displacement = dist - eqDist;
    return -k * displacement * dir;
}

// ═══════════════════════════════════════════════════════════
//  Main force computation
// ═══════════════════════════════════════════════════════════

void InteractionEngine::computeForces(std::vector<Atom>& atoms) {
    // Reset forces
    for (auto& a : atoms) a.force = glm::vec3(0.0f);

    int n = static_cast<int>(atoms.size());
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            glm::vec3 diff = atoms[i].pos - atoms[j].pos;
            float dist = glm::length(diff);
            if (dist < 0.01f) continue;
            glm::vec3 dir = diff / dist;

            // Check if bonded
            bool bonded = false;
            float eqDist = 0;
            for (const auto& bond : atoms[i].bonds) {
                if (bond.otherAtomIdx == j) {
                    bonded = true;
                    eqDist = (atoms[i].element->covalentRadius +
                              atoms[j].element->covalentRadius) / 100.0f;
                    break;
                }
            }

            glm::vec3 f(0.0f);
            if (bonded) {
                f += bondSpringForce(atoms[i], atoms[j], eqDist, dist, dir);
            }
            f += coulombForce(atoms[i], atoms[j], dist, dir);
            f += ljForce(atoms[i], atoms[j], dist, dir);

            atoms[i].force += f;
            atoms[j].force -= f; // Newton's third law
        }
    }
}

// ═══════════════════════════════════════════════════════════
//  Bond formation / breaking — EMERGENT from properties
// ═══════════════════════════════════════════════════════════

bool InteractionEngine::tryIonicBond(Atom& a, Atom& b, int idxA, int idxB) {
    // Determine donor (low χ) and acceptor (high χ)
    Atom* donor   = (a.element->electronegativity < b.element->electronegativity) ? &a : &b;
    Atom* acceptor= (donor == &a) ? &b : &a;
    int donorIdx  = (donor == &a) ? idxA : idxB;
    int accIdx    = (donor == &a) ? idxB : idxA;

    if (!donor->wantsToLoseElectron()) return false;
    if (!acceptor->wantsElectron())    return false;

    // Energy check: IE(donor) - EA(acceptor) - Coulomb stabilization
    float dist = glm::length(a.pos - b.pos);
    float coulombStab = coulombK / dist; // stabilization from ion pair
    float deltaE = donor->element->ionizationEnergy -
                   acceptor->element->electronAffinity - coulombStab;
    if (deltaE > 0) return false; // endothermic — no bond

    // Transfer electron!
    Electron e = donor->removeOuterElectron();
    acceptor->addElectron(e);

    // Record bond
    Bond bondA; bondA.otherAtomIdx = accIdx; bondA.type = Bond::IONIC;
    bondA.strength = -deltaE;
    Bond bondB; bondB.otherAtomIdx = donorIdx; bondB.type = Bond::IONIC;
    bondB.strength = -deltaE;
    donor->bonds.push_back(bondA);
    acceptor->bonds.push_back(bondB);

    return true;
}

bool InteractionEngine::tryCovalentBond(Atom& a, Atom& b, int idxA, int idxB) {
    int availA = a.availableValenceElectrons();
    int availB = b.availableValenceElectrons();
    if (availA <= 0 || availB <= 0) return false;

    // Bond order = min available, capped at 3
    int order = std::min({availA, availB, 3});
    float bondEnergy = 2.0f * static_cast<float>(order); // rough eV estimate

    Bond bondA; bondA.otherAtomIdx = idxB; bondA.type = Bond::COVALENT;
    bondA.order = order; bondA.strength = bondEnergy;
    Bond bondB; bondB.otherAtomIdx = idxA; bondB.type = Bond::COVALENT;
    bondB.order = order; bondB.strength = bondEnergy;
    a.bonds.push_back(bondA);
    b.bonds.push_back(bondB);

    return true;
}

bool InteractionEngine::shouldBreakBond(const Bond& bond) const {
    float thermalEnergy = kB * temperature;
    return thermalEnergy > bond.strength * 0.5f; // break if thermal > 50% bond
}

void InteractionEngine::updateBonds(std::vector<Atom>& atoms) {
    int n = static_cast<int>(atoms.size());

    // Break bonds that are too stretched or thermally excited
    for (int i = 0; i < n; ++i) {
        auto& bondList = atoms[i].bonds;
        for (auto it = bondList.begin(); it != bondList.end(); ) {
            int j = it->otherAtomIdx;
            float dist = glm::length(atoms[i].pos - atoms[j].pos);
            float maxDist = bondingRange * 1.5f;

            if (dist > maxDist || shouldBreakBond(*it)) {
                // Remove bond from both atoms
                auto& otherBonds = atoms[j].bonds;
                otherBonds.erase(
                    std::remove_if(otherBonds.begin(), otherBonds.end(),
                        [i](const Bond& b){ return b.otherAtomIdx == i; }),
                    otherBonds.end());

                // If ionic, return electron
                if (it->type == Bond::IONIC && atoms[i].charge > 0) {
                    Electron e = atoms[j].removeOuterElectron();
                    atoms[i].addElectron(e);
                }
                it = bondList.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Try to form new bonds
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            float dist = glm::length(atoms[i].pos - atoms[j].pos);
            if (dist > bondingRange) continue;

            // Skip if already bonded
            bool alreadyBonded = false;
            for (const auto& b : atoms[i].bonds)
                if (b.otherAtomIdx == j) { alreadyBonded = true; break; }
            if (alreadyBonded) continue;

            // Skip if noble gases
            if (atoms[i].element->category == "noble_gas" ||
                atoms[j].element->category == "noble_gas") continue;

            float chiA = atoms[i].element->electronegativity;
            float chiB = atoms[j].element->electronegativity;
            if (chiA < 0.01f || chiB < 0.01f) continue; // no data

            float deltaChi = std::abs(chiA - chiB);

            if (deltaChi > ionicThreshold) {
                tryIonicBond(atoms[i], atoms[j], i, j);
            } else {
                tryCovalentBond(atoms[i], atoms[j], i, j);
            }
        }
    }
}

} // namespace physics
