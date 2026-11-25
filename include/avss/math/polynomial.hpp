#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

#include <vector>
#include <cstddef>
#include <NTL/ZZ_pX.h>
#include "avss/math/field.hpp"

/*
 * Alias para el anillo de polinomios univariados F_p[X].
 */
using Uni_Poly = NTL::ZZ_pX;

/*
 * Representa el polinomio bivariado P(X,Y), de grado total como máximo t.
 * Lo representa como un vector de polinomios univariados:
 * P(X,Y) = suma desde i=0 hasta t de X^i*c_i(Y), con grado(c_i) <= t-i.
 * Se interpreta como un polinomio en X cuyos t+1 coeficientes c_i(Y) son
 * polinomios univariados en Y.
 */
struct BivPoly {
    std::vector<Uni_Poly> coeffs_X;

    FieldElement evaluate(const FieldElement& x, const FieldElement& y) const;
};

/*
 * Genera aleatoriamente el polinomio secreto phi(X,Y). 
 * Fija el término constante a_00 al valor del secreto s.
 */
BivPoly rand_biv_poly(size_t t, const FieldElement& a_00);

/*
 * Genera aleatoriamente el polinomio de máscara r(X,Y), que servirá 
 * para ocultar el polinomio secreto phi(X,Y).
 */
BivPoly rand_biv_poly(size_t t); 

// Suma de polinomios y multiplicación por un escalar 
BivPoly add(const BivPoly& a, const BivPoly& b);
BivPoly scalar_mul(const BivPoly& p, const FieldElement& scalar);

/*
 * Evaluaciones parciales: P(x_i,Y) y P(X,y_i)
 */
Uni_Poly eval_X(const BivPoly& P, const FieldElement& x_i);
Uni_Poly eval_Y(const BivPoly& P, const FieldElement& y_i);

#endif // POLYNOMIAL_H
