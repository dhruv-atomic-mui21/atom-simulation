#include "quantum.h"
#include <cmath>
#include <algorithm>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace physics {

QuantumSampler::QuantumSampler() : gen_(std::random_device{}()) {}

// ═══════════════════════════════════════════════════════════
//  Slater's Rules for effective nuclear charge
// ═══════════════════════════════════════════════════════════
float QuantumSampler::computeZeff(int Z, int targetN, int targetL,
                                   const std::vector<std::pair<int,int>>& allNL) {
    // Slater grouping: (1s)(2s2p)(3s3p)(3d)(4s4p)(4d)(4f)(5s5p)...
    // Simplified: group by n, except d and f are separate groups.
    auto slaterGroup = [](int n, int l) -> int {
        if (l >= 2) return n * 10 + l; // d,f get own group
        return n * 10;                  // s,p share a group
    };

    int targetGroup = slaterGroup(targetN, targetL);
    double sigma = 0.0;

    for (const auto& [en, el] : allNL) {
        int grp = slaterGroup(en, el);
        if (grp == targetGroup) {
            // Same group: each other electron contributes 0.35 (or 0.30 for 1s)
            sigma += (targetN == 1) ? 0.30 : 0.35;
        } else if (grp < targetGroup) {
            if (targetL >= 2) {
                // For d/f electrons: all lower electrons contribute 1.00
                sigma += 1.00;
            } else {
                // For s/p: (n-1) shell contributes 0.85, deeper shells 1.00
                int eGroup = slaterGroup(en, el);
                int targetSP = targetN * 10;
                int prevSP   = (targetN - 1) * 10;
                if (eGroup >= prevSP && eGroup < targetSP) {
                    sigma += 0.85;
                } else {
                    sigma += 1.00;
                }
            }
        }
        // Electrons in higher groups don't shield
    }
    // We counted the target electron itself; subtract its contribution
    sigma -= (targetN == 1) ? 0.30 : 0.35;

    float zeff = static_cast<float>(Z) - static_cast<float>(sigma);
    return std::max(zeff, 1.0f);
}

// ═══════════════════════════════════════════════════════════
//  Radial wavefunction R(n,l,r; Z_eff) — from ref-repo
// ═══════════════════════════════════════════════════════════
double QuantumSampler::radialR(int n, int l, float zEff, double r) {
    double a0 = 1.0 / zEff; // scaled Bohr radius
    double rho = 2.0 * r / (n * a0);

    // Associated Laguerre L_{n-l-1}^{2l+1}(rho)
    int k = n - l - 1;
    int alpha = 2 * l + 1;
    double L = 1.0;
    if (k == 1) {
        L = 1.0 + alpha - rho;
    } else if (k > 1) {
        double Lm2 = 1.0;
        double Lm1 = 1.0 + alpha - rho;
        for (int j = 2; j <= k; ++j) {
            L = ((2*j - 1 + alpha - rho) * Lm1 - (j - 1 + alpha) * Lm2) / j;
            Lm2 = Lm1;
            Lm1 = L;
        }
    }

    double norm = std::pow(2.0 / (n * a0), 3) *
                  std::tgamma(n - l) / (2.0 * n * std::tgamma(n + l + 1));
    return std::sqrt(norm) * std::exp(-rho / 2.0) * std::pow(rho, l) * L;
}

// ═══════════════════════════════════════════════════════════
//  Associated Legendre P_l^m(x) — from ref-repo
// ═══════════════════════════════════════════════════════════
double QuantumSampler::assocLegendre(int l, int m, double x) {
    int am = std::abs(m);
    double Pmm = 1.0;
    if (am > 0) {
        double somx2 = std::sqrt((1.0 - x) * (1.0 + x));
        double fact = 1.0;
        for (int j = 1; j <= am; ++j) {
            Pmm *= -fact * somx2;
            fact += 2.0;
        }
    }
    if (l == am) return Pmm;
    double Pm1m = x * (2 * am + 1) * Pmm;
    if (l == am + 1) return Pm1m;
    double Pll = 0;
    for (int ll = am + 2; ll <= l; ++ll) {
        Pll = ((2*ll - 1) * x * Pm1m - (ll + am - 1) * Pmm) / (ll - am);
        Pmm = Pm1m;
        Pm1m = Pll;
    }
    return Pm1m;
}

// ═══════════════════════════════════════════════════════════
//  CDF Sampling (adapted from ref-repo sampleR / sampleTheta)
// ═══════════════════════════════════════════════════════════

float QuantumSampler::sampleR(int n, int l, float zEff) {
    const int N = 4096;
    double a0 = 1.0 / zEff;
    double rMax = 10.0 * n * n * a0;

    // Build CDF on the fly (could cache per (n,l,zEff) later)
    std::vector<double> cdf(N);
    double dr = rMax / (N - 1);
    double sum = 0.0;
    for (int i = 0; i < N; ++i) {
        double r = i * dr;
        double R = radialR(n, l, zEff, r);
        double pdf = r * r * R * R;
        sum += pdf;
        cdf[i] = sum;
    }
    for (auto& v : cdf) v /= sum;

    std::uniform_real_distribution<double> dis(0.0, 1.0);
    double u = dis(gen_);
    int idx = static_cast<int>(
        std::lower_bound(cdf.begin(), cdf.end(), u) - cdf.begin());
    return static_cast<float>(idx * dr);
}

float QuantumSampler::sampleTheta(int l, int m) {
    const int N = 2048;
    int am = std::abs(m);

    std::vector<double> cdf(N);
    double dtheta = M_PI / (N - 1);
    double sum = 0.0;
    for (int i = 0; i < N; ++i) {
        double theta = i * dtheta;
        double Plm = assocLegendre(l, am, std::cos(theta));
        double pdf = std::sin(theta) * Plm * Plm;
        sum += pdf;
        cdf[i] = sum;
    }
    for (auto& v : cdf) v /= sum;

    std::uniform_real_distribution<double> dis(0.0, 1.0);
    double u = dis(gen_);
    int idx = static_cast<int>(
        std::lower_bound(cdf.begin(), cdf.end(), u) - cdf.begin());
    return static_cast<float>(idx * dtheta);
}

float QuantumSampler::samplePhi() {
    std::uniform_real_distribution<float> dis(0.0f, 2.0f * static_cast<float>(M_PI));
    return dis(gen_);
}

// ═══════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════

glm::vec3 QuantumSampler::samplePosition(int n, int l, int m, float zEff) {
    float r     = sampleR(n, l, zEff);
    float theta = sampleTheta(l, std::abs(m));
    float phi   = samplePhi();

    return glm::vec3(
        r * std::sin(theta) * std::cos(phi),
        r * std::cos(theta),
        r * std::sin(theta) * std::sin(phi)
    );
}

float QuantumSampler::probabilityDensity(int n, int l, int m, float zEff,
                                          float r, float theta, float phi) {
    double R = radialR(n, l, zEff, static_cast<double>(r));
    double Plm = assocLegendre(l, std::abs(m), std::cos(theta));
    double intensity = (R * R) * (Plm * Plm);
    return static_cast<float>(intensity);
}

glm::vec4 QuantumSampler::heatmapColor(float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    // Fire heatmap from ref-repo: black→purple→red→orange→yellow→white
    const int N = 6;
    glm::vec4 stops[N] = {
        {0.0f, 0.0f, 0.0f, 1.0f},   // black
        {0.5f, 0.0f, 0.99f, 1.0f},  // purple
        {0.8f, 0.0f, 0.0f, 1.0f},   // red
        {1.0f, 0.5f, 0.0f, 1.0f},   // orange
        {1.0f, 1.0f, 0.0f, 1.0f},   // yellow
        {1.0f, 1.0f, 1.0f, 1.0f},   // white
    };
    float s = value * (N - 1);
    int i = static_cast<int>(s);
    int j = std::min(i + 1, N - 1);
    float t = s - i;
    return glm::mix(stops[i], stops[j], t);
}

} // namespace physics
