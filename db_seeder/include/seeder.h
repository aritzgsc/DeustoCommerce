#ifndef INCLUDE_SEEDER_H_
#define INCLUDE_SEEDER_H_

#include "sqlite3.h"

int seedPaises(sqlite3* db);
int seedCiudades(sqlite3* db);

int seedAlmacenes(sqlite3* db);

int seedCategorias(sqlite3* db);
int seedProductos(sqlite3* db);

int seedStock(sqlite3* db);

#endif /* INCLUDE_SEEDER_H_ */
