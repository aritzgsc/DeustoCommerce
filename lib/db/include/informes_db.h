#ifndef DB_INCLUDE_INFORMES_DB_H_
#define DB_INCLUDE_INFORMES_DB_H_

#include "sqlite3.h"
#include <time.h>

// STRUCTS

typedef struct {

    int idProd;
    char* nombreProd;
    char* nombreCat;
    int totalVendido;   // suma de CANT en PRODUCTOS_PEDIDO
    double totalIngresos;  // suma de CANT * PRECIO_COMPRA

} ProductoVenta;

typedef struct {

    int idProd;
    char* nombreProd;
    char* nombreCat;
    int idAlm;
    char* nombreAlm;
    char* variante;
    int cantStock;      // unidades paradas en almacén

} ProductoDeadStock;

typedef struct {

    char fecha[16];      // "YYYY-MM-DD"
    char tipo[32];       // "INGRESO" o "GASTO"
    char id[32];
    char concepto[256];
    double importe;

} BalanceItem;

// TOP VENTAS

// Devuelve el ranking de productos más vendidos.
// limit = número máximo de resultados.
// *n = número real de resultados. Liberar con liberarTopVentas().
ProductoVenta* getTopVentas(sqlite3* db, int limit, int* n);
void liberarTopVentas(ProductoVenta* items, int n);

// DEAD STOCK

// Devuelve productos con stock en almacén pero sin ninguna venta registrada.
// idAlm = -1 -> todos los almacenes.
// *n = número de resultados. Liberar con liberarDeadStock().
ProductoDeadStock* getDeadStock(sqlite3* db, int idAlm, int* n);
void liberarDeadStock(ProductoDeadStock* items, int n);

// Exporta el dead stock a un fichero CSV.
// Devuelve 0 si OK.
int exportarDeadStockCsv(ProductoDeadStock* items, int n, char* rutaSalida);

// BALANCE FINANCIERO

// Lee reg_financiero.csv y devuelve los registros en el rango de fechas.
// fIni / fFin: timestamps. Pasar 0 para sin límite.
// *n = número de registros. Liberar con free().
BalanceItem* getBalance(char* rutaCsv, time_t fIni, time_t fFin, int* n);

// Calcula el total de ingresos y gastos de un array de BalanceItem.
void calcularTotalesBalance(BalanceItem* items, int n, double* totalIngresos, double* totalGastos);

#endif /* DB_INCLUDE_INFORMES_DB_H_ */
