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
    float       atomicMass       = 0;
    float       electronegativity= 0;
    float       ionizationEnergy = 0;   // eV
    float       electronAffinity = 0;   // eV
    float       atomicRadius     = 0;   // pm
    float       covalentRadius   = 0;   // pm
    float       vdwRadius        = 0;   // pm
    int         valenceElectrons = 0;
    int         period = 0, group = 0;
    std::string category;
    std::vector<int> electronConfig;     // electrons per shell [2,8,1]
    std::vector<int> oxidationStates;
    glm::vec3   color = glm::vec3(1.0f);
};

class PeriodicTable {
public:
    static PeriodicTable& instance();
    bool loadFromFile(const std::string& path);
    const ElementData& get(int atomicNumber) const;
    int count() const { return static_cast<int>(elements_.size()); }
private:
    PeriodicTable() = default;
    std::unordered_map<int, ElementData> elements_;
    static ElementData dummy_;
};

} // namespace physics
