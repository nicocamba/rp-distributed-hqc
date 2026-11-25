#include "proximity_proof.hpp"
#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include <NTL/ZZ_pX.h>
#include <NTL/vec_ZZ_p.h>
#include <stdexcept>
#include <cstring>

/*
 * Genera el desafío no interactivo rho mediante Fiat–Shamir.
 * Aplica hash a la concatenación de una etiqueta de dominio y los compromisos públicos.
 * Según el lema de Schwartz–Zippel 
 */
FieldElement fiat_shamir_rho(
    // Raíz del árbol global de psi(X,Y)
    const Commitment& c_psi,
    // Raíces de los árboles de psi(X,x_i), el share de cada P_i
    const std::vector<Commitment>& c_psi_Y
) {

    /*
     * Etiquetas de separación de dominio para proteger contra ataques de repetición.
     * Evita que Hash(a) produzca el mismo resultado en otro contexto de hash.
     * Se usan los 13 bytes ASCII de "AVSS_PROX_RHO" 
     */ 
    static const uint8_t TAG[] = {
        'P','R','O','X','_','R','H','O'
    };

 // Reserva espacio y construye buf: TAG || c_psi || c_psi_Y
    std::vector<uint8_t> buf;
    buf.reserve(sizeof(TAG) + HASH_SIZE * (1 + c_psi_Y.size()));
    buf.insert(buf.end(), TAG, TAG + sizeof(TAG));
    buf.insert(buf.end(), c_psi.begin(), c_psi.end());
    for (const auto& c : c_psi_Y)
        buf.insert(buf.end(), c.begin(), c.end());

    // Aplica hash al arreglo buf
    Hash_type h = hash_bytes(buf.data(), buf.size());
    NTL::ZZ z;
    NTL::ZZFromBytes(z, h.data(), static_cast<long>(HASH_SIZE));
    FieldElement rho;
    // Reducción módulo p
    NTL::conv(rho, z % NTL::ZZ_p::modulus());
    return rho;
}

ProximityProof prove_prox(
    const BivPoly& psi,
    size_t t,
    const std::vector<FieldElement>& eval_pts,
    const Commitment& c_psi,
    const std::vector<Commitment>& c_psi_Y,
    const std::vector<MerkleTree::Tree>& row_trees
) {
    const size_t n = eval_pts.size(); // número de participantes P_i

    if (n < t + 1) {
        throw std::invalid_argument(
            "erreur : n < t+1 ; pas assez de points pour interpoler.");
    }
    if (row_trees.size() != n || c_psi_Y.size() != n) {
        throw std::invalid_argument(
            "erreur : tailles incoherentes entre n, row_trees et c_psi_Y).");
    }

    ProximityProof proof;
    // Genera el desafío no interactivo mediante Fiat–Shamir
    proof.rho = fiat_shamir_rho(c_psi, c_psi_Y);
    // h(X) = psi(X, rho)
    proof.h_X = eval_Y(psi, proof.rho);

    // Genera t+1 aperturas para cada P_i.
    proof.openings.resize(n);
    for (size_t i = 0; i < n; ++i) {
        ProximityProof:: RowOpen& op = proof.openings[i];

        for (size_t j = 0; j <= t; ++j) {
            op.indices.push_back(j);
            // Calcula psi(x_i,x_j) 
            op.evals.push_back(psi.evaluate(eval_pts[i], eval_pts[j]));
            // Obtiene la ruta Merkle de cada hoja j
            op.paths.push_back(row_trees[i].path(j));
        }
    }

    return proof;
}

bool verify_prox(
    const ProximityProof& proof,
    size_t t,
    const std::vector<FieldElement>& eval_pts,
    const Commitment& c_psi,
    const std::vector<Commitment>& c_psi_Y
) {
    const size_t n = eval_pts.size();

    // Comprueba las dimensiones
    if (proof.openings.size() != n || c_psi_Y.size() != n) {
        return false;
    }
 
    // Verifica el desafío no interactivo de Fiat–Shamir
    FieldElement expected_rho = fiat_shamir_rho(c_psi, c_psi_Y);
    if (proof.rho != expected_rho) {
        return false;
    }

    // Comprueba que grado(h(X)) <= t (sin reducir el polinomio usando t+1 puntos)
    if (NTL::deg(proof.h_X) > static_cast<long>(t)) {
        return false;
    }

    for (size_t i = 0; i < n; ++i) {
        const ProximityProof::RowOpen& op = proof.openings[i];

        // Verifica los t+1 puntos
        if (op.indices.size() != t + 1 ||
            op.evals.size() != t + 1 ||
            op.paths.size() != t + 1) {
            return false;
        }

        /* 
         * Comprueba que las hojas corresponden a la 
         * raíz indicada mediante su ruta en el árbol Merkle 
         */
        for (size_t k = 0; k <= t; ++k) {
            Hash_type leaf = hash_element(op.evals[k]);
            if (!verify_path(c_psi_Y[i], leaf, op.paths[k])) {
                return false;
            }
        }

        // Reconstruye el polinomio univariado p_i mediante interpolación de Lagrange
        NTL::vec_ZZ_p xa, ya;
        xa.SetLength(static_cast<long>(t + 1));
        ya.SetLength(static_cast<long>(t + 1));
        for (size_t k = 0; k <= t; ++k) {
            xa[static_cast<long>(k)] = eval_pts[op.indices[k]];
            ya[static_cast<long>(k)] = op.evals[k];
        }
        Uni_Poly p_i; // En realidad es psi
        NTL::interpolate(p_i, xa, ya);

        // Comprueba la intersección: psi(x_i,rho) == h(x_i)
        FieldElement p_i_rho, h_xi;
        NTL::eval(p_i_rho, p_i, proof.rho);
        NTL::eval(h_xi, proof.h_X, eval_pts[i]);

        if (p_i_rho != h_xi) { // ¿Se cumple psi(x_i, rho) == h(x_i)?
            return false;
        }
    }

    return true;
} // tema estudiado: usar AVSS para la clave secreta en HQC distribuido
