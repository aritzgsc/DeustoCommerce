#ifndef DB_INCLUDE_RESENAS_DB_H_
#define DB_INCLUDE_RESENAS_DB_H_

#include "sqlite3.h"

// CONSULTAS

// Devuelve la valoración media ponderada de un producto.
// Usa el campo PESO de la reseña para ponderar.
// Devuelve -1 si no hay reseñas.
double getValoracionMedia(sqlite3* db, int idProd);

// Devuelve el número total de reseñas de un producto.
int getNumResenas(sqlite3* db, int idProd);

#endif /* DB_INCLUDE_RESENAS_DB_H_ */
