# AVSS mínimo y prototipo DKG honesto

Esta carpeta contiene el núcleo matemático y de protocolo de la implementación de la tesis, una demo AVSS (`avss_demo`) y un prototipo DKG honesto (`dkg_demo`). Se han dejado fuera las figuras, los ejecutables precompilados, los benchmarks y las pruebas unitarias.

Fuente: [Lyna27/projet-avss](https://github.com/Lyna27/projet-avss). La carpeta conserva el algoritmo de ese repositorio; `src/field.cpp` incluye únicamente una adaptación del origen de aleatoriedad para Windows.

## Qué implementa

1. **Campo finito y polinomios.** Trabaja en `F_p` con NTL. El Dealer crea un polinomio bivariado `φ(X,Y)` de grado total como máximo `t`, con `φ(0,0)=s`, y un polinomio aleatorio de máscara `r(X,Y)`.
2. **Deal.** Evalúa ambos polinomios en los puntos de los participantes. Forma los shares privados `aᵢⱼ=φ(xᵢ,xⱼ)` y `bᵢⱼ=r(xᵢ,xⱼ)`; construye compromisos Merkle; deriva `α` con Fiat–Shamir; forma `ψ=φ+αr`; y construye la prueba de proximidad.
3. **GetShare.** Cada participante valida el mensaje del Dealer, intercambia subshares con pruebas Merkle mediante mensajes `Echo`, aplica los umbrales `Ready` y reconstruye por interpolación sus polinomios locales `φ(X,xᵢ)` y `r(X,xᵢ)`.
4. **Comprobaciones de la demo.** `main.cpp` verifica la prueba, los caminos Merkle y que una interpolación de `t+1` participantes recupere el secreto de ejemplo. También imprime las filas de shares, raíces de compromiso, resultado de las verificaciones, tiempo y número de hashes.
5. **Prototipo DKG honesto.** `dkg_main.cpp` simula que cada participante escoge independientemente un elemento aleatorio de `F_p` y prepara un `Deal` AVSS completo para su contribución: sus propios `φᵢ`, `rᵢ`, matrices `Aᵢ` y `Bᵢ`, compromisos, prueba de proximidad y mensajes privados. Después se procesan las fases `Deal`, `Echo` y `Ready` de las `n` instancias AVSS activas. Cada participante suma las shares que recibió. Una interpolación de `t+1` shares agregadas comprueba que el resultado coincide con la suma de las contribuciones.

## Alcance y límites

- Es una **simulación en un solo proceso**: `main.cpp` y `dkg_main.cpp` pasan las estructuras de mensajes directamente entre funciones. No implementa sockets, temporizadores ni una red satelital real. En el DKG, los mensajes y estados se indexan por `[dealer][participante]` para mantener separadas las `n` instancias.
- Cada instancia AVSS conserva matrices `n×n`: las filas y columnas representan las evaluaciones en los `n` puntos de los participantes. La diagonal contiene la parte que el propio dealer/participante conserva localmente; los mensajes `Echo` se envían a los otros `n−1` participantes. No se elimina esa fila o columna de las matrices.
- `dkg_demo` es por ahora un prototipo con todos los participantes honestos: incluye las contribuciones de todos y no implementa aún el acuerdo de un conjunto común de dealers válidos ante fallos o mensajes maliciosos. La demo imprime las contribuciones y la suma para permitir inspeccionar el cálculo; una DKG real no revela esos valores.
- Implementa el AVSS sobre un **escalar de un campo primo**. No implementa el KEM HQC, la clave HQC como vector de bits, ni los decodificadores Reed–Muller/Reed–Solomon de la parte de descifrado distribuido.
- La fase `Recover` del protocolo no está implementada. La interpolación que aparece en la demo es una comprobación de reconstrucción, no el protocolo `Recover`.
- Los valores por defecto (`p=7919`, `n=10`, `t=3`, `s=62`) son para demostración, no parámetros criptográficos de producción.
- Para esta demo se exige `n > 3t`; `p` debe ser primo y mayor que `n`, para que los puntos `1,...,n` sean distintos en `F_p`.

## Archivos

| Archivo | Función |
|---|---|
| `main.cpp` | Orquesta y muestra la ejecución de Deal y GetShare; hace las comprobaciones finales. |
| `dkg_main.cpp` | Ejecuta la demo DKG honesta y muestra las contribuciones, shares individuales y shares agregadas. |
| `include/dkg.hpp`, `src/dkg.cpp` | Prepara una instancia AVSS por dealer, procesa sus fases por separado con estados `[dealer][participante]`, obtiene `phi_i(0,x_j)`, suma las contribuciones y comprueba la reconstrucción. |
| `include/field.hpp`, `src/field.cpp` | Tipo `FieldElement`, inicialización de `F_p` y muestreo aleatorio. En Windows usa el generador criptográfico del sistema (CNG); en macOS/Linux usa `/dev/urandom`. |
| `include/polynomial.hpp`, `src/polynomial.cpp` | Representación, generación, evaluación, suma y multiplicación escalar de polinomios bivariados. |
| `include/merkle.hpp`, `src/merkle.cpp` | SHA-256, árboles Merkle, compromisos, aperturas y verificación de caminos. |
| `include/proximity_proof.hpp`, `src/proximity_proof.cpp` | Prueba y verificación de proximidad, con desafío Fiat–Shamir y aperturas Merkle. |
| `include/deal.hpp`, `src/deal.cpp` | Mensajes públicos/privados y algoritmo del Dealer. |
| `include/getshare.hpp`, `src/getshare.cpp` | Estado local de cada participante, mensajes Echo/Ready y reconstrucción de shares locales. |
| `CMakeLists.txt` | Compila la demo con C++17, NTL y GMP; añade la biblioteca del sistema necesaria para aleatoriedad en Windows. |

## Dependencias

Se necesitan un compilador C++17, CMake, Ninja, NTL y GMP. En Windows, una combinación sencilla es [MSYS2](https://www.msys2.org/) **UCRT64**: instala las dependencias desde su terminal UCRT64 para que compilador y bibliotecas usen la misma toolchain.

```sh
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-ntl mingw-w64-ucrt-x86_64-gmp
```

Referencias de paquetes: [NTL para UCRT64](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-ntl) y [GMP para UCRT64](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-gmp).

## Compilar y ejecutar en Windows

Abre la terminal **MSYS2 UCRT64** y ejecuta:

```sh
cd /c/Users/nicpr/Desktop/AVSS
cmake -S . -B build -G Ninja
cmake --build build
./build/avss_demo.exe
```

Para ejecutar el prototipo DKG honesto:

```sh
./build/dkg_demo.exe
```

Se pueden pasar `n`, `t` y `p` en ese orden:

```sh
./build/avss_demo.exe 10 3 7919
```

El DKG acepta los mismos parámetros:

```sh
./build/dkg_demo.exe 10 3 7919
```

En macOS/Linux, con NTL y GMP instalados, los mismos comandos CMake compilan la demo; el ejecutable suele llamarse `build/avss_demo`.

## Qué observar al ejecutarlo

- Las matrices de evaluaciones `aᵢⱼ` y `bᵢⱼ` que representan el share y la máscara.
- Las raíces Merkle públicas y la prueba de proximidad.
- La aceptación/rechazo del Dealer y la validación de los mensajes Echo.
- La propagación de Ready y la reconstrucción de `φ(X,xᵢ)`, `r(X,xᵢ)` y `s=φ(0,0)`.
- El tiempo total y el contador de hashes.

El código sirve para estudiar el flujo del protocolo y sus verificaciones. No debe interpretarse como una implementación completa del descifrado HQC ni como una configuración lista para producción.
