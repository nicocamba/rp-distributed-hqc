#include "dkg.hpp"

#include <NTL/ZZ_pX.h>
#include <stdexcept>
#include <utility>

#include "deal.hpp"
#include "getshare.hpp"

namespace {

std::vector<GetShareResult> run_getshare_for_deal(
    const DealResult& deal,
    std::size_t n,
    std::size_t threshold,
    const std::vector<FieldElement>& x_coords
) {
    std::vector<ParticipantState> participants;
    participants.reserve(n);
    for (std::size_t j = 0; j < n; ++j)
        participants.push_back(init_participant(j, n, threshold, x_coords));

    std::vector<std::vector<EchoMessage>> echoes(n);
    for (std::size_t j = 0; j < n; ++j) {
        echoes[j] = receive_deal(
            participants[j], deal.broadcast_data, deal.private_data[j]);
        if (n > 1 && echoes[j].size() != n - 1)
            throw std::runtime_error("A participant rejected the Dealer's Deal.");
    }

    // Simula el intercambio de Echo entre todos los participantes.
    std::vector<bool> ready_to_send(n, false);
    for (std::size_t sender = 0; sender < n; ++sender) {
        std::size_t echo_index = 0;
        for (std::size_t receiver = 0; receiver < n; ++receiver) {
            if (receiver == sender)
                continue;
            if (receive_echo(participants[receiver], echoes[sender][echo_index]))
                ready_to_send[receiver] = true;
            ++echo_index;
        }
    }

    // Dos rondas bastan en esta simulación con todos los participantes honestos.
    for (int round = 0; round < 2; ++round) {
        std::vector<bool> next_ready(n, false);
        for (std::size_t sender = 0; sender < n; ++sender) {
            if (!ready_to_send[sender])
                continue;
            for (std::size_t receiver = 0; receiver < n; ++receiver) {
                if (receive_rdy(participants[receiver], sender))
                    next_ready[receiver] = true;
            }
        }
        ready_to_send = std::move(next_ready);
    }

    std::vector<GetShareResult> local_polynomials;
    local_polynomials.reserve(n);
    for (const ParticipantState& participant : participants) {
        auto local = reconstruct(participant);
        if (!local)
            throw std::runtime_error("A participant could not reconstruct its local share.");
        local_polynomials.push_back(std::move(*local));
    }
    return local_polynomials;
}

FieldElement interpolate_at_zero(
    const std::vector<FieldElement>& x_coords,
    const std::vector<FieldElement>& values,
    std::size_t count
) {
    FieldElement result;
    NTL::conv(result, 0L);

    for (std::size_t i = 0; i < count; ++i) {
        FieldElement numerator, denominator;
        NTL::conv(numerator, 1L);
        NTL::conv(denominator, 1L);
        for (std::size_t j = 0; j < count; ++j) {
            if (i == j)
                continue;
            numerator *= -x_coords[j];
            denominator *= x_coords[i] - x_coords[j];
        }
        result += values[i] * (numerator / denominator);
    }
    return result;
}

} // namespace

DKGResult run_dkg(
    std::size_t n,
    std::size_t threshold,
    const std::vector<FieldElement>& x_coords
) {
    if (x_coords.size() != n)
        throw std::invalid_argument("The number of evaluation points must equal n.");
    if (n <= 3 * threshold)
        throw std::invalid_argument("The AVSS demo requires n > 3t.");

    DKGResult result;
    result.dealer_secrets.resize(n);
    result.dealer_shares.assign(n, std::vector<FieldElement>(n));
    result.participant_shares.assign(n, FieldElement());
    NTL::conv(result.aggregate_secret, 0L);
    result.reconstruction_matches_sum = false;

    FieldElement zero;
    NTL::conv(zero, 0L);

    // Cada participante actúa como dealer en su propia instancia AVSS.
    for (std::size_t dealer = 0; dealer < n; ++dealer) {
        const FieldElement contribution = random_element();
        result.dealer_secrets[dealer] = contribution;
        result.aggregate_secret += contribution;

        DealResult deal = run_deal(contribution, n, threshold, x_coords);
        const std::vector<GetShareResult> local_polynomials =
            run_getshare_for_deal(deal, n, threshold, x_coords);

        for (std::size_t participant = 0; participant < n; ++participant) {
            FieldElement share;
            NTL::eval(share, local_polynomials[participant].phi_share, zero);
            result.dealer_shares[dealer][participant] = share;
            result.participant_shares[participant] += share;
        }
    }

    const FieldElement reconstructed = interpolate_at_zero(
        x_coords, result.participant_shares, threshold + 1);
    result.reconstruction_matches_sum = (reconstructed == result.aggregate_secret);
    return result;
}
