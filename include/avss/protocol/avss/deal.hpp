#ifndef DEAL_H
#define DEAL_H

#include <vector>
#include <cstddef>
#include "avss/math/field.hpp"
#include "avss/math/polynomial.hpp"
#include "avss/crypto/merkle.hpp"
#include "avss/proofs/proximity_proof.hpp"  

/* 
 * Estructura que agrupa los datos públicos difundidos 
 * a todos los participantes de la red.
 */
struct BroadcastMessage {
    Commitment c; // raíz del árbol global asociado a psi
    ProximityProof pi; // prueba NI-IOPP de evaluación
    Commitment c_phi; // raíz del árbol global asociado a phi
    std::vector<Commitment> c_r_X; // compromisos de las columnas de r
    std::vector<Commitment> c_r_Y; // compromisos de las filas de r
    std::vector<Commitment> c_psi_X; // compromisos de las columnas: psi(X, x_i)
    std::vector<Commitment> c_psi_Y; // compromisos de los shares: psi(x_i, Y)
};

// Estructura con las partes privadas destinadas a los P_i.
struct PrivateMessage {
    std::vector<FieldElement> a_i; // a_{i,j} = phi(x_i, x_j)_j  
    std::vector<FieldElement> b_i; // b_{i,j} = r(x_i, x_j)_j 
    MerklePath pi_i;  // ruta Merkle desde la hoja c_a(x_i, Y) hasta c_phi 
};

// Estado interno del Dealer
struct DealerInternalState {
    std::vector<Commitment> c_a_X_Y; // feuilles : c_a(x_i,Y)_i
    MerkleTree::Tree tree_c_phi; // árbol completo 
};

// Estructura con los datos generados por el Dealer
struct DealResult {
    BroadcastMessage broadcast_data; // mensaje público
    std::vector<PrivateMessage> private_data; // vector con los shares privados, de tamaño n
    DealerInternalState internal_state; // datos internos del Dealer 
};

// Función principal del Dealer
DealResult run_deal(
    const FieldElement& secret,
    size_t n,
    size_t t,
    const std::vector<FieldElement>& x_coords
);

/* 
 * Genera el desafío no interactivo mediante Fiat–Shamir.
 * Combina la máscara r y el compromiso de phi para obtener:
 * alpha = H(c_r(x1,Y) || ... || c_r(xn,Y) || c_phi)
 */
FieldElement compute_alpha(
    const std::vector<Commitment>& c_r_Y,
    const Commitment& c_phi
);

#endif // DEAL_H
