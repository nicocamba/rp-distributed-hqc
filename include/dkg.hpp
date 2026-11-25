#ifndef DKG_H
#define DKG_H

#include <cstddef>
#include <vector>
#include "field.hpp"

/*
 * Resultado de la simulación centralizada del DKG.
 * Los secretos individuales y la clave global se incluyen únicamente
 * para poder inspeccionar y comprobar la demo; una DKG real no los revela.
 */
struct DKGResult {
    std::vector<FieldElement> dealer_secrets;
    std::vector<std::vector<FieldElement>> dealer_shares;
    std::vector<FieldElement> participant_shares;
    FieldElement aggregate_secret;
    bool reconstruction_matches_sum;
};

/*
 * Simula un DKG honesto: cada participante muestrea un secreto de F_p,
 * lo reparte mediante una instancia de Deal/GetShare, y cada participante
 * suma las contribuciones que recibió.
 */
DKGResult run_dkg(
    std::size_t n,
    std::size_t threshold,
    const std::vector<FieldElement>& x_coords
);

#endif // DKG_H
