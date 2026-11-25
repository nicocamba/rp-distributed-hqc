#include "polynomial.hpp"
#include <algorithm>

/*
 * Evalúa punto a punto el polinomio bivariado P(X,Y).
 * Aplica el método de Horner respecto a la variable X.
 * Antes, evalúa cada coeficiente c_i(Y) en y.
 */
FieldElement BivPoly::evaluate(const FieldElement& x, const FieldElement& y) const {
    FieldElement result; 
    for (long i = static_cast<long>(coeffs_X.size()) - 1; i >= 0; --i) {
        FieldElement c_i_at_y;
        NTL::eval(c_i_at_y, coeffs_X[static_cast<size_t>(i)], y);
        result = result * x + c_i_at_y;
    }
    return result;
}

/*
 * Genera aleatoriamente el polinomio de máscara r(X,Y), de 
 * grado total <= t
 * Cada coeficiente de X^i es un polinomio univariado de grado 
 * <= t-i en Y, cuyos términos se eligen uniformemente 
 * al azar.
 */
BivPoly rand_biv_poly(size_t t) {
    BivPoly P;
    P.coeffs_X.resize(t + 1);
    for (size_t i = 0; i <= t; ++i) {
        long degree_bound = static_cast<long>(t - i);
        for (long j = 0; j <= degree_bound; ++j) {
            NTL::SetCoeff(P.coeffs_X[i], j, random_element());
        }
    }
    return P;
}

/*
 * Genera el polinomio secreto phi(X,Y).
 * Primero genera un polinomio aleatorio y después fija 
 * su término constante a_00 al valor del secreto.
 */
BivPoly rand_biv_poly(size_t t, const FieldElement& a_00) {
    BivPoly P = rand_biv_poly(t);
    NTL::SetCoeff(P.coeffs_X[0], 0, a_00);
    return P;
}

/*
 * Suma dos polinomios bivariados, sumando
 * término a término sus polinomios univariados. 
 * Ajusta automáticamente el vector resultado a la dimensión 
 * máxima y rellena con el polinomio cero para cubrir 
 * las diferencias de grado.
 */
BivPoly add(const BivPoly& a, const BivPoly& b) {
    BivPoly result;
    size_t max_size = std::max(a.coeffs_X.size(), b.coeffs_X.size());
    result.coeffs_X.resize(max_size);
    Uni_Poly zero; 
    for (size_t i = 0; i < max_size; ++i) {
        const Uni_Poly& ai = (i < a.coeffs_X.size()) ? a.coeffs_X[i] : zero;
        const Uni_Poly& bi = (i < b.coeffs_X.size()) ? b.coeffs_X[i] : zero;
        result.coeffs_X[i] = ai + bi;
    }
    return result;
}

/*
 * Multiplica un polinomio bivariado por un escalar,
 * multiplicando por él cada coeficiente de X^i.
 */
BivPoly scalar_mul(const BivPoly& p, const FieldElement& scalar) {
    BivPoly result;
    result.coeffs_X.resize(p.coeffs_X.size());
    for (size_t i = 0; i < p.coeffs_X.size(); ++i) {
        NTL::mul(result.coeffs_X[i], p.coeffs_X[i], scalar);
    }
    return result;
}


/*
 * Calcula P(x_i,Y) mediante el método de Horner.
 */
Uni_Poly eval_X(const BivPoly& P, const FieldElement& x_i) {
    Uni_Poly result; 
    for (long k = static_cast<long>(P.coeffs_X.size()) - 1; k >= 0; --k) {
        Uni_Poly scaled;
        NTL::mul(scaled, result, x_i);
        result = scaled + P.coeffs_X[static_cast<size_t>(k)];
    }
    return result;
}

/*
 * Calcula P(X,y_i) evaluando sus coeficientes.
 */
Uni_Poly eval_Y(const BivPoly& P, const FieldElement& y_i) {
    Uni_Poly result;
    for (size_t k = 0; k < P.coeffs_X.size(); ++k) {
        FieldElement scalar_coeff;
        NTL::eval(scalar_coeff, P.coeffs_X[k], y_i);
        NTL::SetCoeff(result, static_cast<long>(k), scalar_coeff);
    }
    return result;
}
