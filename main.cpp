// Compilación: ejecuta make avss_demo y después ./avss_demo
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <time.h>
#include "deal.hpp"
#include "getshare.hpp" 

// Contador de hashes definido en merkle.cpp
extern long long hash_count; // 64 bits

//Parámetros predeterminados que pueden modificarse al ejecutar el programa
long p = 7919;
size_t n = 10; 
size_t t = 3;

//Funciones auxiliares para mostrar datos y realizar conversiones en la demostración

/*
 * Muestra un Hash_type de 32 bytes (64 caracteres hexadecimales) y escribe
 * solo el primer y el último carácter hexadecimal: 35eb5246...4fb3ff2c
*/ 
static std::string short_hex(const Hash_type& d) {
    std::ostringstream s;
    for (size_t i = 0; i < 4; ++i)
        s << std::hex << std::setw(2) << std::setfill('0') << (int)d[i];
    s << "...";
    for (size_t i = 28; i < 32; ++i)
        s << std::hex << std::setw(2) << std::setfill('0') << (int)d[i];
    return s.str();
}

// Muestra un FieldElement como entero 
static long fe_to_long(const FieldElement& x) {
    return NTL::to_long(NTL::rep(x));
}

// Separador visual
static void sep(char c = '-', int w = 56) {
    std::cout << std::string(w, c) << "\n";
}

// Interpolación de Lagrange 
static FieldElement lagrange_eval(
    const std::vector<FieldElement>& xs,
    const std::vector<FieldElement>& ys,
    const FieldElement& x
) {
    FieldElement result;
    NTL::conv(result, 0); // result = 0
    
    for (size_t i = 0; i < xs.size(); ++i) {
        FieldElement num, den;
        NTL::conv(num, 1); // numerador = 1
        NTL::conv(den, 1); // denominador = 1
        
        for (size_t j = 0; j < xs.size(); ++j) {
            if (i != j) {
                num *= (x - xs[j]); // num * (x - x_j)
                den *= (xs[i] - xs[j]); // den * (x_i - x_j)
            }
        }
        // resultado = suma de y_i * (numerador / denominador)
        // En main basta con evaluar en x = 0
        result += ys[i] * (num / den);
    }
    return result;
}

static std::vector<FieldElement> x_pts(size_t n) {
    std::vector<FieldElement> pts; // pts = [0,...,0]
    for (size_t i = 1; i <= n; ++i) { // empieza en i = 1 
        // Convierte i, un entero, en FieldElement
        FieldElement x; NTL::conv(x, static_cast<long>(i));
        pts.push_back(x); // pts = [1, 2,..., n]
    }
    return pts;
}


int main(int argc, char* argv[]) {
    if (argc >= 2) n = std::atoi(argv[1]); // convierte texto ASCII a entero
    if (argc >= 3) t = std::atoi(argv[2]); 
    if (argc >= 4) p = std::atol(argv[3]); // convierte texto ASCII a long para permitir un p grande

    // Comprobación del umbral de seguridad
    if (t >= n) {
        std::cerr << "Error: the threshold t must be less than n.\n";
        return 1;
    }

    if (t * 3 >= n) {
    std::cerr << "Error: the condition t < n/3 is not satisfied.\n";
    return 1;
}
    init_field(NTL::ZZ(p));
    

    FieldElement secret; NTL::conv(secret, 62L); // secret = 62 de tipo long

    // Muestra los parámetros
    std::cout << " p = " << p << "\n";
    std::cout << " n = " << n << "\n";
    std::cout << " t = " << t << "\n";
    std::cout << " secret s = " << fe_to_long(secret) << "\n";

    std::vector<FieldElement> pts = x_pts(n);
    std::cout << " evaluation points: x_i = 1, 2, ..., " << n << "\n";

    clock_t start_time = clock();
    // Ejecución del algoritmo Deal

    sep();
    std::cout << " Running the Deal algorithm: \n";
    DealResult res = run_deal(secret, n, t, pts);
    std::cout << " phi(X,Y) and r(X,Y) of degree <= " << t << " are generated, with phi(0,0) = s\n";
    std::cout << " Evaluations of phi and r are stored in matrices a_{i,j} and b_{i,j}, with " << n << "x" << n << " = " << n*n << " evaluations\n";
    std::cout << " Merkle commitments built\n";
    std::cout << " // alpha = Hash(c_phi || root(c_r_X[i]) || root(c_r_Y[i])) is computed\n";
    std::cout << " psi = phi + alpha*r is evaluated\n";
    std::cout << " Proximity proof generated\n";

    // BroadcastMessage: datos públicos que el Dealer envía a todos los participantes P_i
    sep();
    // Extrae los datos de BroadcastMessage y los muestra con el formato abreviado de short_hex()
    const auto& bcast = res.broadcast_data;
    std::cout << " Public data sent by the Dealer in BroadcastMessage:\n";
    std::cout << " c_phi is the Merkle root of phi: " << short_hex(bcast.c_phi) << "\n";
    std::cout << " c is the Merkle root of psi: " << short_hex(bcast.c) << "\n";
    std::cout << " rho is the Fiat-Shamir challenge: " << fe_to_long(bcast.pi.rho) << "\n";
    std::cout << " Number of commitments: " << n << " commitments c_r_X[i], c_r_Y[i], c_psi_X[i], c_psi_Y[i]\n";
    std::cout << " Example: c_r_X[0]: " << short_hex(bcast.c_r_X[0]) << "\n";
    std::cout << " Example: c_psi_Y[0]: " << short_hex(bcast.c_psi_Y[0]) << "\n";

    // PrivateMessages: datos privados que recibe cada P_i
    sep();
    std::cout << " Private values phi(x_i, Y) and r(x_i, Y) that D sends to each P_i in PrivateMessages:\n";
    for (size_t i = 0; i < n; ++i) {
        const auto& private_m = res.private_data[i];
        // Muestra la fila b_i 
        std::cout << " P_" << i+1
                  << " : a_{(" << i+1 << ",1)..n} = [";
        for (size_t j = 0; j < n; ++j) {
            std::cout << fe_to_long(private_m.a_i[j]); // private_m.a_i[j] = phi(x_i, Y)
            if (j < n-1) std::cout << ", ";
        }
        std::cout << "]\n";

        // Muestra la fila b_i 
        std::cout << "       b_{(" << i+1 << ",1)..n} = [";
        for (size_t j = 0; j < n; ++j) {
            std::cout << fe_to_long(private_m.b_i[j]); // private_m.b_i[j] = r(x_i, Y)
            if (j < n-1) std::cout << ", ";
        }
        std::cout << "]\n";
    }

    // Verifica la prueba de proximidad
    sep();
    std::cout << " Verifying the proximity proof for the polynomial degree and commitments: \n";
    // pi contiene, para cada P_i, las aperturas y rutas Merkle de las hojas correspondientes a c_psi_Y[i]
    bool prox_verif = verify_prox(
        bcast.pi, t, pts, bcast.c, bcast.c_psi_Y);
    std::cout << " verify_prox(bcast.pi,t, pts, bcast.c, bcast.c_psi_Y) = "
            // Si prox_verif es true, la prueba es válida; en caso contrario, no lo es
              << (prox_verif ? "Valid" : "Invalid") << "\n"; 
              
    // Verifica las rutas Merkle pi_i, con las que cada P_i comprueba su parte
    sep();
    std::cout << "Verifying Merkle paths pi_i\n";
    bool paths_verified = true;
    for (size_t i = 0; i < n; ++i) {
        Commitment c_a_i = MerkleTree::commit(res.private_data[i].a_i); // c_a_i = Hash(a_i)
        bool check_path = verify_path(bcast.c_phi, c_a_i, res.private_data[i].pi_i);
        std::cout << " P_" << i+1 << " : pi_" << i+1 << " in c_phi = "
                // Si check_path es true, la ruta es válida; en caso contrario, no lo es
                  << (check_path ? "Valid" : "Invalid") << "\n";
        // Si alguna ruta es inválida, paths_verified pasa a false
        if (!check_path) paths_verified = false;
    }

    // Reconstrucción del secreto a partir de los datos de Deal
    sep();
    std::cout << "Checking secret reconstruction with a simulation of t+1 honest participants:\n";

    // Usa los primeros t+1 puntos para interpolar el secreto local phi(x_i,0) de cada P_i
    FieldElement zero; NTL::conv(zero, 0L);
    std::vector<FieldElement> phi_x_0(t + 1); // phi_xi_Y = [phi(0,x_1), phi(0,x_2), ..., phi(0,x_{t+1})]
    for (size_t i = 0; i <= t; ++i) {
        phi_x_0[i] = lagrange_eval(pts, res.private_data[i].a_i, zero); // phi_xi_Y[0] = phi_xi_Y(x_i, 0)
    }
    // Interpola los primeros t+1 secretos locales phi_xi_Y[0] para recuperar phi(0,0)
    std::vector<FieldElement> pts_t1(pts.begin(), pts.begin() + static_cast<long>(t+1));
    FieldElement recovered = lagrange_eval(pts_t1, phi_x_0, zero);
    bool good_secret = (recovered == secret);
    std::cout << "reconstructed value: " << fe_to_long(recovered) << " " 
              << (good_secret ? "Valid" : "Invalid") << "\n\n";

    // Ejecución de GetShare entre los participantes P_i
    sep();
    std::cout << "Running the GetShare algorithm for participants P_i:\n";
    std::cout << "They verify the received shares and prepare Echo messages (subshares + Merkle proofs)\n";

    // Inicializa el estado de cada P_i después de recibir los shares 
    // con hon_d = 0, V = {} y ready_count = 0
    std::vector<ParticipantState> reseau;
    for (size_t i = 0; i < n; ++i) {
        reseau.push_back(init_participant(i, n, t, pts));
    }

    // Procesa el mensaje del Dealer
    std::vector<std::vector<EchoMessage>> echos_sub(n);
    bool D_hon = true;
    for (size_t exp = 0; exp < n; ++exp) {
        echos_sub[exp] = receive_deal(reseau[exp], bcast, res.private_data[exp]);
        if (echos_sub[exp].empty()) D_hon = false;
    }
    std::cout <<"hon_d = " << (D_hon ? "1: valid, honest Dealer" : "0: error, Dealer rejected") << "\n";

    // Intercambio de mensajes Echo entre P_i y P_j
    sep();
    std::cout << "Exchanging Echo messages containing subshares from P_i to P_j: \n";
    std::vector<bool> rdy_2send(n, false); // al principio nadie está listo
    for (size_t exp = 0; exp < n; ++exp) {
        size_t indice_echo = 0;
        // P_i envía sus mensajes Echo a los demás P_j
        for (size_t j = 0; j < n; ++j) {
            if (j == exp) continue;
            // Si el Echo es válido, añade a_i,j y b_i,j al conjunto V 
            //Devuelve true si |V| > 2t y hon_d = 1
            bool signal_ready = receive_echo(reseau[j], echos_sub[exp][indice_echo]);
            if (signal_ready) rdy_2send[j] = true; // Si es true, P_j está listo y envía Ready
            indice_echo++;
        }
    }
    std::cout << "Echo subshares have been verified and added to set V.\n";
    // Al final, rdy_2send = [F, T, F, T, T]: P2, P4 y P5 están listos, pero P1 y P3 no

    // Acumulación de mensajes Ready
    sep();
    std::cout << "Collecting 2t+1 Ready messages: \n";
    for (int tour = 1; tour <= 2; ++tour) { // Dos rondas para el ejemplo con un Dealer honesto y t+1 participantes honestos 
        std::vector<bool> new_ready(n, false);
        for (size_t exp = 0; exp < n; ++exp) {
            if (!rdy_2send[exp]) continue; // Se omiten los P_i que no están listos según rdy_2send
            for (size_t j = 0; j < n; ++j) {
                // Cada P_i que está listo envía Ready a los demás P_j 
                if (receive_rdy(reseau[j], exp)) new_ready[j] = true;
            }
        }
        rdy_2send = new_ready; // P1 y P3 reciben al menos t+1 mensajes Ready y quedan listos; rdy_2send = [T, F, T, F, F]
    } // Hace falta una segunda ronda para que rdy_2send = [T, T, T, T, T]
    std::cout << " " << (2*t+1) << "/" << n 
          << " participants have reached at least 2t+1 Ready messages; secret reconstruction is possible.\n";

    // Reconstruye los polinomios phi(X,x_j), que no contienen directamente el secreto en el origen 
    sep();
    std::cout << "Participants P_i reconstruct phi_i(X) = phi(X, x_i) and r_i(X) = r(X, x_i)\n";
    std::cout << " from subshares phi(x_j, x_i) received from each P_j\n";
    bool good_sub = true;
    std::vector<GetShareResult> final_sub(n); // Contendrá phi_i(X) y r_i(X)
    for (size_t i = 0; i < n; ++i) {
        auto sub = reconstruct(reseau[i]); // P_i interpola los valores phi(x_j, x_i) válidos de V
        if (!sub) { // Si reconstruct = false, P_i no tiene suficientes subshares válidos en V
            good_sub = false;
        } else {
            final_sub[i] = sub.value(); // En caso contrario, P_i reconstruyó phi(X, x_i) y r(X, x_i)
            std::cout << " P_" << i+1 << " reconstructed polynomials phi_" << i+1 << "(X) and r_" << i+1 << "(X)\n";
        }
    }

    // Comprobación adicional: también se puede recuperar phi(0,0) a partir de los phi(X,x_i) de GetShare
    sep();
    std::cout << "Checking that secret phi(0,0) is recovered from the phi(X,x_i) values produced by GetShare:\n";
    std::vector<FieldElement> phi_xi_0(t + 1); // vector con phi(X, x_i) para i=1,...,t+1
    for (size_t i = 0; i <= t; ++i) {
        FieldElement eval_0;
        NTL::eval(eval_0, final_sub[i].phi_share, zero); // evaluación de phi(0, x_i) 
        phi_xi_0[i] = eval_0;
    }
    // Interpolación de Lagrange sobre phi(0,x_i)
    FieldElement s_found = lagrange_eval(pts_t1, phi_xi_0, zero);
    bool good_s = (s_found == secret); // Compara el secreto original con el recuperado
    
    std::cout << "Original secret: " << fe_to_long(secret) << "\n";
    std::cout << "Secret reconstructed by the network: " << fe_to_long(s_found) << "\n";


    bool all_verif = prox_verif && paths_verified && good_secret && D_hon && good_sub && good_s;
    
    sep();
    if (all_verif) {
        std::cout << " Deal and GetShare completed successfully\n";
    } else {
        std::cout << " Error: a protocol step failed.\n";
    }

    sep();
    clock_t end_time = clock();
    double time_dif = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    std::cout << "Execution time: " << time_dif << " seconds\n";
    std::cout << "Number of hashes computed: " << hash_count << "\n";

    return all_verif ? 0 : 1;
}
