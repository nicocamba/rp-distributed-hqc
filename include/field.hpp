#ifndef FIELD_H
#define FIELD_H

#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>

/*
 * Alias del cuerpo finito F_p basado en NTL::ZZ_p, que proporciona
 * aritmética modular y los operadores +, -, *, ==, != e inverso 1/a.
 */
using FieldElement = NTL::ZZ_p;

/*
 * Configura el módulo p del cuerpo finito F_p.
 * Debe elegirse según el nivel de seguridad requerido y la necesidad
 * de usar una FFT en futuras pruebas de proximidad.
 * Se debe llamar una sola vez antes de realizar operaciones
 * algebraicas con FieldElement.
 */
void init_field(const NTL::ZZ& prime);

/* 
 * Genera un elemento aleatorio uniforme de F_p.
 * El campo debe haberse configurado previamente mediante init_field.
 */
FieldElement random_element();

#endif // FIELD_H
