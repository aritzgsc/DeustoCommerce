#ifndef DB_INCLUDE_ALMACENES_DB_H_
#define DB_INCLUDE_ALMACENES_DB_H_

#include "sqlite3.h"
#include "estructuras.h"

// CONSULTAS DE ALMACÉN

// Devuelve todos los almacenes con su ocupación calculada.
// *n = número de almacenes.
Almacen* getAlmacenes(sqlite3* db, int* n);

// Devuelve un almacén por ID o NULL si no existe.
Almacen* getAlmacenPorId(sqlite3* db, int idAlm);

void liberarAlmacen(Almacen* a);
void liberarAlmacenes(Almacen* almacenes, int nAlm);

// Devuelve la ocupación actual (suma de CANT) de un almacén.
int getOcupacionAlmacen(sqlite3* db, int idAlm);

// Devuelve el stock de un producto específico en un almacén específico
int getStockProdAlm(sqlite3* db, int idAlm, int idProd, char* variante);

// Devuelve el stock de un almacén paginado.
// *total = número total de filas de stock.
StockProd* getStockAlmacen(sqlite3* db, int idAlm, int pagina, int* total);

// Libera un array de StockItem.
void liberarStock(StockProd* prods, int n);

// Actualiza el estado del stock a disponible.
int actualizarEstadoStock(sqlite3* db, int idAlm, int idProd, const char* variante, int cant);

// ADMIN

// Crea un almacén nuevo. Devuelve el ID generado o -1 si falla.
int crearAlmacen(sqlite3* db, Almacen a);

// Elimina un almacén y reubica su stock en el almacén más cercano.
// Devuelve 0 si OK, -1 si falla.
int eliminarAlmacen(sqlite3* db, int idAlm, time_t timestampEjecucion);

// GESTIÓN DE STOCK

// Añade cant unidades de (idProd, variante) al almacén.
// Si ya existe la fila suma, si no la crea.
// Devuelve 0 si OK.
int addStock(sqlite3* db, int idAlm, int idProd, char* variante, int cant, time_t timestampEjecucion);

// Mueve cant unidades de (idProd, variante) de origen a destino.
// Devuelve 0 si OK, -1 si no hay suficiente stock.
int moverStock(sqlite3* db, int idAlmOrigen, int idAlmDestino, int idProd, char* variante, int cant, time_t timestampEjecucion);

// Ejecuta el restock automático de un almacén:
// llena hasta el ~80% de capacidad repartiendo equitativamente.
// Devuelve unidades añadidas o -1 si falla.
int restock(sqlite3* db, int idAlm, double* costeReal, time_t timestampEjecucion);

#endif /* DB_INCLUDE_ALMACENES_DB_H_ */
