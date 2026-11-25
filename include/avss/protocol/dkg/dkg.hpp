#ifndef DKG_H
#define DKG_H

#include <cstddef>
#include <vector>
#include "avss/math/field.hpp"

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
 * Simula un DKG honesto con n instancias AVSS activas: cada participante
 * muestrea un secreto de F_p y prepara un Deal completo; después se procesan
 * las fases Deal, Echo y Ready de todas las instancias. Cada participante
 * suma las contribuciones que recibió.
 */
DKGResult run_dkg(
    std::size_t n,
    std::size_t threshold,
    const std::vector<FieldElement>& x_coords
);

#endif // DKG_H
