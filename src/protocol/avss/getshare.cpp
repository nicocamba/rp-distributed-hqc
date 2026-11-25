#include "avss/protocol/avss/getshare.hpp"
#include <NTL/ZZ_p.h>
#include <NTL/ZZ_pX.h>
#include <NTL/vec_ZZ_p.h>
#include <stdexcept>

ParticipantState init_participant(
    size_t my_index,
    size_t n_Pi,
    size_t threshold,
    const std::vector<FieldElement>& x_coords
) {
    if (my_index >= n_Pi)
        throw std::invalid_argument("erreur : indice hors bornes.");
    if (x_coords.size() != n_Pi)
        throw std::invalid_argument("erreur : x_coords doit valoir n.");

    ParticipantState state;
    state.my_index = my_index;
    state.n_Pi = n_Pi;
    state.threshold = threshold;
    state.x_coords = x_coords;

    state.hon_d = false;
    state.V.clear();

    state.seen_echo.assign(n_Pi, false);
    state.rdy_counted.assign(n_Pi, false);

    state.ready_count = 0;
    state.ready_sent = false;

    return state;
}

std::vector<EchoMessage> receive_deal(
    ParticipantState& state,
    const BroadcastMessage& broadcast,
    const PrivateMessage& my_share
) {
    const size_t n = state.n_Pi;
    const size_t t = state.threshold;
    const size_t i = state.my_index;

    // Si falla la prueba de proximidad, se rechaza el mensaje.
    if (!verify_prox(broadcast.pi, t, state.x_coords, broadcast.c, broadcast.c_psi_Y)) {
        return {};
    }

    state.alpha = compute_alpha(broadcast.c_r_Y, broadcast.c_phi);

    std::vector<FieldElement> w_row(n);
    for (size_t j = 0; j < n; ++j)
        w_row[j] = my_share.a_i[j] + state.alpha * my_share.b_i[j];

    Commitment c_a_recomp = MerkleTree::commit(my_share.a_i);
    if (!verify_path(broadcast.c_phi, c_a_recomp, my_share.pi_i))
        return {};

    Commitment c_b_recomp = MerkleTree::commit(my_share.b_i);
    if (c_b_recomp != broadcast.c_r_Y[i])
        return {};

    Commitment c_w_recomp = MerkleTree::commit(w_row);
    if (c_w_recomp != broadcast.c_psi_Y[i])
        return {};

    state.hon_d = true;
    state.c_r_Y = broadcast.c_r_Y;
    state.c_psi_Y = broadcast.c_psi_Y;

    ValidPair my_pair;
    my_pair.from_index = i;
    my_pair.a_ji = my_share.a_i[i]; 
    my_pair.b_ji = my_share.b_i[i]; 
    state.V.push_back(my_pair);

    MerkleTree::Tree tree_b = MerkleTree::tree_field(my_share.b_i);
    MerkleTree::Tree tree_w = MerkleTree::tree_field(w_row);

    std::vector<EchoMessage> echo_2snd;
    echo_2snd.reserve(n - 1);

    for (size_t j = 0; j < n; ++j) {
        if (j == i) continue;

        EchoMessage echo;
        echo.from_index = i;
        echo.a_ij = my_share.a_i[j]; 
        echo.b_ij = my_share.b_i[j]; 
        echo.path_r = tree_b.path(j);  
        echo.path_psi = tree_w.path(j);  
        echo_2snd.push_back(std::move(echo));
    }

    return echo_2snd;
}

bool receive_echo(
    ParticipantState& state,
    const EchoMessage& echo
) {
    const size_t j = echo.from_index;

    if (j >= state.n_Pi || state.seen_echo[j])
        return false;
    state.seen_echo[j] = true;

    FieldElement w_ji = echo.a_ij + state.alpha * echo.b_ij;

    Hash_type feuille_w = hash_element(w_ji);
    if (!verify_path(state.c_psi_Y[j], feuille_w, echo.path_psi))
        return false;

    Hash_type feuille_b = hash_element(echo.b_ij);
    if (!verify_path(state.c_r_Y[j], feuille_b, echo.path_r))
        return false;

    ValidPair paire_validee;
    paire_validee.from_index = j;
    paire_validee.a_ji = echo.a_ij; 
    paire_validee.b_ji = echo.b_ij; 
    state.V.push_back(paire_validee);

    if (!state.ready_sent && state.hon_d && state.V.size() > 2 * state.threshold) {
        state.ready_sent = true;
        return true; 
    }
    return false;
}

bool receive_rdy(
    ParticipantState& state,
    size_t from_index
) {
    if (from_index >= state.n_Pi || state.rdy_counted[from_index])
        return false;

    state.rdy_counted[from_index] = true;
    state.ready_count++;

    if (!state.ready_sent && state.ready_count >= state.threshold + 1) {
        state.ready_sent = true;
        return true; 
    }
    return false;
}

std::optional<GetShareResult> reconstruct(
    const ParticipantState& state
) {
    if (state.ready_count < 2 * state.threshold + 1)
        return std::nullopt;

    if (state.V.size() <= state.threshold)
        return std::nullopt;

    const size_t nb_points = state.threshold + 1;

    NTL::vec_ZZ_p xs, phi_vals, r_vals;
    xs.SetLength(static_cast<long>(nb_points));
    phi_vals.SetLength(static_cast<long>(nb_points));
    r_vals.SetLength(static_cast<long>(nb_points));

    for (size_t k = 0; k < nb_points; ++k) {
        const ValidPair& paire = state.V[k];
        xs[static_cast<long>(k)] = state.x_coords[paire.from_index];
        phi_vals[static_cast<long>(k)] = paire.a_ji;
        r_vals[static_cast<long>(k)] = paire.b_ji;
    }

    GetShareResult resultat;
    NTL::interpolate(resultat.phi_share, xs, phi_vals);
    NTL::interpolate(resultat.r_share, xs, r_vals);

    return resultat;
}
