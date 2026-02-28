#include "molecule.h"
#include "atom.h"
#include <queue>
#include <algorithm>
#include <map>

namespace physics {

void MoleculeTracker::update(const Atom* atoms, int atomCount) {
    molecules_.clear();
    if (atomCount == 0) return;

    std::vector<bool> visited(atomCount, false);
    int molId = 0;

    for (int i = 0; i < atomCount; ++i) {
        if (visited[i]) continue;

        // BFS to find connected component
        Molecule mol;
        mol.id = molId;
        std::queue<int> q;
        q.push(i);
        visited[i] = true;

        while (!q.empty()) {
            int cur = q.front(); q.pop();
            mol.atomIndices.push_back(cur);

            for (const auto& bond : atoms[cur].bonds) {
                int other = bond.otherAtomIdx;
                if (other >= 0 && other < atomCount && !visited[other]) {
                    visited[other] = true;
                    q.push(other);
                }
            }
        }

        // Compute properties
        mol.totalMass = 0;
        mol.centerOfMass = glm::vec3(0);
        mol.totalBondEnergy = 0;

        for (int idx : mol.atomIndices) {
            mol.totalMass += atoms[idx].mass;
            mol.centerOfMass += atoms[idx].mass * atoms[idx].pos;
        }
        if (mol.totalMass > 0)
            mol.centerOfMass /= mol.totalMass;

        // Sum bond energies (only count each bond once)
        for (int idx : mol.atomIndices) {
            for (const auto& bond : atoms[idx].bonds) {
                if (bond.otherAtomIdx > idx)
                    mol.totalBondEnergy += bond.strength;
            }
        }

        mol.formula = computeFormula(atoms, mol.atomIndices);
        molecules_.push_back(mol);
        ++molId;
    }
}

std::string MoleculeTracker::computeFormula(const Atom* atoms,
                                             const std::vector<int>& indices) {
    if (indices.size() == 1)
        return atoms[indices[0]].element->symbol;

    // Count atoms by symbol, ordered by Hill system (C first, H second, then alphabetical)
    std::map<std::string, int> counts;
    for (int idx : indices)
        counts[atoms[idx].element->symbol]++;

    std::string formula;
    // Hill system: C first, H second
    auto append = [&](const std::string& sym) {
        auto it = counts.find(sym);
        if (it != counts.end()) {
            formula += sym;
            if (it->second > 1) formula += std::to_string(it->second);
            counts.erase(it);
        }
    };
    append("C");
    append("H");
    // Then alphabetical
    for (const auto& [sym, count] : counts) {
        formula += sym;
        if (count > 1) formula += std::to_string(count);
    }
    return formula;
}

} // namespace physics
