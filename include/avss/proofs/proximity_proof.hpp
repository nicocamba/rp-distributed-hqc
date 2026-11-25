#ifndef PROXIMITY_PROOF_H
#define PROXIMITY_PROOF_H
#include "avss/math/polynomial.hpp"
#include "avss/crypto/merkle.hpp"
#include <vector>
#include <cstddef>

// Estructura de la prueba de proximidad
struct ProximityProof {
    FieldElement rho; 
    Uni_Poly h_X; // = psi(X,rho)
    struct  RowOpen {
        // Al menos t+1 índices abiertos
        std::vector<size_t> indices; 
        // Valores correspondientes de psi
        std::vector<FieldElement> evals; 
        // Sus rutas Merkle
        std::vector<MerklePath> paths;   
    };
    // Aperturas para cada P_i
    std::vector< RowOpen> openings; 
};


/*
 * Hace que la prueba IOPP sea no interactiva mediante Fiat–Shamir: 
 * El Dealer calcula rho aplicando hash a sus propios 
 * compromisos públicos. 
 * Si modifica el polinomio secreto para hacer trampa, cambia el hash,
 * y con él cambia rho; así se detecta el intento de fraude.
 */
FieldElement fiat_shamir_rho(
    // Raíz del árbol Merkle de psi
    const Commitment& c_psi,
    // Raíz del árbol Merkle de psi(x_i,Y)
    const std::vector<Commitment>& c_psi_Y
);

/* Demuestra que el grado de psi(X,Y) es <= t calculando h_X = psi(X,rho).
 * Abre al menos t+1 posiciones del árbol Merkle y proporciona sus
 * rutas hasta la raíz c_psi_Y correspondiente a cada P_i.
 */
ProximityProof prove_prox(
    const BivPoly& psi,
    size_t t,
    const std::vector<FieldElement>& eval_pts,
    const Commitment& c_psi,
    const std::vector<Commitment>& c_psi_Y,
    const std::vector<MerkleTree::Tree>& row_trees
);

/*
 * Verifica la validez de los shares recibidos 
 * comprobando la prueba de proximidad.
 * Los P_i comprueban el cálculo Fiat–Shamir y el grado de 
 * h_X, y verifican todas las pruebas de inclusión Merkle.
 */
bool verify_prox(
    const ProximityProof& proof,
    size_t t,
    const std::vector<FieldElement>& eval_pts,
    const Commitment& c_psi,
    const std::vector<Commitment>& c_psi_Y
);

#endif // PROXIMITY_PROOF_H
