#include "avss/protocol/dkg/dkg.hpp"

#include <NTL/ZZ_pX.h>
#include <stdexcept>
#include <utility>

#include "avss/protocol/avss/deal.hpp"
#include "avss/protocol/avss/getshare.hpp"

namespace {

using DealerStates = std::vector<std::vector<ParticipantState>>;
using DealerEchoes = std::vector<std::vector<std::vector<EchoMessage>>>;
using DealerPolynomials = std::vector<std::vector<GetShareResult>>;

DealerPolynomials run_all_getshares(
    const std::vector<DealResult>& deals,
    std::size_t n,
    std::size_t threshold,
    const std::vector<FieldElement>& x_coords
) {
    // states[d][j] es el estado del participante P_j en el AVSS iniciado
    // por P_d. Hay n instancias AVSS, cada una con n estados de participante.
    DealerStates states(n);
    DealerEchoes echoes(n, std::vector<std::vector<EchoMessage>>(n));
    std::vector<std::vector<bool>> ready_to_send(
        n, std::vector<bool>(n, false));

    for (std::size_t dealer = 0; dealer < n; ++dealer) {
        for (std::size_t participant = 0; participant < n; ++participant) {
            states[dealer].push_back(
                init_participant(participant, n, threshold, x_coords));
        }
    }

    // Fase Deal: cada P_d ya ha creado su propio phi_d, r_d, A_d, B_d,
    // compromisos y prueba. Ahora cada participante procesa el broadcast
    // de cada dealer y su mensaje privado correspondiente.
    for (std::size_t dealer = 0; dealer < n; ++dealer) {
        for (std::size_t participant = 0; participant < n; ++participant) {
            echoes[dealer][participant] = receive_deal(
                states[dealer][participant],
                deals[dealer].broadcast_data,
                deals[dealer].private_data[participant]);

            if (n > 1 && echoes[dealer][participant].size() != n - 1) {
                throw std::runtime_error(
                    "A participant rejected a dealer's AVSS Deal.");
            }
        }
    }

    // Fase Echo: se enrutan simultáneamente los Echo de cada instancia AVSS.
    for (std::size_t dealer = 0; dealer < n; ++dealer) {
        for (std::size_t sender = 0; sender < n; ++sender) {
            std::size_t echo_index = 0;
            for (std::size_t receiver = 0; receiver < n; ++receiver) {
                if (receiver == sender)
                    continue;
                if (receive_echo(
                        states[dealer][receiver],
                        echoes[dealer][sender][echo_index])) {
                    ready_to_send[dealer][receiver] = true;
                }
                ++echo_index;
            }
        }
    }

    // Fase Ready: dos rondas de difusión bastan para esta simulación
    // síncrona y completamente honesta, en cada una de las n instancias.
    for (int round = 0; round < 2; ++round) {
        std::vector<std::vector<bool>> next_ready(
            n, std::vector<bool>(n, false));

        for (std::size_t dealer = 0; dealer < n; ++dealer) {
            for (std::size_t sender = 0; sender < n; ++sender) {
                if (!ready_to_send[dealer][sender])
                    continue;

                for (std::size_t receiver = 0; receiver < n; ++receiver) {
                    if (receive_rdy(states[dealer][receiver], sender))
                        next_ready[dealer][receiver] = true;
                }
            }
        }

        ready_to_send = std::move(next_ready);
    }

    DealerPolynomials local_polynomials(n);
    for (std::size_t dealer = 0; dealer < n; ++dealer) {
        local_polynomials[dealer].reserve(n);
        for (const ParticipantState& participant : states[dealer]) {
            auto local = reconstruct(participant);
            if (!local) {
                throw std::runtime_error(
                    "A participant could not reconstruct its AVSS share.");
            }
            local_polynomials[dealer].push_back(std::move(*local));
        }
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

    // Cada participante genera independientemente su contribución y prepara
    // una instancia completa del Deal AVSS con el mismo conjunto de n parties.
    std::vector<DealResult> deals;
    deals.reserve(n);
    for (std::size_t dealer = 0; dealer < n; ++dealer) {
        const FieldElement contribution = random_element();
        result.dealer_secrets[dealer] = contribution;
        result.aggregate_secret += contribution;
        deals.push_back(run_deal(contribution, n, threshold, x_coords));
    }

    // Se mantienen activas las n instancias: Deal, Echo y Ready se procesan
    // por separado para cada dealer, indexando los datos por [dealer][party].
    const DealerPolynomials local_polynomials =
        run_all_getshares(deals, n, threshold, x_coords);

    FieldElement zero;
    NTL::conv(zero, 0L);
    for (std::size_t dealer = 0; dealer < n; ++dealer) {
        for (std::size_t participant = 0; participant < n; ++participant) {
            FieldElement share;
            NTL::eval(
                share,
                local_polynomials[dealer][participant].phi_share,
                zero);
            result.dealer_shares[dealer][participant] = share;
            result.participant_shares[participant] += share;
        }
    }

    const FieldElement reconstructed = interpolate_at_zero(
        x_coords, result.participant_shares, threshold + 1);
    result.reconstruction_matches_sum = (reconstructed == result.aggregate_secret);
    return result;
}
