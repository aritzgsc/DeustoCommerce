#ifndef DB_INCLUDE_PRODUCTOS_DB_H_
#define DB_INCLUDE_PRODUCTOS_DB_H_

#include "sqlite3.h"
#include "estructuras.h"

// FILTROS DE BÚSQUEDA

typedef struct {
    char nombre[256];     	// vacío = sin filtro
    int idCategoria;     	// -1 = sin filtro
    double precioMin;       // -1 = sin filtro
    double precioMax;       // -1 = sin filtro
    int idAlm;				// -1 = sin filtro
} FiltrosProducto;

// Inicializa un FiltrosProducto sin ningún filtro activo
FiltrosProducto filtrosVacios();

void liberarProducto(Producto* p);

// CONSULTAS

void getEstadoSistema(sqlite3* db, int* catalogo, int* redLogis, int* pedidosPend, int* productosSinStock, double* ocupacionRed);

// Devuelve array de productos paginado según filtros.
// *total se rellena con el número total de resultados (para paginación).
// El array devuelto debe liberarse con free().
Producto* buscarProductos(sqlite3* db, FiltrosProducto f, int pagina, int* total);

// Devuelve si el producto está o no en el almacén especificado
int isProductoInAlmacen(sqlite3* db, int idProd, int idAlm);

// Devuelve un producto por ID o NULL si no existe.
// Debe liberarse con free().
Producto* getProductoPorId(sqlite3* db, int idProd);

// Devuelve el stock total de un producto en todos los almacenes (idAlm = -1) o en un almacen concreto
int getStockProducto(sqlite3* db, int idProd, int idAlm);

// ADMIN

// Inserta un nuevo producto. Devuelve el ID generado o -1 si falla.
int crearProducto(sqlite3* db, Producto p);

// Actualiza nombre, descripción, precio y descuento. Devuelve 0 si OK.
int editarProducto(sqlite3* db, Producto p);

// Elimina el producto y todoo su stock. Devuelve 0 si OK.
int eliminarProducto(sqlite3* db, int idProd, int idAlm);

// CATEGORÍAS

// Devuelve array de todas las categorías.
// *n se rellena con el número de categorías.
// El array devuelto debe liberarse con free().
Categoria* getCategorias(sqlite3* db, int* n);

// VARIANTES

// Actualiza las variantes disponibles en el almacén de la categoría producto.
void filtrarVariantesConStockEnAlm(sqlite3* db, Producto* p, int idAlm);

#endif /* DB_INCLUDE_PRODUCTOS_DB_H_ */
