#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace physics {

struct ElementData {
    int         atomicNumber = 0;
    std::string symbol;
    std::string name;
    float       atomicMass       = 0;   // amu
    float       electronegativity= 0;   // Pauling scale
    float       ionizationEnergy = 0;   // eV (1st)
    float       secondIonization = 0;   // eV (2nd)
    float       electronAffinity = 0;   // eV
    float       atomicRadius     = 0;   // pm
    float       covalentRadius   = 0;   // pm
    float       vdwRadius        = 0;   // pm
    float       metallicRadius   = 0;   // pm
    int         valenceElectrons = 0;
    int         period = 0, group = 0;
    std::string category;
    std::string phase;                  // "solid","liquid","gas" at STP
    float       meltingPoint     = 0;   // K
    float       boilingPoint     = 0;   // K
    float       density          = 0;   // g/cm³
    std::vector<int> electronConfig;    // electrons per shell [2,8,1]
    std::vector<int> oxidationStates;
    glm::vec3   color = glm::vec3(1.0f);
};

class PeriodicTable {
public:
    static PeriodicTable& instance();
    bool loadFromFile(const std::string& path);
    const ElementData& get(int atomicNumber) const;
    bool has(int atomicNumber) const;
    int count() const { return static_cast<int>(elements_.size()); }
    const std::unordered_map<int, ElementData>& all() const { return elements_; }
private:
    PeriodicTable() = default;
    std::unordered_map<int, ElementData> elements_;
    static ElementData dummy_;
};

} // namespace physics
