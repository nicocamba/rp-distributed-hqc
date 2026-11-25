#include "field.hpp"
#include <stdexcept>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#else
#include <fstream>
#endif

namespace {

bool is_init = false;

/*
 * Inicializa el generador aleatorio de NTL con una fuente criptográfica del sistema operativo.
 * En Windows usa CNG; en macOS y Linux usa /dev/urandom.
 */
void seed_rng() {
    constexpr std::size_t SEED_BYTES = 32;
    std::vector<unsigned char> seed(SEED_BYTES);

#if defined(_WIN32)
    const NTSTATUS status = BCryptGenRandom(
        nullptr,
        seed.data(),
        static_cast<ULONG>(seed.size()),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
    if (status != 0) {
        throw std::runtime_error(
            "error: BCryptGenRandom could not provide a seed.");
    }
#else
    std::ifstream urandom("/dev/urandom", std::ios::binary);
    if (!urandom) {
        throw std::runtime_error(
            "error: could not open /dev/urandom.");
    }
    urandom.read(reinterpret_cast<char*>(seed.data()),
                 static_cast<std::streamsize>(SEED_BYTES));
    if (!urandom) {
        throw std::runtime_error(
            "error: incomplete read from /dev/urandom.");
    }
#endif

    NTL::SetSeed(seed.data(), static_cast<long>(seed.size()));
}

} // anonymous namespace

void init_field(const NTL::ZZ& prime) {
    NTL::ZZ_p::init(prime);
    seed_rng();
    is_init = true;
}

FieldElement random_element() {
    if (!is_init) {
        throw std::runtime_error(
            "error: the field is not initialized.");
    }
    FieldElement x;
    NTL::random(x);
    return x;
}
