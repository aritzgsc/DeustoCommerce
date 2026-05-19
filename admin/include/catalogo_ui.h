#ifndef INCLUDE_CATALOGO_UI_H_
#define INCLUDE_CATALOGO_UI_H_

#include <catalogo_db.h>
#include "sqlite3.h"

// Pantalla principal del catálogo (listado paginado sin filtros)
void pantallaCatalogo(sqlite3* db);

// Pantallas de selección (funciones auxiliares)
int seleccionarProducto(sqlite3* db, char* varianteSalida, int maxLen, int idAlm);
int seleccionarCategoria(sqlite3* db);
int seleccionarVariante(Categoria* cat, char* varianteSalida, int maxLen);

// Pantalla de búsqueda con filtros
// Mantiene estado de filtros entre llamadas
void pantallaBuscar(sqlite3* db);

// Detalle de un producto
// idAlm = -1 -> muestra stock global, idAlm > 0 -> muestra stock de ese almacén
int pantallaVerProducto(sqlite3* db, int idProd, int idAlm);

// Formulario de nuevo producto
void pantallaNuevoProducto(sqlite3* db);

// Formulario de edición de producto
void pantallaEditarProducto(sqlite3* db, int idProd);

// Confirmación y borrado de producto
// idAlm = -1 -> borra en todos los almacenes, idAlm > 0 -> solo en ese almacén
void pantallaEliminarProducto(sqlite3* db, int idProd, int idAlm);

#endif /* INCLUDE_CATALOGO_UI_H_ */
