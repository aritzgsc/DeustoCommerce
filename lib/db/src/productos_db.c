#include "productos_db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ITEMS_POR_PAGINA 15

// AUXILIARES INTERNAS

// Rellena un Producto a partir de un sqlite3_stmt posicionado en una fila.
// Columnas esperadas: ID_PR, NOM_PR, DESCRIP_PR, PRECIO_PR, DESCTO_PR, ID_CAT, NOM_CAT

static Producto productoDB(sqlite3_stmt* stmt) {

    Producto p = {0};
    p.id               = sqlite3_column_int(stmt, 0);
    p.nombre           = strdup((char*)sqlite3_column_text(stmt, 1));
    p.descripcion      = sqlite3_column_text(stmt, 2) ? strdup((char*)sqlite3_column_text(stmt, 2)) : NULL;
    p.precio           = sqlite3_column_double(stmt, 3);
    p.descuento        = sqlite3_column_double(stmt, 4);
    p.categoria.id     = sqlite3_column_int(stmt, 5);
    p.categoria.nombre = strdup((char*)sqlite3_column_text(stmt, 6));

    // Parseamos las variantes de la categoría
    char* variantesRaw = (char*)sqlite3_column_text(stmt, 7);

    if (variantesRaw && strlen(variantesRaw) > 0 && strcmp(variantesRaw, "UNICA") != 0) {

        // Hacemos una copia para el split
        char* variantesCopy = strdup(variantesRaw);

        // Contamos cuántas hay (número de comas + 1)
        int n = 1;
        for (char* c = variantesCopy; *c; c++) if (*c == ',') n++;

        p.categoria.variantes  = malloc(sizeof(char*) * n);
        p.categoria.nVariantes = 0;

        // Split por coma
        char* token = strtok(variantesCopy, ",");
        while (token && p.categoria.nVariantes < n) {
            // Quitamos espacios al inicio
            while (*token == ' ') token++;
            p.categoria.variantes[p.categoria.nVariantes++] = strdup(token);
            token = strtok(NULL, ",");
        }

        free(variantesCopy);

    } else {

        // UNICA o sin variantes
        p.categoria.variantes    = malloc(sizeof(char*));
        p.categoria.variantes[0] = strdup("UNICA");
        p.categoria.nVariantes   = 1;

    }

    return p;

}

void liberarProducto(Producto* p) {

    if (!p) return;
    free(p->nombre);
    if (p->descripcion) free(p->descripcion);
    free(p->categoria.nombre);
    for (int i = 0; i < p->categoria.nVariantes; i++) free(p->categoria.variantes[i]);
    free(p->categoria.variantes);

}

// FILTROS

FiltrosProducto filtrosVacios() {

    FiltrosProducto f;
    memset(f.nombre, 0, sizeof(f.nombre));
    f.idCategoria = -1;
    f.precioMin   = -1;
    f.precioMax   = -1;
    f.idAlm       = -1;
    return f;

}

// CONSULTAS

void getEstadoSistema(sqlite3* db, int* catalogo, int* redLogis, int* pedidosPend, int* productosSinStock, double* ocupacionRed) {

	if (!db || !catalogo || !redLogis || !pedidosPend || !productosSinStock || !ocupacionRed) return;

	// Consultamos a la vista del estado del sistema que tenemos creada en la BD

	sqlite3_stmt* pstmt;

	char* sql = "SELECT * FROM ESTADO_SISTEMA";

	if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return;

	if (sqlite3_step(pstmt) == SQLITE_ROW) {

		// Actualizamos las variables con los datos de la vista

		*catalogo = sqlite3_column_int(pstmt, 0);
		*redLogis = sqlite3_column_int(pstmt, 1);
		*pedidosPend = sqlite3_column_int(pstmt, 2);
		*productosSinStock = sqlite3_column_int(pstmt, 3);
		*ocupacionRed = sqlite3_column_double(pstmt, 4);

	}

	sqlite3_finalize(pstmt);

}


Producto* buscarProductos(sqlite3* db, FiltrosProducto f, int pagina, int* total) {

    if (!db || !total) return NULL;

    // Construimos la WHERE dinámica

    char where[512] = "WHERE 1=1";

    if (strlen(f.nombre) > 0) {

        char tmp[300];
        snprintf(tmp, sizeof(tmp), " AND P.NOM_PR LIKE '%%%s%%'", f.nombre);
        strncat(where, tmp, sizeof(where) - strlen(where) - 1);

    }

    if (f.idCategoria != -1) {

        char tmp[64];
        snprintf(tmp, sizeof(tmp), " AND P.ID_CAT = %d", f.idCategoria);
        strncat(where, tmp, sizeof(where) - strlen(where) - 1);

    }

    if (f.precioMin >= 0) {

        char tmp[64];
        snprintf(tmp, sizeof(tmp), " AND P.PRECIO_PR >= %.2f", f.precioMin);
        strncat(where, tmp, sizeof(where) - strlen(where) - 1);

    }

    if (f.precioMax >= 0) {

        char tmp[64];
        snprintf(tmp, sizeof(tmp), " AND P.PRECIO_PR <= %.2f", f.precioMax);
        strncat(where, tmp, sizeof(where) - strlen(where) - 1);

    }

    if (f.idAlm != -1) {
        char tmp[128];
        snprintf(tmp, sizeof(tmp),
            " AND P.ID_PR IN "
            "(SELECT DISTINCT ID_PR FROM STOCK_ALMACEN "
            " WHERE ID_ALM = %d AND CANT > 0)", f.idAlm);
        strncat(where, tmp, sizeof(where) - strlen(where) - 1);
    }

    // COUNT total

    char sqlCount[1024];
    snprintf(sqlCount, sizeof(sqlCount),
        "SELECT COUNT(*) FROM PRODUCTO P "
        "JOIN CATEGORIA C ON P.ID_CAT = C.ID_CAT %s", where);

    sqlite3_stmt* pstmtCount;
    *total = 0;
    if (sqlite3_prepare_v2(db, sqlCount, -1, &pstmtCount, NULL) == SQLITE_OK) {
        if (sqlite3_step(pstmtCount) == SQLITE_ROW) *total = sqlite3_column_int(pstmtCount, 0);
        sqlite3_finalize(pstmtCount);
    }

    if (*total == 0) return NULL;

    // SELECT paginado

    int offset = (pagina - 1) * ITEMS_POR_PAGINA;
    if (offset < 0) offset = 0;
    char sql[1200];

    snprintf(sql, sizeof(sql),
        "SELECT P.ID_PR, P.NOM_PR, P.DESCRIP_PR, P.PRECIO_PR, P.DESCTO_PR, "
        "       C.ID_CAT, C.NOM_CAT "
        "FROM PRODUCTO P "
        "JOIN CATEGORIA C ON P.ID_CAT = C.ID_CAT "
        "%s "
        "ORDER BY P.ID_PR ASC "
        "LIMIT %d OFFSET %d",
        where, ITEMS_POR_PAGINA, offset);

    sqlite3_stmt* pstmtSelect;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmtSelect, NULL) != SQLITE_OK) return NULL;

    Producto* resultado = malloc(sizeof(Producto) * ITEMS_POR_PAGINA);
    int n = 0;

    while (sqlite3_step(pstmtSelect) == SQLITE_ROW && n < ITEMS_POR_PAGINA) resultado[n++] = productoDB(pstmtSelect);

    sqlite3_finalize(pstmtSelect);
    return resultado;

}

int isProductoInAlmacen(sqlite3* db, int idProd, int idAlm) {

	sqlite3_stmt* pstmt;
	char* sql = "SELECT COALESCE(SUM(CANT), 0) FROM STOCK_ALMACEN WHERE ID_PR = ? AND ID_ALM = ?";

	if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return 0;

	sqlite3_bind_int(pstmt, 1, idProd);
	sqlite3_bind_int(pstmt, 2, idAlm);

	int ret = 0;
	if (sqlite3_step(pstmt) == SQLITE_ROW) ret = sqlite3_column_int(pstmt, 0);

	sqlite3_finalize(pstmt);

	return ret;

}

Producto* getProductoPorId(sqlite3* db, int idProd) {

    if (!db) return NULL;

    char sql[] =
        "SELECT P.ID_PR, P.NOM_PR, P.DESCRIP_PR, P.PRECIO_PR, P.DESCTO_PR, "
        "       C.ID_CAT, C.NOM_CAT, C.VARIANTES_CAT "
        "FROM PRODUCTO P, CATEGORIA C "
        "WHERE P.ID_CAT = C.ID_CAT AND P.ID_PR = ?";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return NULL;

    sqlite3_bind_int(stmt, 1, idProd);

    Producto* p = NULL;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        p  = malloc(sizeof(Producto));
        *p = productoDB(stmt);
    }

    sqlite3_finalize(stmt);
    return p;

}

int getStockProducto(sqlite3* db, int idProd, int idAlm) {

	if (!db) return 0;

    sqlite3_stmt* pstmt;
    char sql[512] = "SELECT COALESCE(SUM(CANT), 0) FROM STOCK_ALMACEN "
                 "WHERE ID_PR = ? AND DISPONIBLE = 1";

    if (idAlm != -1) strcat(sql, " AND ID_ALM = ?");

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return 0;

    sqlite3_bind_int(pstmt, 1, idProd);
    sqlite3_bind_int(pstmt, 2, idAlm);

    int stock = 0;
    if (sqlite3_step(pstmt) == SQLITE_ROW) stock = sqlite3_column_int(pstmt, 0);

    sqlite3_finalize(pstmt);

    return stock;

}

// ADMIN

int crearProducto(sqlite3* db, Producto p) {

	if (!db) return -1;

    sqlite3_stmt* stmt;
    char sql[] =
        "INSERT INTO PRODUCTO (NOM_PR, DESCRIP_PR, PRECIO_PR, DESCTO_PR, ID_CAT) "
        "VALUES (?, ?, ?, ?, ?)";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, p.nombre,      -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, p.descripcion, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, p.precio);
    sqlite3_bind_double(stmt, 4, p.descuento);
    sqlite3_bind_int(stmt,   5, p.categoria.id);

    int id = -1;
    if (sqlite3_step(stmt) == SQLITE_DONE) id = (int)sqlite3_last_insert_rowid(db);

    sqlite3_finalize(stmt);
    return id;

}

int editarProducto(sqlite3* db, Producto p) {

	if (!db) return -1;

    sqlite3_stmt* stmt;
    char sql[] =
        "UPDATE PRODUCTO SET NOM_PR = ?, DESCRIP_PR = ?, "
        "PRECIO_PR = ?, DESCTO_PR = ?, ID_CAT = ? "
        "WHERE ID_PR = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt,   1, p.nombre,      -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt,   2, p.descripcion, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 3, p.precio);
    sqlite3_bind_double(stmt, 4, p.descuento);
    sqlite3_bind_int(stmt,    5, p.categoria.id);
    sqlite3_bind_int(stmt,    6, p.id);

    int result = sqlite3_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    return result;

}

int eliminarProducto(sqlite3* db, int idProd, int idAlm) {

	if (!db) return -1;

    sqlite3_stmt* pstmt;
    char sql[512];

    if (idAlm == -1) strncpy(sql, "DELETE FROM PRODUCTO WHERE ID_PR = ?", sizeof(sql));
    else strncpy(sql, "DELETE FROM STOCK_ALMACEN WHERE ID_PR = ? AND ID_ALM = ?", sizeof(sql));

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int(pstmt, 1, idProd);
    if (idAlm != -1) sqlite3_bind_int(pstmt, 2, idAlm);

    int result = sqlite3_step(pstmt) == SQLITE_DONE ? 0 : -1;

    sqlite3_finalize(pstmt);

    return result;

}

// CATEGORÍAS

Categoria* getCategorias(sqlite3* db, int* n) {

	if (!db || !n) return NULL;

    sqlite3_stmt* pstmt;
    char *sql = "SELECT ID_CAT, NOM_CAT FROM CATEGORIA ORDER BY ID_CAT ASC";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    // Primera pasada: contamos

    *n = 0;
    while (sqlite3_step(pstmt) == SQLITE_ROW) (*n)++;
    sqlite3_finalize(pstmt);

    if (*n == 0) return NULL;

    // Segunda pasada: rellenamos

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    Categoria* cats = malloc(sizeof(Categoria) * (*n));
    int i = 0;
    while (sqlite3_step(pstmt) == SQLITE_ROW && i < *n) {

        cats[i].id     = sqlite3_column_int(pstmt, 0);
        cats[i].nombre = strdup((char*)sqlite3_column_text(pstmt, 1));
        i++;

    }

    sqlite3_finalize(pstmt);
    return cats;

}

// VARIANTES

void filtrarVariantesConStockEnAlm(sqlite3* db, Producto* p, int idAlm) {

	sqlite3_stmt* pstmt;
	char *sql = "SELECT COALESCE(SUM(CANT), 0) FROM STOCK_ALMACEN WHERE ID_ALM = ? AND ID_PR = ? AND VARIANTE = ?";

	if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return;

	char** variantesConStockEnAlm = malloc(sizeof(char*) * p->categoria.nVariantes);
	int nVarsNuevo = 0;

	for (int i = 0 ; i < p->categoria.nVariantes ; i++) {

		sqlite3_bind_int(pstmt, 1, idAlm);
		sqlite3_bind_int(pstmt, 2, p->id);
		sqlite3_bind_text(pstmt, 3, p->categoria.variantes[i], -1, SQLITE_STATIC);

		sqlite3_step(pstmt);

		if (sqlite3_column_int(pstmt, 0) != 0) {
			variantesConStockEnAlm[nVarsNuevo] = strdup(p->categoria.variantes[i]);
			nVarsNuevo++;
		}

		sqlite3_reset(pstmt);

	}

	for (int i = 0; i < p->categoria.nVariantes; i++) free(p->categoria.variantes[i]);
	free(p->categoria.variantes);

	p->categoria.variantes = variantesConStockEnAlm;
	p->categoria.nVariantes = nVarsNuevo;

}
