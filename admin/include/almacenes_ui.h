#ifndef INCLUDE_ALMACENES_UI_H_
#define INCLUDE_ALMACENES_UI_H_

#include "sqlite3.h"

// Pantalla global de almacenes (tabla con todos los almacenes y ocupación)
void pantallaAlmacenes(sqlite3* db);

// Detalle de un almacén con su stock paginado
int pantallaVerAlmacen(sqlite3* db, int idAlm);

// Formulario de nuevo almacén
void pantallaNuevoAlmacen(sqlite3* db);

// Entrada manual de mercancía en un almacén
void pantallaAddStock(sqlite3* db, int idAlm);

// Trasvase de stock a otro almacén
void pantallaMoverStock(sqlite3* db, int idAlm);

// Restock automático hasta el ~80%
void pantallaRestock(sqlite3* db, int idAlm);

// Confirmación y cierre de almacén con reubicación de stock
void pantallaEliminarAlmacen(sqlite3* db, int idAlm);

#endif /* INCLUDE_ALMACENES_UI_H_ */
