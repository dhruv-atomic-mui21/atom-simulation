#include "electron.h"
#include <algorithm>

namespace physics {

// Aufbau filling order: (n+l, n) pairs sorted by n+l then n.
static const int fillOrder[][2] = {
    {1,0},{2,0},{2,1},{3,0},{3,1},{4,0},{3,2},{4,1},{5,0},{4,2},
    {5,1},{6,0},{4,3},{5,2},{6,1},{7,0},{5,3},{6,2},{7,1},{6,3}
};
static const int fillOrderSize = sizeof(fillOrder)/sizeof(fillOrder[0]);

// Max electrons in a subshell = 2*(2l+1).
static int maxInSubshell(int l) { return 2 * (2 * l + 1); }

std::vector<Electron> fillElectronShells(int atomicNumber) {
    std::vector<Electron> electrons;
    int remaining = atomicNumber;

    for (int i = 0; i < fillOrderSize && remaining > 0; ++i) {
        int n = fillOrder[i][0];
        int l = fillOrder[i][1];
        int capacity = maxInSubshell(l);
        int toFill = std::min(remaining, capacity);

        // Fill m values from -l to +l, each with spin +1/2 then -1/2
        for (int ml = -l; ml <= l && toFill > 0; ++ml) {
            for (int spin : {1, -1}) {
                if (toFill <= 0) break;
                Electron e;
                e.qn = {n, l, ml, spin};
                electrons.push_back(e);
                --toFill;
                --remaining;
            }
        }
    }
    return electrons;
}

int countValenceElectrons(const std::vector<Electron>& electrons) {
    if (electrons.empty()) return 0;
    int maxN = outermostShell(electrons);
    int count = 0;
    for (const auto& e : electrons)
        if (e.qn.n == maxN) ++count;
    return count;
}

int outermostShell(const std::vector<Electron>& electrons) {
    int maxN = 0;
    for (const auto& e : electrons)
        if (e.qn.n > maxN) maxN = e.qn.n;
    return maxN;
}

} // namespace physics
