#ifndef MERKLE_H
#define MERKLE_H

#include <array>
#include <vector>
#include <cstdint>
#include <cstddef>
#include "field.hpp"

constexpr size_t HASH_SIZE = 32; // 32 bytes = 256 bits para SHA-256
using Hash_type = std::array<uint8_t, HASH_SIZE>;
using Commitment = Hash_type;

/*
 * Función hash estándar H: {0,1}* -> {0,1}^256.
 * Recibe el mensaje data, de longitud arbitraria, y su tamaño len
 * y devuelve su hash SHA-256 de 32 bytes.
 */
Hash_type hash_bytes(const uint8_t* data, size_t len);

/* 
 * Aplica hash a un elemento de F_p tras convertirlo en una cadena 
 * de bytes de tamaño fijo y aplicar SHA-256.
 */
Hash_type hash_element(const FieldElement& x);

/*
 * Concatena dos hashes de 32 bytes 
 * para obtener un nuevo hash.
 */ 
Hash_type hash_concat(const Hash_type& left, const Hash_type& right);

// Estructura de una ruta en un árbol Merkle
struct MerklePath {
    std::vector<Hash_type> siblings;
    // Hash(A,B) no es igual a Hash(B,A): 
    std::vector<bool> directions; // true = el hermano está a la derecha; false = a la izquierda
};

namespace MerkleTree {

    // Árbol Merkle que conserva todos sus niveles
    struct Tree {
        // Niveles del árbol: levels[0] son las hojas y levels.back() contiene la raíz
        std::vector<std::vector<Hash_type>> levels; 

        // Devuelve la raíz del árbol a partir de sus hojas
        Commitment root() const; 

        /*
         * Recibe la posición de una hoja y 
         * construye su ruta hasta la raíz.
         * Esta prueba criptográfica permite comprobar 
         * que la hoja pertenece al árbol, sin conocer 
         * el resto de los datos.
         */
        MerklePath path(size_t position) const; 
    };

    /* Recibe bloques de 32 bytes (Hash_type)
     * y construye el árbol completo a partir de hojas ya hasheadas.
     */ 
    Tree build(const std::vector<Hash_type>& leaves);

    /*
     * Construye un árbol Merkle a partir de elementos de F_p
     * Aplica hash_element() a los elementos y luego llama a build() 
     * para construir el árbol.
     */ 

    Tree tree_field(const std::vector<FieldElement>& values);

    /* 
     * Construye un árbol Merkle a partir de compromisos polinómicos.
     * Como Commitment = Hash_type, basta con llamar a build().
     */
    Tree tree_commits(const std::vector<Commitment>& values);

    /* Calcula la raíz del árbol Merkle a partir 
     * de una lista de elementos de F_p, sin almacenar 
     los niveles del árbol completo 
     */ 
    Commitment commit(const std::vector<FieldElement>& values);

    /* Calcula la raíz del árbol Merkle a partir 
     * de una lista de compromisos, sin almacenar 
     los niveles del árbol completo 
     */ 
    Commitment commit(const std::vector<Commitment>&   values);

} // fin del espacio de nombres MerkleTree

/* Comprueba que una hoja pertenece
 * al árbol recalculando la raíz a partir de la ruta 
 * MerklePath proporcionada.
 * Devuelve true si la raíz recalculada coincide con la 
 * raíz dada; si la prueba o la hoja no son válidas, devuelve false.
 */ 
bool verify_path(const Commitment& root, const Hash_type& leaf, const MerklePath& path);

#endif // MERKLE_H
