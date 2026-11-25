#include "deal.hpp"
#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include <stdexcept>

FieldElement compute_alpha(
    const std::vector<Commitment>& c_r_Y,
    const Commitment& c_phi
) {
    // Etiqueta de separación de dominio para fiat_shamir_rho, que evita colisiones.
    static const uint8_t TAG[] = {
        'A','L','P','H','A'
    };
    std::vector<uint8_t> buf;
    buf.insert(buf.end(), TAG, TAG + sizeof(TAG));
    for (const auto& c : c_r_Y)
        buf.insert(buf.end(), c.begin(), c.end());
    buf.insert(buf.end(), c_phi.begin(), c_phi.end());

    Hash_type h = hash_bytes(buf.data(), buf.size());
    NTL::ZZ z;
    NTL::ZZFromBytes(z, h.data(), static_cast<long>(HASH_SIZE));
    FieldElement alpha;
    NTL::conv(alpha, z % NTL::ZZ_p::modulus());
    return alpha;
}

DealResult run_deal(
    const FieldElement& secret,
    size_t n,
    size_t t,
    const std::vector<FieldElement>& x_coords
) {
    if (x_coords.size() != n)
        throw std::invalid_argument("erreur : x_coords.size() != n");
    if (n < t + 1)
        throw std::invalid_argument("erreur : n < t+1");

    // Genera phi(X,Y) aleatoriamente, fijando phi(0,0) = s 
    BivPoly phi = rand_biv_poly(t, secret);

    // Genera r(X,Y) aleatoriamente 
    BivPoly r = rand_biv_poly(t);
   
    std::vector<std::vector<FieldElement>> matrix_a(n, std::vector<FieldElement>(n));
    std::vector<std::vector<FieldElement>> matrix_b(n, std::vector<FieldElement>(n));
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j) {
            // matrix_a[i][j] = phi(x_i,x_j) 
            matrix_a[i][j] = phi.evaluate(x_coords[i], x_coords[j]);
            // matrix_b[i][j] = r(x_i,x_j)  
            matrix_b[i][j] = r.evaluate(x_coords[i], x_coords[j]);
        }

    // Compromiso Merkle de la columna b_{j,i} 
    std::vector<Commitment> c_r_X(n);
    for (size_t i = 0; i < n; ++i) {
        std::vector<FieldElement> col;
        for (size_t j = 0; j < n; ++j) col.push_back(matrix_b[j][i]);
        c_r_X[i] = MerkleTree::commit(col);
    }

    // Compromiso Merkle de la fila b_{i,j} 
    std::vector<Commitment> c_r_Y(n);
    for (size_t i = 0; i < n; ++i)
        c_r_Y[i] = MerkleTree::commit(matrix_b[i]);

    // Compromiso Merkle de la fila a_{i,j}
    std::vector<MerkleTree::Tree> tree_a(n);
    std::vector<Commitment> c_a_X_Y(n);
    for (size_t i = 0; i < n; ++i) {
        tree_a[i]  = MerkleTree::tree_field(matrix_a[i]);
        c_a_X_Y[i] = tree_a[i].root();
    }

    // c_phi es el compromiso Merkle de las raíces de las filas de a 
    MerkleTree::Tree tree_c_phi = MerkleTree::tree_commits(c_a_X_Y);
    Commitment c_phi = tree_c_phi.root();

    // pi_i es la ruta Merkle desde la hoja c_{a(x_i,Y)} hasta c_phi 
    std::vector<MerklePath> pi_paths(n);
    for (size_t i = 0; i < n; ++i)
        pi_paths[i] = tree_c_phi.path(i);

    // alpha = H(c_r_Y[0] || ... || c_r_Y[n-1] || c_phi) 
    FieldElement alpha = compute_alpha(c_r_Y, c_phi);
    // psi = phi + alpha * r 
    BivPoly psi = add(phi, scalar_mul(r, alpha));

    //  Matriz de evaluaciones de psi 
    std::vector<std::vector<FieldElement>> psi_mat(n, std::vector<FieldElement>(n));
    for (size_t i = 0; i < n; ++i)
        for (size_t j = 0; j < n; ++j)
            psi_mat[i][j] = psi.evaluate(x_coords[i], x_coords[j]);

    // Compromiso Merkle de la columna psi(x_j,x_i) 
    // La hoja j es psi_mat[j][i], en la columna i de la matriz psi
    std::vector<Commitment> c_psi_X(n);
    for (size_t i = 0; i < n; ++i) {
        std::vector<FieldElement> col;
        for (size_t j = 0; j < n; ++j) col.push_back(psi_mat[j][i]);
        c_psi_X[i] = MerkleTree::commit(col);
    }

    // Compromiso Merkle de la fila psi(x_i,x_j) 
    // Se conservan los árboles completos  
    std::vector<MerkleTree::Tree> row_trees_psi(n);
    std::vector<Commitment> c_psi_Y(n);
    for (size_t i = 0; i < n; ++i) {
        row_trees_psi[i] = MerkleTree::tree_field(psi_mat[i]);
        c_psi_Y[i] = row_trees_psi[i].root();
    }

    // c es el compromiso Merkle de los compromisos c_{psi(X,x_i)} 
    Commitment c = MerkleTree::commit(c_psi_X);

    // pi es la prueba de proximidad de psi para el umbral t 
    ProximityProof pi = prove_prox(psi, t, x_coords, c, c_psi_Y, row_trees_psi);

    // Difunde c, pi, c_phi y los compromisos c_r_X, c_r_Y, c_psi_X y c_psi_Y
    BroadcastMessage bcast;
    bcast.c = c;
    bcast.pi = std::move(pi);
    bcast.c_phi = c_phi;
    bcast.c_r_X = std::move(c_r_X);
    bcast.c_r_Y = std::move(c_r_Y);
    bcast.c_psi_X = std::move(c_psi_X);
    bcast.c_psi_Y = std::move(c_psi_Y);

    // Envía a cada P_i sus valores {a_{i,j}, b_{i,j}}_j y la ruta pi_i 
    std::vector<PrivateMessage> privates(n);
    for (size_t i = 0; i < n; ++i) {
        privates[i].a_i = matrix_a[i];
        privates[i].b_i = matrix_b[i];
        privates[i].pi_i = pi_paths[i];
    }

    DealerInternalState internal;
    internal.c_a_X_Y = std::move(c_a_X_Y);
    internal.tree_c_phi = std::move(tree_c_phi);

    DealResult res;
    res.broadcast_data = std::move(bcast);
    res.private_data = std::move(privates);
    res.internal_state = std::move(internal);
    return res;
}
