#ifndef GETSHARE_H
#define GETSHARE_H

#include <vector>
#include <optional>
#include <cstddef>

#include "polynomial.hpp"
#include "deal.hpp"
#include "proximity_proof.hpp" 

/* 
 * Estructura del mensaje que P_i difunde a P_j
 * Contiene los subshares y sus pruebas.
 */
struct EchoMessage {
    size_t from_index; // índice de P_i
    FieldElement a_ij; // a_{i,j} = \phi(x_i, x_j)
    FieldElement b_ij; // b_{i,j} = r(x_i, x_j)
    MerklePath path_r; // prueba que conecta las hojas b_{i,j} con c_r_Y[i]
    MerklePath path_psi; // prueba que conecta las hojas psi(x_i, x_j) con c_psi_Y[i]
};

 /*
 * Estructura de un par (a,b)_{j,i}, verificado y validado 
 * por P_i tras recibir el subshare de P_j.
 * Así, P_i reúne en V dos columnas: 
 * V_phi = {a_{1,i}, a_{2,i}, ..., a_{n,i}} 
 * V_r = {b_{1,i}, b_{2,i}, ..., b_{n,i}} 
 */
struct ValidPair {
    size_t from_index; // índice de P_j
    FieldElement a_ji; // phi(x_j, x_i)
    FieldElement b_ji; // r(x_j, x_i)
};

/*
 * Estructura con el resultado final del protocolo GetShare:
 * Para cada P_i, sus polinomios univariados locales de grado t en X  
 * phi(X,x_i) y r(X,x_i), obtenidos mediante interpolación de Lagrange 
 * a partir de V_phi y V_r.
 * Por tanto, phi(0,x_i) y r(0,x_i) son los secretos locales de P_i 
 */
struct GetShareResult {
    Uni_Poly phi_share; // phi(X,x_i)
    Uni_Poly r_share;  // r(X,x_i)
};

/*
 * Estado local completo de un P_i durante el protocolo AVSS 
*/
struct ParticipantState {
    // Parámetros de identidad
    size_t my_index;
    size_t n_Pi;
    size_t threshold;
    std::vector<FieldElement> x_coords; 

    // Datos recibidos del algoritmo Deal
    bool hon_d; 
    FieldElement alpha; 

    // Fase Echo
    std::vector<ValidPair> V; 
    // Asegura que haya una contribución de cada P_i para lograr
    // una interpolación correcta y sin duplicados
    std::vector<bool> seen_echo;

    // Fase Ready
    size_t ready_count;
    bool ready_sent;
    std::vector<bool> rdy_counted;

    // Compromisos públicos
    std::vector<Commitment> c_r_Y; // raíces de las columnas r(X, x_j)
    std::vector<Commitment> c_psi_Y; // raíces de las columnas \psi(X, x_j)
};

// Inicializa el estado de un P_i
ParticipantState init_participant(
    size_t my_index,
    size_t n_Pi,
    size_t threshold,
    const std::vector<FieldElement>&  x_coords
);

/* 
 * Procesa el mensaje enviado por el Dealer:
 * Verifica la prueba de proximidad y las rutas Merkle de este participante.
 * Si todo es válido, pone hon_d a true y genera mensajes Echo para los demás.
 */
std::vector<EchoMessage> receive_deal(
    ParticipantState& state,
    const BroadcastMessage& broadcast,
    const PrivateMessage& my_share
);

/*
 * La ejecuta P_j al recibir un mensaje Echo de P_i.
 * Verifica las rutas Merkle enviadas por P_j. 
 * Si son válidas, añade la contribución al conjunto V.
 * Devuelve true si debe enviarse 
 * el mensaje Ready al alcanzar |V| > 2t.
 */
bool receive_echo(
    ParticipantState& state,
    const EchoMessage& echo
);

/*
 * Procesa un mensaje Ready.
 * Incrementa el contador y devuelve true si
 * se han recibido al menos t+1 Ready y debe enviar su propio Ready.
 */
bool receive_rdy(
    ParticipantState& state,
    size_t from_index
);

/*
 * Reconstruye el share del participante.
 * Comprueba que se recibieron >= 2t+1 Ready y que |V| > t.
 * Si se cumplen ambas condiciones, interpola mediante Lagrange 
 * y devuelve los polinomios univariados phi(X,x_i) y r(X,x_i).
 */
std::optional<GetShareResult> reconstruct(
    const ParticipantState& state
);



#endif /*
 * Estructura con el tiempo de ejecución y el número de
 * hashes del algoritmo GetShare.
 */
