#include <cstdlib>
#include <iostream>
#include <vector>

#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>

#include "dkg.hpp"
#include "field.hpp"

namespace {

long to_long(const FieldElement& value) {
    return NTL::to_long(NTL::rep(value));
}

std::vector<FieldElement> make_evaluation_points(std::size_t n) {
    std::vector<FieldElement> points;
    points.reserve(n);
    for (std::size_t i = 1; i <= n; ++i) {
        FieldElement point;
        NTL::conv(point, static_cast<long>(i));
        points.push_back(point);
    }
    return points;
}

} // namespace

int main(int argc, char* argv[]) {
    std::size_t n = 10;
    std::size_t threshold = 3;
    long p = 7919;

    if (argc >= 2)
        n = static_cast<std::size_t>(std::strtoul(argv[1], nullptr, 10));
    if (argc >= 3)
        threshold = static_cast<std::size_t>(std::strtoul(argv[2], nullptr, 10));
    if (argc >= 4)
        p = std::strtol(argv[3], nullptr, 10);

    if (threshold >= n || 3 * threshold >= n) {
        std::cerr << "Error: the DKG demo requires n > 3t.\n";
        return 1;
    }
    if (p <= static_cast<long>(n)) {
        std::cerr << "Error: p must be greater than n.\n";
        return 1;
    }

    init_field(NTL::ZZ(p));
    const std::vector<FieldElement> points = make_evaluation_points(n);

    std::cout << "Honest DKG simulation over F_" << p << "\n";
    std::cout << "Participants n = " << n << ", threshold t = " << threshold << "\n";
    std::cout << "Each participant samples a field element and runs Deal/GetShare as a dealer.\n";
    std::cout << "This single-process demo displays secrets for inspection; a real DKG would not reveal them.\n\n";

    const DKGResult result = run_dkg(n, threshold, points);

    for (std::size_t dealer = 0; dealer < n; ++dealer) {
        std::cout << "Dealer P_" << dealer + 1
                  << " sampled a_" << dealer + 1 << " = "
                  << to_long(result.dealer_secrets[dealer]) << "\n";
        std::cout << "  Shares phi_" << dealer + 1 << "(0, x_j): [";
        for (std::size_t participant = 0; participant < n; ++participant) {
            if (participant != 0)
                std::cout << ", ";
            std::cout << to_long(result.dealer_shares[dealer][participant]);
        }
        std::cout << "]\n";
    }

    std::cout << "\nAggregated share held by each participant:\n";
    for (std::size_t participant = 0; participant < n; ++participant) {
        std::cout << "  P_" << participant + 1 << ": y_" << participant + 1
                  << " = " << to_long(result.participant_shares[participant]) << "\n";
    }

    const FieldElement sampled_sum = result.aggregate_secret;
    std::cout << "\nSum of dealer contributions (simulation check): "
              << to_long(sampled_sum) << "\n";
    std::cout << "Interpolation from t+1 aggregated shares: "
              << (result.reconstruction_matches_sum ? "Valid" : "Invalid") << "\n";
    std::cout << (result.reconstruction_matches_sum
        ? "Honest DKG simulation completed successfully.\n"
        : "Error: reconstructed value differs from the sum of contributions.\n");

    return result.reconstruction_matches_sum ? 0 : 1;
}
