#include "atom.h"
#include <algorithm>

namespace physics {

void Atom::init(int atomicNumber) {
    elementZ = atomicNumber;
    element = &PeriodicTable::instance().get(atomicNumber);
    electrons = fillElectronShells(atomicNumber);
    charge = 0;
    mass = element->atomicMass;
    visualRadius = element->atomicRadius / 100.0f; // pm → Å scale
    if (visualRadius < 0.5f) visualRadius = 0.5f;
    updateEffectiveValence();
}

int Atom::totalBondOrder() const {
    int total = 0;
    for (const auto& b : bonds) total += b.order;
    return total;
}

int Atom::availableValenceElectrons() const {
    if (!element) return 0;
    int valence = element->valenceElectrons;
    int bonded = totalBondOrder();
    return std::max(0, valence - bonded);
}

void Atom::updateEffectiveValence() {
    effectiveValence = availableValenceElectrons();
}

bool Atom::wantsElectron() const {
    if (!element) return false;
    return element->electronAffinity > 0.3f &&
           element->valenceElectrons < 8;
}

bool Atom::wantsToLoseElectron() const {
    if (!element) return false;
    return element->ionizationEnergy < 8.0f &&
           element->valenceElectrons <= 2;
}

Electron Atom::removeOuterElectron() {
    Electron e = electrons.back();
    electrons.pop_back();
    charge += 1;
    updateEffectiveValence();
    return e;
}

void Atom::addElectron(const Electron& e) {
    electrons.push_back(e);
    charge -= 1;
    updateEffectiveValence();
}

} // namespace physics
