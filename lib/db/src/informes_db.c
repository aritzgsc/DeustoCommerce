#include "informes_db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TOP VENTAS

ProductoVenta* getTopVentas(sqlite3* db, int limit, int* n) {

    if (!db || !n) return NULL;

    char sql[512];
    snprintf(sql, sizeof(sql),
        "SELECT P.ID_PR, P.NOM_PR, C.NOM_CAT, "
        "       SUM(PP.CANT) AS TOTAL_VENDIDO, "
        "       SUM(PP.CANT * PP.PRECIO_COMPRA) AS TOTAL_INGRESOS "
        "FROM PRODUCTOS_PEDIDO PP "
        "JOIN PRODUCTO P ON PP.ID_PR = P.ID_PR "
        "JOIN CATEGORIA C ON P.ID_CAT = C.ID_CAT "
        "GROUP BY P.ID_PR "
        "ORDER BY TOTAL_VENDIDO DESC "
        "LIMIT %d", limit);

    sqlite3_stmt* pstmt;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    ProductoVenta* items = malloc(sizeof(ProductoVenta) * limit);
    *n = 0;

    while (sqlite3_step(pstmt) == SQLITE_ROW && *n < limit) {

        ProductoVenta* v = &items[*n];
        v->idProd = sqlite3_column_int(pstmt, 0);
        v->nombreProd = strdup((char*)sqlite3_column_text(pstmt, 1));
        v->nombreCat = strdup((char*)sqlite3_column_text(pstmt, 2));
        v->totalVendido = sqlite3_column_int(pstmt, 3);
        v->totalIngresos = sqlite3_column_double(pstmt, 4);
        (*n)++;

    }

    sqlite3_finalize(pstmt);
    return items;

}

void liberarTopVentas(ProductoVenta* items, int n) {

    if (!items) return;
    for (int i = 0; i < n; i++) {

        free(items[i].nombreProd);
        free(items[i].nombreCat);

    }

    free(items);

}

// DEAD STOCK

ProductoDeadStock* getDeadStock(sqlite3* db, int idAlm, int* n) {

    if (!db || !n) return NULL;

    // Productos con stock en almacén pero que no aparecen en ningún pedido
    char sql[1024];

    const char* base_query =
            "SELECT SA.ID_PR, P.NOM_PR, C.NOM_CAT, "
            "       SA.ID_ALM, A.NOM_ALM, SA.VARIANTE, SA.CANT "
            "FROM STOCK_ALMACEN SA "
            "JOIN PRODUCTO P        ON SA.ID_PR  = P.ID_PR "
            "LEFT JOIN CATEGORIA C  ON P.ID_CAT  = C.ID_CAT "
            "JOIN ALMACEN A         ON SA.ID_ALM = A.ID_ALM "
            "WHERE SA.CANT > 0 "
            "AND NOT EXISTS ("
            "    SELECT 1 "
            "    FROM PRODUCTOS_PEDIDO PP "
            "    JOIN PEDIDO PED ON PP.ID_PED = PED.ID_PED "
            "    WHERE PP.ID_PR = SA.ID_PR "
            "    AND PED.F_ENV_PED >= date('now', '-3 month')"
            ") ";

    if (idAlm == -1) {
        snprintf(sql, sizeof(sql), "%s ORDER BY SA.CANT DESC", base_query);
    } else {
        snprintf(sql, sizeof(sql), "%s AND SA.ID_ALM = %d ORDER BY SA.CANT DESC", base_query, idAlm);
    }

    sqlite3_stmt* pstmt;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    // Contamos primero
    *n = 0;
    while (sqlite3_step(pstmt) == SQLITE_ROW) (*n)++;
    sqlite3_finalize(pstmt);

    if (*n == 0) return NULL;

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    ProductoDeadStock* items = malloc(sizeof(ProductoDeadStock) * (*n));
    int i = 0;

    while (sqlite3_step(pstmt) == SQLITE_ROW && i < *n) {

        items[i].idProd     = sqlite3_column_int(pstmt, 0);
        items[i].nombreProd = strdup((char*)sqlite3_column_text(pstmt, 1));
        items[i].nombreCat  = strdup((char*)sqlite3_column_text(pstmt, 2));
        items[i].idAlm      = sqlite3_column_int(pstmt, 3);
        items[i].nombreAlm  = strdup((char*)sqlite3_column_text(pstmt, 4));
        items[i].variante   = strdup((char*)sqlite3_column_text(pstmt, 5));
        items[i].cantStock  = sqlite3_column_int(pstmt, 6);
        i++;

    }

    sqlite3_finalize(pstmt);
    return items;

}

void liberarDeadStock(ProductoDeadStock* items, int n) {

    if (!items) return;
    for (int i = 0; i < n; i++) {

        free(items[i].nombreProd);
        free(items[i].nombreCat);
        free(items[i].nombreAlm);
        free(items[i].variante);

    }

    free(items);

}

int exportarDeadStockCsv(ProductoDeadStock* items, int n, char* rutaSalida) {

    if (!items || n == 0 || !rutaSalida) return -1;

    FILE* f = fopen(rutaSalida, "w");
    if (!f) return -1;

    // Cabecera
    fprintf(f, "ID_PR;NOMBRE;CATEGORIA;ID_ALM;ALMACEN;VARIANTE;CANT_STOCK\n");

    for (int i = 0; i < n; i++) fprintf(f, "%d;\"%s\";\"%s\";%d;\"%s\";\"%s\";%d\n", items[i].idProd, items[i].nombreProd, items[i].nombreCat, items[i].idAlm, items[i].nombreAlm, items[i].variante, items[i].cantStock);

    fclose(f);
    return 0;

}

// BALANCE

BalanceItem* getBalance(char* rutaCsv, time_t fIni, time_t fFin, int* n) {

	if (!rutaCsv || !n) return NULL;

    FILE* f = fopen(rutaCsv, "r");
    if (!f) return NULL;

    char linea[512];
    *n = 0;

    // Saltamos cabecera
    fgets(linea, sizeof(linea), f);

    // Primera pasada: contamos líneas
    while (fgets(linea, sizeof(linea), f) != NULL) {
        char fecha[32] = {0};
        sscanf(linea, "%31[^;]", fecha);

        struct tm tm = {0};
        if (sscanf(fecha, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {
            tm.tm_year -= 1900;
            tm.tm_mon  -= 1;
            time_t t = mktime(&tm);
            if ((fIni == 0 || t >= fIni) && (fFin == 0 || t <= fFin)) (*n)++;
        }
    }

    if (*n == 0) { fclose(f); return NULL; }

    // Segunda pasada: rellenamos
    rewind(f);
    fgets(linea, sizeof(linea), f); // saltamos cabecera

    BalanceItem* items = calloc(*n, sizeof(BalanceItem));
    int i = 0;

    while (fgets(linea, sizeof(linea), f) != NULL && i < *n) {
        char tmp[512];
        strncpy(tmp, linea, sizeof(tmp) - 1);
        tmp[511] = '\0';

        // Limpiamos los saltos de línea finales (\n o \r)
        tmp[strcspn(tmp, "\r\n")] = 0;

        // Extraemos campos sin necesidad de crear arrays dinámicos (mallocs) extra
        char* c_fecha = strtok(tmp, ";");
        char* c_tipo = strtok(NULL, ";");
        char* c_concepto = strtok(NULL, ";");
        char* c_importe = strtok(NULL, ";");

        if (!c_fecha || !c_tipo || !c_concepto || !c_importe) continue;

        struct tm tm = {0};
        if (sscanf(c_fecha, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {

            tm.tm_year -= 1900;
            tm.tm_mon  -= 1;
            time_t t = mktime(&tm);

            if ((fIni == 0 || t >= fIni) && (fFin == 0 || t <= fFin)) {
                // Copiamos solo los 10 primeros chars (YYYY-MM-DD), ignorando las horas
                // Como usamos calloc, el carácter [10] ya es un '\0'.
                strncpy(items[i].fecha, c_fecha, 10);

                strncpy(items[i].tipo, c_tipo, sizeof(items[i].tipo) - 1);
                strncpy(items[i].concepto, c_concepto, sizeof(items[i].concepto) - 1);
                items[i].importe = atof(c_importe);
                i++;
            }

        }

    }

    fclose(f);
    *n = i;
    return items;

}

void calcularTotalesBalance(BalanceItem* items, int n, double* totalIngresos, double* totalGastos) {

    *totalIngresos = 0;
    *totalGastos   = 0;
    for (int i = 0; i < n; i++) {

        if (strncmp(items[i].tipo, "INGRESO", 7) == 0) *totalIngresos += items[i].importe;
        else *totalGastos += items[i].importe;

    }

}
