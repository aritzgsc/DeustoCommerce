#ifndef UTIL_INCLUDE_LOGISTICA_H_
#define UTIL_INCLUDE_LOGISTICA_H_

#include "estructuras.h"

// DISTANCIA

// Calcula la distancia en km entre dos ubicaciones usando Haversine.
double calcularDistancia(Ubicacion u1, Ubicacion u2);

// LOGISTICA

// Calcula el coste en tiempo y dinero de un transporte de mercancia
void calcularCosteTrasvase(Ubicacion ubi1, Ubicacion ubi2, int cant, double* precio, time_t* duracion);

// Calcula el coste en tiempo y dinero de una compra de mercancía a externos
void calcularCosteCompraExterno(ProdCant* prods, int nProds, double* precio, time_t* duracion);

// Calcula el coste en tiempo y dinero de un restock automático de un almacén
void calcularCosteRestock(sqlite3* db, int idAlm, double* precio, time_t* duracion);

// Calcula el coste estimado de cerrar un almacén (reubicación de todoo su stock)
void calcularCosteCierreAlmacen(sqlite3* db, int idAlm, double* precio, time_t* duracion);

#endif /* UTIL_INCLUDE_LOGISTICA_H_ */
