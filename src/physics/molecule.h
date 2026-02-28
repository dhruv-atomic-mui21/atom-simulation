#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <unordered_map>

namespace physics {

/// A molecule is a connected cluster of bonded atoms.
struct Molecule {
    int id = -1;
    std::vector<int> atomIndices;
    float totalBondEnergy = 0;    // sum of bond strengths (eV)
    std::string formula;          // e.g. "H2O"
    glm::vec3 centerOfMass = glm::vec3(0);
    float totalMass = 0;
};

/// Manages molecule detection via bond-graph BFS.
class MoleculeTracker {
public:
    /// Rebuild molecule list from current atom bond graph.
    void update(const struct Atom* atoms, int atomCount);

    const std::vector<Molecule>& molecules() const { return molecules_; }
    int count() const { return static_cast<int>(molecules_.size()); }

private:
    std::vector<Molecule> molecules_;

    /// Generate chemical formula from atom indices.
    static std::string computeFormula(const struct Atom* atoms,
                                       const std::vector<int>& indices);
};

} // namespace physics
