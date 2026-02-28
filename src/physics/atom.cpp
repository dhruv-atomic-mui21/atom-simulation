#include "atom.h"
#include <algorithm>

namespace physics {

void Atom::init(int atomicNumber) {
    elementZ = atomicNumber;
    element = &PeriodicTable::instance().get(atomicNumber);
    electrons = fillElectronShells(atomicNumber);
    charge = 0;
    visualRadius = element->atomicRadius / 100.0f; // pm → Å scale
    if (visualRadius < 0.5f) visualRadius = 0.5f;
}

int Atom::availableValenceElectrons() const {
    if (!element) return 0;
    // Count unbonded valence electrons
    int valence = element->valenceElectrons;
    int bonded = 0;
    for (const auto& b : bonds) bonded += b.order;
    return std::max(0, valence - bonded);
}

bool Atom::wantsElectron() const {
    if (!element) return false;
    return element->electronAffinity > 0.5f &&
           element->valenceElectrons < 8;
}

bool Atom::wantsToLoseElectron() const {
    if (!element) return false;
    return element->ionizationEnergy < 8.0f &&
           element->valenceElectrons <= 2;
}

Electron Atom::removeOuterElectron() {
    // Remove the last electron (outermost shell)
    Electron e = electrons.back();
    electrons.pop_back();
    charge += 1;
    return e;
}

void Atom::addElectron(const Electron& e) {
    electrons.push_back(e);
    charge -= 1;
}

} // namespace physics
