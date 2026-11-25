#include "avss/crypto/merkle.hpp"
#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include <cstring>
#include <stdexcept>
#include <vector>

long long hash_count = 0;

namespace {

/* Implementación interna de SHA-256 según la norma FIPS 180-4.
 * Consulta este enlace para ver la especificación oficial del NIST:
 * Enlace: https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf
 */
static const uint32_t SHA256_K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

/* 
 * Vector de inicialización H^(0), compuesto por 8 valores 
 * de 32 bits que se usan al inicio del hash.
 */
static const uint32_t SHA256_IV[8] = {
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
};

/* 
 * Aplica una rotación circular de n bits
 * hacia la derecha sobre una palabra de 32 bits.
 */
inline uint32_t rotr32(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32u - n));
}

//Función de compresión de SHA-256 
void sha256_block(uint32_t state[8], const uint8_t block[64]) {
    uint32_t w[64]; // message schedule : W_0, W_1, ..., W_63

    // Inicializa las primeras 16 palabras W_0, ..., W_15 
    for (int i = 0; i < 16; ++i) { 
        // Combina 4 bytes en una palabra de 32 bits en orden big-endian
        w[i] = (uint32_t(block[4*i+0]) << 24) // AA000000
              | (uint32_t(block[4*i+1]) << 16) // +00BB0000
              | (uint32_t(block[4*i+2]) <<  8) // +0000CC00
              |  uint32_t(block[4*i+3]);  // + 000000DD = AABBCCDD
    }

    /* 
     * Expande el mensaje para obtener W_16, ..., W_63 mediante la
     * fórmula: W_i = g1(W_{i-2}) + W_{i-7} + g0(W_{i-15}) + W_{i-16}
     * Así se obtienen 64 palabras para las 64 rondas de compresión. 
     */
    for (int i = 16; i < 64; ++i) {
        uint32_t g0 = rotr32(w[i-15], 7) ^ rotr32(w[i-15], 18) ^ (w[i-15] >>  3);
        uint32_t g1 = rotr32(w[i- 2], 17) ^ rotr32(w[i- 2], 19) ^ (w[i- 2] >> 10);
        w[i] = g1 + w[i-7] + g0 + w[i-16];
    }

    // Inicializa las variables de trabajo con el estado actual
    uint32_t a=state[0], b=state[1], c=state[2], d=state[3];
    uint32_t e=state[4], f=state[5], g=state[6], h=state[7];

    /* 
     * Bucle de compresión de 64 rondas que mezcla, 
     * suma y combina mediante XOR las variables de trabajo con
     * los mensajes w[i] y las constantes SHA256_K[i], módulo 2^32.
     */
    for (int i = 0; i < 64; ++i) {
        uint32_t s1  = rotr32(e, 6)  ^ rotr32(e, 11) ^ rotr32(e, 25);

        /* 
         * Elige entre f y g según el bit de e.
         * ch = (e AND f) XOR (NOT e AND g)
         * Si el bit de e es 1, toma el bit correspondiente de f 
         * en esa posición; de lo contrario, toma el bit de g.
         */
        uint32_t ch  = (e & f) ^ (~e & g);
        uint32_t t1  = h + s1 + ch + SHA256_K[i] + w[i];
        uint32_t s0  = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);

        // En el vector (a,b,c), toma el bit mayoritario 
        // (0 o 1) entre a, b y c para cada posición
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2  = s0 + maj;

        //Actualiza las variables de trabajo para la ronda siguiente
        h=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }

    // Suma el estado anterior a las variables de trabajo tras las 64 rondas
    state[0]+=a; state[1]+=b; state[2]+=c; state[3]+=d;
    state[4]+=e; state[5]+=f; state[6]+=g; state[7]+=h;
}

// Función SHA-256 completa, incluido el relleno 
Hash_type sha256_raw(const uint8_t* data, size_t len) {
    hash_count++;
    uint32_t state[8];
    // Inicializa el arreglo de estados con los valores de H^(0)
    for (int i = 0; i < 8; ++i) state[i] = SHA256_IV[i];

    /* 
     * Llama a sha256_block() para procesar todos los bloques de al menos 64 bytes
     * El bucle se detiene cuando quedan menos bytes por leer y reserva ese resto  
     * del mensaje para el paso de relleno.
     */
    size_t offset = 0;
    for (; offset + 64 <= len; offset += 64) {
        sha256_block(state, data + offset);
    }

    /* 
     * Crea el arreglo vacío buf, de 128 bytes.
     * Copia los bytes restantes (tail) desde 
     * la posición de lectura (data + offset) al inicio de buf.
     * Marca el final del mensaje con un bit 1 seguido de ceros 
     * para completar el bloque.
     */
    uint8_t buf[128] = {}; 
    size_t tail = len - offset; 
    std::memcpy(buf, data + offset, tail); 
    buf[tail] = 0x80u; 

    /* 
     * Añade el tamaño del mensaje original en bits.
     * La norma exige colocar ese tamaño en los últimos 8 bytes.
     * Si tail < 56, cabe en el bloque actual de 64 bytes.
     * Si no, el relleno ocupa un segundo bloque y el tamaño 
     * se escribe al final de ese segundo bloque (128 bytes en total).
     */
    uint64_t bit_len = static_cast<uint64_t>(len) * 8u;
    if (tail < 56) {
        // Cabe en un único bloque final de 64 bytes
        for (int j = 0; j < 8; ++j)
            buf[63 - j] = static_cast<uint8_t>(
                bit_len >> (8u * static_cast<unsigned>(j)));
        sha256_block(state, buf);
    } else {
        // Requiere dos bloques finales (128 bytes)
        for (int j = 0; j < 8; ++j)
            buf[127 - j] = static_cast<uint8_t>(
                bit_len >> (8u * static_cast<unsigned>(j)));
        sha256_block(state, buf);
        sha256_block(state, buf + 64);
    }

    /* 
     * Convierte el estado final en el arreglo de bytes digest.
     * El arreglo state contiene 8 palabras de 32 bits. 
     * Divide cada palabra en 4 bytes de 8 bits para obtener 
     * el resultado estandarizado de 32 bytes, es decir, 256 bits.
     */
    Hash_type digest;
    for (int j = 0; j < 8; ++j) {
        digest[static_cast<size_t>(4*j+0)] = static_cast<uint8_t>(state[j] >> 24);
        digest[static_cast<size_t>(4*j+1)] = static_cast<uint8_t>(state[j] >> 16);
        digest[static_cast<size_t>(4*j+2)] = static_cast<uint8_t>(state[j] >>  8);
        digest[static_cast<size_t>(4*j+3)] = static_cast<uint8_t>(state[j]);
    }
    return digest;
}

// Calcula la menor potencia de 2 que sea mayor o igual que n.
size_t next_pow2(size_t n) {
    size_t p = 1;
    // Desplazamiento a la izquierda: p se duplica en cada iteración.
    while (p < n) p = p * 2;
    return p;
}

} // Fin del espacio de nombres anónimo

Hash_type hash_bytes(const uint8_t* data, size_t len) {
    return sha256_raw(data, len);
}

Hash_type hash_element(const FieldElement& x) {
    // Convierte el elemento de F_p a ZZ
    NTL::ZZ z = NTL::rep(x);  

    // Calcula log_2(p)/8, el número de bytes necesarios para representar p
    long mod_bytes = NTL::NumBytes(NTL::ZZ_p::modulus());
    if (mod_bytes <= 0) mod_bytes = 1; 

    // Crea un arreglo de bytes de tamaño fijo, rellenado con ceros
    std::vector<uint8_t> buf(static_cast<size_t>(mod_bytes), 0u);
    NTL::BytesFromZZ(buf.data(), z, mod_bytes); // Guarda z en buf

    // Aplica el hash final al arreglo de bytes; la salida tiene 32 bytes
    return hash_bytes(buf.data(), buf.size());
}

Hash_type hash_concat(const Hash_type& left, const Hash_type& right) {
    uint8_t buf[HASH_SIZE * 2]; 
    //Concatena left y right en buf antes de aplicar el hash
    std::memcpy(buf, left.data(),  HASH_SIZE);
    std::memcpy(buf + HASH_SIZE, right.data(), HASH_SIZE);
    return hash_bytes(buf, sizeof(buf)); // hash de la concatenación
}


// MerkleTree

namespace MerkleTree {

Commitment Tree::root() const {
    // Comprueba si el árbol está vacío y si existe una raíz
    if (levels.empty() || levels.back().empty()) {
        throw std::logic_error(
            "erreur : L'arbre est vide ou mal construit.");
    }
    return levels.back()[0]; // devuelve la raíz
}

MerklePath Tree::path(size_t position) const {
    if (levels.empty()) {
        throw std::logic_error(
            "erreur : L'arbre est vide.");
    }
    if (position >= levels[0].size()) {
        throw std::out_of_range(
            "erreur : La position est hors borne.");
    }

    MerklePath p; // contendrá los hashes hermanos durante el ascenso
    size_t idx = position;

    for (size_t lvl = 0; lvl + 1 < levels.size(); ++lvl) {
        // Nodo actual izquierdo: índice par 
        if (idx % 2 == 0) { 
            size_t sib = idx + 1; // hermano derecho (true)

            // Añade a p el hash del nodo hermano
            p.siblings.push_back(levels[lvl][sib]); 
            p.directions.push_back(true);

        // Nodo actual derecho: índice impar 
        } else {
            // idx-1 es el hermano izquierdo (false)
            p.siblings.push_back(levels[lvl][idx - 1]);
            p.directions.push_back(false);
        }
        idx >>= 1; // pasa al nivel superior
    }
    return p; // la ruta completa
}

Tree build(const std::vector<Hash_type>& leaves) {
    if (leaves.empty()) { // comprobación de verificación
        throw std::invalid_argument(
            "erreur : liste de feuilles vide.");
    }

    Tree tree;

    // Rellena las hojas con ceros hasta alcanzar la siguiente potencia de 2 
    size_t padded = next_pow2(leaves.size());

    // Guarda las hojas y el relleno de ceros en el arreglo current
    std::vector<Hash_type> current(padded, Hash_type{});
    for (size_t i = 0; i < leaves.size(); ++i) current[i] = leaves[i];

    // Inserta en el árbol el primer nivel: current = levels[0]
    tree.levels.push_back(current);

    // Construye el árbol nivel por nivel mientras haya más de un nodo 
    // hasta llegar al último nivel, que contiene la raíz.
    while (current.size() > 1) {
        std::vector<Hash_type> next;
        // El tamaño del nivel n+1 es la mitad del tamaño del nivel n
        next.reserve(current.size() / 2);

        // Concatena mediante hash_concat()
        for (size_t i = 0; i < current.size(); i += 2) { 
            next.push_back(hash_concat(current[i], current[i + 1]));
        }
        //Guarda el resultado en tree
        tree.levels.push_back(next);
        current = std::move(next);
    }

    return tree;
}

Tree tree_field(const std::vector<FieldElement>& values) {
    if (values.empty()) {
        throw std::invalid_argument(
            "erreur : liste de valeurs vide.");
    }
    std::vector<Hash_type> leaves;
    leaves.reserve(values.size());
    for (const auto& v : values) {
        leaves.push_back(hash_element(v));
    }
    return build(leaves);
}

Tree tree_commits(const std::vector<Commitment>& values) {
    if (values.empty()) {
        throw std::invalid_argument(
            "erreur : liste d'engagements polynomiaux vide.");
    }
    return build(values); 
}

/* 
 * Funciones auxiliares commit() para obtener la raíz sin guardar
 * todo el árbol, independientemente del tipo de datos.
 */
Commitment commit(const std::vector<FieldElement>& values) {
    return tree_field(values).root();
}

Commitment commit(const std::vector<Commitment>& values) {
    return tree_commits(values).root();
}

} // Fin del espacio de nombres MerkleTree

bool verify_path(const Commitment& root, const Hash_type& leaf, const MerklePath& path) {
    if (path.siblings.size() != path.directions.size()) {
        return false; // ruta mal formada
    }
    Hash_type current = leaf;
    for (size_t i = 0; i < path.siblings.size(); ++i) {

        // Como Hash(A || B) no es igual a Hash(B || A):
        if (path.directions[i]) {
            current = hash_concat(current, path.siblings[i]);
        } else {
            current = hash_concat(path.siblings[i], current);
        }
    }
    return current == root;
}
