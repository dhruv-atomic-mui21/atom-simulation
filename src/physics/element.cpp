#include "element.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace physics {

ElementData PeriodicTable::dummy_;

PeriodicTable& PeriodicTable::instance() {
    static PeriodicTable pt;
    return pt;
}

bool PeriodicTable::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Failed to open elements file: " << path << "\n";
        return false;
    }
    nlohmann::json j;
    f >> j;

    // Helper: safely read a numeric JSON field that might be null
    auto safeFloat = [](const nlohmann::json& v, const char* key, float def) -> float {
        if (v.contains(key) && !v[key].is_null()) return v[key].get<float>();
        return def;
    };
    auto safeInt = [](const nlohmann::json& v, const char* key, int def) -> int {
        if (v.contains(key) && !v[key].is_null()) return v[key].get<int>();
        return def;
    };
    auto safeStr = [](const nlohmann::json& v, const char* key, const std::string& def) -> std::string {
        if (v.contains(key) && !v[key].is_null()) return v[key].get<std::string>();
        return def;
    };

    for (auto& [key, val] : j.items()) {
        ElementData e;
        e.atomicNumber     = safeInt(val, "atomic_number", 0);
        e.symbol           = safeStr(val, "symbol", "?");
        e.name             = safeStr(val, "name", "Unknown");
        e.atomicMass       = safeFloat(val, "atomic_mass", 1.0f);
        e.electronegativity= safeFloat(val, "electronegativity", 0.0f);
        e.ionizationEnergy = safeFloat(val, "ionization_energy_eV", 0.0f);
        e.electronAffinity = safeFloat(val, "electron_affinity_eV", 0.0f);
        e.atomicRadius     = safeFloat(val, "atomic_radius_pm", 100.0f);
        e.covalentRadius   = safeFloat(val, "covalent_radius_pm", 100.0f);
        e.vdwRadius        = safeFloat(val, "vdw_radius_pm", 150.0f);
        e.valenceElectrons = safeInt(val, "valence_electrons", 0);
        e.period           = safeInt(val, "period", 0);
        e.group            = safeInt(val, "group", 0);
        e.category         = safeStr(val, "category", "unknown");

        if (val.contains("electron_config"))
            e.electronConfig = val["electron_config"].get<std::vector<int>>();
        if (val.contains("oxidation_states"))
            e.oxidationStates = val["oxidation_states"].get<std::vector<int>>();
        if (val.contains("color_rgb")) {
            auto& c = val["color_rgb"];
            e.color = glm::vec3(c[0].get<float>()/255.f,
                                c[1].get<float>()/255.f,
                                c[2].get<float>()/255.f);
        }
        elements_[e.atomicNumber] = e;
    }
    std::cout << "Loaded " << elements_.size() << " elements\n";
    return true;
}

const ElementData& PeriodicTable::get(int z) const {
    auto it = elements_.find(z);
    return (it != elements_.end()) ? it->second : dummy_;
}

} // namespace physics
