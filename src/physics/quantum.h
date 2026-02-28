#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <random>

namespace physics {

/// Wavefunction sampling engine — generalized from ref-repo to support
/// any element via effective nuclear charge (Slater's rules).
class QuantumSampler {
public:
    QuantumSampler();

    /// Compute Slater effective nuclear charge for a given electron.
    /// Z = atomic number, n/l = electron's quantum numbers,
    /// allElectronNL = list of (n,l) for all electrons in the atom.
    static float computeZeff(int Z, int targetN, int targetL,
                             const std::vector<std::pair<int,int>>& allElectronNL);

    /// Sample a 3D position from |ψ(n,l,m)|² with effective Z.
    glm::vec3 samplePosition(int n, int l, int m, float zEff);

    /// Compute |ψ|² at a given point (for coloring).
    static float probabilityDensity(int n, int l, int m, float zEff,
                                    float r, float theta, float phi);

    /// Heatmap: probability → RGBA color (from ref-repo).
    static glm::vec4 heatmapColor(float intensity);

private:
    std::mt19937 gen_;

    // CDF sampling (adapted from ref-repo atom_realtime.cpp)
    float sampleR(int n, int l, float zEff);
    float sampleTheta(int l, int m);
    float samplePhi();

    // Radial wavefunction R(n,l,r; Z_eff)
    static double radialR(int n, int l, float zEff, double r);
    // Associated Legendre polynomial P_l^m(cos θ)
    static double assocLegendre(int l, int m, double x);
};

} // namespace physics
