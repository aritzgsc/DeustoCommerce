#include "almacenes_db.h"
#include "productos_db.h"
#include "logistica.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#define ITEMS_POR_PAGINA 15

// AUXILIARES INTERNAS

// Rellena un Almacen desde un pstmt posicionado en una fila.

static Almacen almacenDB(sqlite3_stmt* stmt) {

    Almacen a = {0};
    a.id = sqlite3_column_int(stmt, 0);
    a.nombre = strdup((char*)sqlite3_column_text(stmt, 1));
    a.capacidad = sqlite3_column_int(stmt, 2);
    a.ubicacion.id = sqlite3_column_int(stmt, 3);
    a.ubicacion.direccion = strdup((char*)sqlite3_column_text(stmt, 4));
    a.ubicacion.latitud = sqlite3_column_double(stmt, 5);
    a.ubicacion.longitud = sqlite3_column_double(stmt, 6);
    return a;

}

static char* sqlAlmacenBase =
    "SELECT A.ID_ALM, A.NOM_ALM, A.CAP_MAX, "
    "       U.ID_UB, U.DIR_UB, U.LAT_UB, U.LON_UB "
    "FROM ALMACEN A, UBICACION U "
    "WHERE A.ID_UB = U.ID_UB";

// CONSULTAS

Almacen* getAlmacenes(sqlite3* db, int* n) {

    if (!db || !n) return NULL;

    // Contamos primero

    sqlite3_stmt* pstmtCount;
    char* sqlCount = "SELECT COUNT(*) FROM ALMACEN";
    if (sqlite3_prepare_v2(db, sqlCount, -1, &pstmtCount, NULL) != SQLITE_OK) return NULL;

    if (sqlite3_step(pstmtCount) == SQLITE_ROW) *n = sqlite3_column_int(pstmtCount, 0);

    sqlite3_finalize(pstmtCount);

    if (*n == 0) return NULL;

    sqlite3_stmt* pstmt;
    char sql[1024];
    snprintf(sql, sizeof(sql), "%s ORDER BY A.ID_ALM ASC", sqlAlmacenBase);

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    Almacen* almacenes = malloc(sizeof(Almacen) * (*n));
    int i = 0;
    while (sqlite3_step(pstmt) == SQLITE_ROW && i < *n) almacenes[i++] = almacenDB(pstmt);

    sqlite3_finalize(pstmt);
    return almacenes;

}

Almacen* getAlmacenPorId(sqlite3* db, int idAlm) {

	if (!db) return NULL;

    char sql[512];
    snprintf(sql, sizeof(sql), "%s AND A.ID_ALM = ?", sqlAlmacenBase);

    sqlite3_stmt* pstmt;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    sqlite3_bind_int(pstmt, 1, idAlm);

    Almacen* a = NULL;
    if (sqlite3_step(pstmt) == SQLITE_ROW) {

        a = malloc(sizeof(Almacen));
        *a = almacenDB(pstmt);

    }

    sqlite3_finalize(pstmt);
    return a;

}

int getOcupacionAlmacen(sqlite3* db, int idAlm) {

	if (!db) return 0;

    sqlite3_stmt* pstmt;
    char sql[] = "SELECT COALESCE(SUM(CANT), 0) FROM STOCK_ALMACEN "
                 "WHERE ID_ALM = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return 0;

    sqlite3_bind_int(pstmt, 1, idAlm);

    int ocupacion = 0;
    if (sqlite3_step(pstmt) == SQLITE_ROW) ocupacion = sqlite3_column_int(pstmt, 0);

    sqlite3_finalize(pstmt);
    return ocupacion;

}

StockProd* getStockAlmacen(sqlite3* db, int idAlm, int pagina, int* total) {

	if (!db || !total) return NULL;

    // COUNT

    sqlite3_stmt* pstmtCount;
    char sqlCount[] =
        "SELECT COUNT(*) FROM STOCK_ALMACEN "
        "WHERE ID_ALM = ?";

    *total = 0;

    if (sqlite3_prepare_v2(db, sqlCount, -1, &pstmtCount, NULL) == SQLITE_OK) {

        sqlite3_bind_int(pstmtCount, 1, idAlm);
        if (sqlite3_step(pstmtCount) == SQLITE_ROW) *total = sqlite3_column_int(pstmtCount, 0);
        sqlite3_finalize(pstmtCount);

    }

    if (*total == 0) return NULL;

    // SELECT paginado
    int offset = (pagina - 1) * ITEMS_POR_PAGINA;
    char sql[1024];

    snprintf(sql, sizeof(sql),
        "SELECT SA.ID_PR, P.NOM_PR, SA.VARIANTE, SA.CANT, SA.DISPONIBLE "
        "FROM STOCK_ALMACEN SA, PRODUCTO P "
        "WHERE SA.ID_PR = P.ID_PR AND SA.ID_ALM = ? "
        "ORDER BY P.ID_PR ASC, SA.VARIANTE ASC "
        "LIMIT %d OFFSET %d",
        ITEMS_POR_PAGINA, offset);

    sqlite3_stmt* pstmt;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    sqlite3_bind_int(pstmt, 1, idAlm);

    StockProd* prods = malloc(sizeof(StockProd) * ITEMS_POR_PAGINA);
    int n = 0;

    while (sqlite3_step(pstmt) == SQLITE_ROW && n < ITEMS_POR_PAGINA) {

    	prods[n].producto.id = sqlite3_column_int(pstmt, 0);
    	prods[n].producto.nombre = strdup((char*)sqlite3_column_text(pstmt, 1));
    	prods[n].producto.variante = strdup((char*)sqlite3_column_text(pstmt, 2));
    	prods[n].cantidad = sqlite3_column_int(pstmt, 3);
    	prods[n].disponible = sqlite3_column_int(pstmt, 4);
        n++;

    }

    sqlite3_finalize(pstmt);
    return prods;

}

int getStockProdAlm(sqlite3* db, int idAlm, int idProd, char* variante) {

	sqlite3_stmt* pstmt;
	char* sql = "SELECT CANT FROM STOCK_ALMACEN WHERE ID_ALM = ? AND ID_PR = ? AND VARIANTE = ? AND DISPONIBLE = 1";
	if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return 0;

	sqlite3_bind_int(pstmt, 1, idAlm);
	sqlite3_bind_int(pstmt, 2, idProd);
	sqlite3_bind_text(pstmt, 3, variante, -1, SQLITE_STATIC);

	int ret = 0;
	if (sqlite3_step(pstmt) == SQLITE_ROW) ret = sqlite3_column_int(pstmt, 0);

	sqlite3_finalize(pstmt);

	return ret;

}

void liberarStock(StockProd* prods, int n) {

    if (!prods) return;

    for (int i = 0; i < n; i++) {

        free(prods[i].producto.nombre);
        free(prods[i].producto.variante);

    }

    free(prods);

}

// ADMIN

int crearAlmacen(sqlite3* db, Almacen a) {

    if (!db) return -1;

    // Primero comprobamos que existe el país y la ciudad y actualizamos la ubicación con sus IDs 

    char idPais[4];
    sqlite3_stmt* pstmtComprobPais;
    char* sqlComprobPais = "SELECT ID_PA FROM PAIS WHERE NOM_PA = ?";

    if (sqlite3_prepare_v2(db, sqlComprobPais, -1, &pstmtComprobPais, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmtComprobPais, 1, a.ubicacion.ciudad.pais.nombre, -1, SQLITE_STATIC);

    if (sqlite3_step(pstmtComprobPais) == SQLITE_ROW) {

    	strcpy(idPais, (char*)sqlite3_column_text(pstmtComprobPais, 0));

    } else {

    	sqlite3_stmt* pstmtInsertPais;
    	char* sqlInsertPais = "INSERT INTO PAIS (ID_PA, NOM_PA) VALUES (?, ?)";

    	if (sqlite3_prepare_v2(db, sqlInsertPais, -1, &pstmtInsertPais, NULL) != SQLITE_OK) return -1;

    	char codigo[4] = "";

    	for (int i = 0 ; i < 3 ; i++) codigo[i] = toupper(a.ubicacion.ciudad.pais.nombre[i]);
    	codigo[3] = '\0';

    	sqlite3_bind_text(pstmtInsertPais, 1, codigo, -1, SQLITE_STATIC);
    	sqlite3_bind_text(pstmtInsertPais, 2, a.ubicacion.ciudad.pais.nombre, -1, SQLITE_STATIC);

    	if (sqlite3_step(pstmtInsertPais) != SQLITE_DONE) {

    	    sqlite3_finalize(pstmtInsertPais);
    	    return -1;

    	}

    	strcpy(idPais, codigo);
    	sqlite3_finalize(pstmtInsertPais);

    }

   	sqlite3_finalize(pstmtComprobPais);

    int idCiudad;
    sqlite3_stmt* pstmtComprobCiudad;
    char* sqlComprobCiudad = "SELECT ID_CIU FROM CIUDAD WHERE NOM_CIU = ?";

    if (sqlite3_prepare_v2(db, sqlComprobCiudad, -1, &pstmtComprobCiudad, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmtComprobCiudad, 1, a.ubicacion.ciudad.nombre, -1, SQLITE_STATIC);

    if (sqlite3_step(pstmtComprobCiudad) == SQLITE_ROW) {

    	idCiudad = sqlite3_column_int(pstmtComprobCiudad, 0);

    } else {

    	sqlite3_stmt* pstmtInsertCiudad;
    	char* sqlInsertCiudad = "INSERT INTO CIUDAD (NOM_CIU, LAT_CIU, LON_CIU, POBLACION_CIU, ID_PA) VALUES (?, NULL, NULL, NULL, ?)";

    	if (sqlite3_prepare_v2(db, sqlInsertCiudad, -1, &pstmtInsertCiudad, NULL) != SQLITE_OK) return -1;

    	sqlite3_bind_text(pstmtInsertCiudad, 1, a.ubicacion.ciudad.nombre, -1, SQLITE_STATIC);
    	sqlite3_bind_text(pstmtInsertCiudad, 2, idPais, -1, SQLITE_STATIC);

    	if (sqlite3_step(pstmtInsertCiudad) != SQLITE_DONE) {

    	    sqlite3_finalize(pstmtInsertCiudad);
    	    return -1;

    	}

    	idCiudad = sqlite3_last_insert_rowid(db);

    }

    sqlite3_finalize(pstmtComprobCiudad);
    
    // Luego insertamos la ubicación
    sqlite3_stmt* pstmtUbi;
    char sqlUbi[] =
        "INSERT INTO UBICACION (DIR_UB, LAT_UB, LON_UB, ID_CIU) "
        "VALUES (?, ?, ?, ?)";

    if (sqlite3_prepare_v2(db, sqlUbi, -1, &pstmtUbi, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmtUbi, 1, a.ubicacion.direccion, -1, SQLITE_STATIC);
    sqlite3_bind_double(pstmtUbi, 2, a.ubicacion.latitud);
    sqlite3_bind_double(pstmtUbi, 3, a.ubicacion.longitud);
    sqlite3_bind_int(pstmtUbi, 4, idCiudad);

    if (sqlite3_step(pstmtUbi) != SQLITE_DONE) {

        sqlite3_finalize(pstmtUbi);
        return -1;

    }

    int idUbi = (int)sqlite3_last_insert_rowid(db);
    sqlite3_finalize(pstmtUbi);

    // Luego el almacén
    sqlite3_stmt* stmtAlm;
    char sqlAlm[] = "INSERT INTO ALMACEN (NOM_ALM, CAP_MAX, ID_UB) VALUES (?, ?, ?)";

    if (sqlite3_prepare_v2(db, sqlAlm, -1, &stmtAlm, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmtAlm, 1, a.nombre, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmtAlm, 2, a.capacidad);
    sqlite3_bind_int(stmtAlm, 3, idUbi);

    int id = -1;
    if (sqlite3_step(stmtAlm) == SQLITE_DONE) id = (int)sqlite3_last_insert_rowid(db);

    sqlite3_finalize(stmtAlm);
    return id;

}

int eliminarAlmacen(sqlite3* db, int idAlm) {

    if (!db) return -1;

    // Obtenemos el almacén a eliminar para tener su ubicación
    Almacen* aElim = getAlmacenPorId(db, idAlm);
    if (!aElim) return -1;

    // Obtenemos todos los almacenes restantes para reubicar stock
    int nAlm = 0;
    Almacen* todos = getAlmacenes(db, &nAlm);

    // Reubicamos cada fila de stock al almacén más cercano
    sqlite3_stmt* stmtStock;
    char sqlStock[] =
        "SELECT ID_PR, VARIANTE, CANT FROM STOCK_ALMACEN "
        "WHERE ID_ALM = ? AND DISPONIBLE = 1";

    if (sqlite3_prepare_v2(db, sqlStock, -1, &stmtStock, NULL) == SQLITE_OK) {

    	sqlite3_bind_int(stmtStock, 1, idAlm);

        while (sqlite3_step(stmtStock) == SQLITE_ROW) {

            int idProd  = sqlite3_column_int(stmtStock, 0);
            char* variante = strdup((char*)sqlite3_column_text(stmtStock, 1));
            int cant    = sqlite3_column_int(stmtStock, 2);

            // Buscamos el almacén más cercano con espacio
            int idDestino = -1;
            double minDist   = 1e18;

            for (int i = 0; i < nAlm; i++) {

                if (todos[i].id == idAlm) continue;
                int ocup = getOcupacionAlmacen(db, todos[i].id);
                if (ocup + cant > todos[i].capacidad) continue;

                double dist = calcularDistancia(
                    aElim->ubicacion, todos[i].ubicacion);
                if (dist < minDist) {
                    minDist = dist;
                    idDestino = todos[i].id;
                }

            }

            if (idDestino != -1) addStock(db, idDestino, idProd, variante, cant);

            free(variante);

        }

        sqlite3_finalize(stmtStock);

    }

    // Liberamos memoria
    for (int i = 0; i < nAlm; i++) {

        free(todos[i].nombre);
        free(todos[i].ubicacion.direccion);

    }

    free(todos);
    free(aElim->nombre);
    free(aElim->ubicacion.direccion);
    free(aElim);

    // Borramos el almacén (CASCADE borra su stock y su ubicación)
    sqlite3_stmt* pstmt;
    char sql[] = "DELETE FROM ALMACEN WHERE ID_ALM = ?";
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int(pstmt, 1, idAlm);
    int res = sqlite3_step(pstmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(pstmt);
    return res;

}

// GESTIÓN DE STOCK

int addStock(sqlite3* db, int idAlm, int idProd, char* variante, int cant) {

    if (!db || cant < 0) return -1;

    // Intentamos UPDATE primero
    sqlite3_stmt* pstmtUpdate;
    char sqlUpdate[] =
        "UPDATE STOCK_ALMACEN SET CANT = CANT + ? "
        "WHERE ID_ALM = ? AND ID_PR = ? AND VARIANTE = ? AND DISPONIBLE = 1";

    if (sqlite3_prepare_v2(db, sqlUpdate, -1, &pstmtUpdate, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int(pstmtUpdate, 1, cant);
    sqlite3_bind_int(pstmtUpdate, 2, idAlm);
    sqlite3_bind_int(pstmtUpdate, 3, idProd);
    sqlite3_bind_text(pstmtUpdate, 4, variante, -1, SQLITE_STATIC);
    sqlite3_step(pstmtUpdate);

    int filas = sqlite3_changes(db);
    sqlite3_finalize(pstmtUpdate);

    if (filas > 0) return 0;

    // Si no existía, INSERT
    sqlite3_stmt* pstmtInsert;
    char sqlInsert[] =
        "INSERT INTO STOCK_ALMACEN (ID_PR, ID_ALM, VARIANTE, DISPONIBLE, CANT) "
        "VALUES (?, ?, ?, 1, ?)";

    if (sqlite3_prepare_v2(db, sqlInsert, -1, &pstmtInsert, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int(pstmtInsert,  1, idProd);
    sqlite3_bind_int(pstmtInsert,  2, idAlm);
    sqlite3_bind_text(pstmtInsert, 3, variante, -1, SQLITE_STATIC);
    sqlite3_bind_int(pstmtInsert,  4, cant);

    int res = sqlite3_step(pstmtInsert) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(pstmtInsert);
    return res;

}

int moverStock(sqlite3* db, int idAlmOrigen, int idAlmDestino, int idProd, char* variante, int cant) {

	if (!db || cant < 0) return -1;

    // Verificamos que hay suficiente stock en origen
    sqlite3_stmt* pstmtCheck;
    char sqlCheck[] =
        "SELECT CANT FROM STOCK_ALMACEN "
        "WHERE ID_ALM = ? AND ID_PR = ? AND VARIANTE = ? AND DISPONIBLE = 1";

    if (sqlite3_prepare_v2(db, sqlCheck, -1, &pstmtCheck, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int(pstmtCheck,  1, idAlmOrigen);
    sqlite3_bind_int(pstmtCheck,  2, idProd);
    sqlite3_bind_text(pstmtCheck, 3, variante, -1, SQLITE_STATIC);

    int stockActual = 0;
    if (sqlite3_step(pstmtCheck) == SQLITE_ROW) stockActual = sqlite3_column_int(pstmtCheck, 0);
    sqlite3_finalize(pstmtCheck);

    if (stockActual < cant) return -10;	// Stock insuficiente

    // Restamos en origen
    sqlite3_stmt* pstmtResta;
    char sqlResta[1024];

    if (cant == stockActual) strncpy(sqlResta, "DELETE FROM STOCK_ALMACEN WHERE ID_ALM = ? AND ID_PR = ? AND VARIANTE = ? AND DISPONIBLE = 1", sizeof(sqlResta));
	else strncpy(sqlResta, "UPDATE STOCK_ALMACEN SET CANT = CANT - ? WHERE ID_ALM = ? AND ID_PR = ? AND VARIANTE = ? AND DISPONIBLE = 1", sizeof(sqlResta));

    if (sqlite3_prepare_v2(db, sqlResta, -1, &pstmtResta, NULL) != SQLITE_OK) return -1;

    if (cant == stockActual) {

    	sqlite3_bind_int(pstmtResta,  1, idAlmOrigen);
    	sqlite3_bind_int(pstmtResta,  2, idProd);
    	sqlite3_bind_text(pstmtResta, 3, variante, -1, SQLITE_STATIC);

    } else {

		sqlite3_bind_int(pstmtResta,  1, cant);
		sqlite3_bind_int(pstmtResta,  2, idAlmOrigen);
		sqlite3_bind_int(pstmtResta,  3, idProd);
		sqlite3_bind_text(pstmtResta, 4, variante, -1, SQLITE_STATIC);

    }

    sqlite3_step(pstmtResta);
    sqlite3_finalize(pstmtResta);

    // Sumamos en destino
    return addStock(db, idAlmDestino, idProd, variante, cant);

}

int restock(sqlite3* db, int idAlm) {

	if (!db) return -1;

    Almacen* a = getAlmacenPorId(db, idAlm);
    if (!a) return -1;

    int ocupacion = getOcupacionAlmacen(db, idAlm);
    int objetivo = (int)(a->capacidad * 0.8);

    free(a->nombre);
    free(a->ubicacion.direccion);
    free(a);

    if (ocupacion >= objetivo) return 0;  // ya está al 80% o más

    int targetProdsDistintos = 1250 + (int) (((double) rand() / RAND_MAX * 100));

    sqlite3_stmt* pstmtCount;
    char sqlCount[] = "SELECT COUNT(DISTINCT ID_PR) FROM STOCK_ALMACEN WHERE ID_ALM = ?";

    if (sqlite3_prepare_v2(db, sqlCount, -1, &pstmtCount, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int(pstmtCount, 1, idAlm);

    int nProdsActuales = 0;
    if (sqlite3_step(pstmtCount) == SQLITE_ROW) nProdsActuales = sqlite3_column_int(pstmtCount, 0);

    sqlite3_finalize(pstmtCount);

    if (nProdsActuales < targetProdsDistintos) {

        int faltan = targetProdsDistintos - nProdsActuales;
        sqlite3_stmt* pstmtNuevos;

        char sqlNuevos[] =
            "SELECT P.ID_PR, C.VARIANTES_CAT FROM PRODUCTO P "
            "JOIN CATEGORIA C ON P.ID_CAT = C.ID_CAT "
            "WHERE P.ID_PR NOT IN (SELECT ID_PR FROM STOCK_ALMACEN WHERE ID_ALM = ?) "
            "ORDER BY RANDOM() LIMIT ?";

        if (sqlite3_prepare_v2(db, sqlNuevos, -1, &pstmtNuevos, NULL) == SQLITE_OK) {

            sqlite3_bind_int(pstmtNuevos, 1, idAlm);
            sqlite3_bind_int(pstmtNuevos, 2, faltan);

            typedef struct { int id; char variantes[512]; } ProdNuevo;

            ProdNuevo* nuevos = malloc(sizeof(ProdNuevo) * faltan);
            int nNuevos = 0;

            while (sqlite3_step(pstmtNuevos) == SQLITE_ROW && nNuevos < faltan) {

                nuevos[nNuevos].id = sqlite3_column_int(pstmtNuevos, 0);
                strncpy(nuevos[nNuevos].variantes, (char*)sqlite3_column_text(pstmtNuevos, 1), sizeof(nuevos[nNuevos].variantes) - 1);
                nNuevos++;

            }

            sqlite3_finalize(pstmtNuevos);

            sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

            for (int i = 0; i < nNuevos; i++) {

                char varRaw[512];
                strncpy(varRaw, nuevos[i].variantes, sizeof(varRaw) - 1);
                char* tok = strtok(varRaw, ",");
                while (tok) {

                    while (*tok == ' ') tok++;

                    int len = strlen(tok);

                    while (len > 0 && (tok[len-1] == '\r' || tok[len-1] == '\n')) tok[--len] = '\0';

                    addStock(db, idAlm, nuevos[i].id, tok, 0);
                    tok = strtok(NULL, ",");

                }

                nProdsActuales++;

            }

            sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
            free(nuevos);
        }
    }

    if (nProdsActuales == 0) return 0;
    int targetBasePorProd = objetivo / nProdsActuales;
    if (targetBasePorProd == 0) return 0;

    sqlite3_stmt* pstmt;
    char sql[] =
        "SELECT SA.ID_PR, SA.VARIANTE, SA.CANT, NV.N_VARIANTES "
        "FROM STOCK_ALMACEN SA "
        "JOIN (SELECT ID_PR, COUNT(*) AS N_VARIANTES "
        "      FROM STOCK_ALMACEN "
        "      WHERE ID_ALM = ? AND DISPONIBLE = 1 "
        "      GROUP BY ID_PR) NV ON SA.ID_PR = NV.ID_PR "
        "WHERE SA.ID_ALM = ? AND SA.DISPONIBLE = 1";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;
    sqlite3_bind_int(pstmt, 1, idAlm);
    sqlite3_bind_int(pstmt, 2, idAlm);

    int totalAnadido = 0;

    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    while (sqlite3_step(pstmt) == SQLITE_ROW) {

        int idProd = sqlite3_column_int(pstmt, 0);
        char* variante = strdup((char*)sqlite3_column_text(pstmt, 1));
        int cantActual = sqlite3_column_int(pstmt, 2);
        int nVariantes = sqlite3_column_int(pstmt, 3);

        int targetBasePorVariante = nVariantes > 1 ? (targetBasePorProd / nVariantes) + 1 : targetBasePorProd;

        int targetAleatorio = targetBasePorVariante * 0.8 + (int)(((double)rand() / RAND_MAX) * targetBasePorVariante * 0.4);
        int deficitVariante = targetAleatorio - cantActual;

        if (deficitVariante > 0) {
            addStock(db, idAlm, idProd, variante, deficitVariante);
            totalAnadido += deficitVariante;
        }

        free(variante);

    }

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    sqlite3_finalize(pstmt);
    return totalAnadido;

}
