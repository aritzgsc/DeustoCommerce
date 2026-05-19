#include "almacenes_db.h"
#include "usuario_db.h"
#include "logistica.h"
#include "utils_ui.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// CONSULTAS CARRITO

ItemCarrito* getCarrito(sqlite3* db, const char* correo, int* n) {

    if (!db || !n) return NULL;
    *n = 0;

    // Contamos primero para saber si hay algo
    sqlite3_stmt* pstmtCount;
    const char* sqlCount = "SELECT COUNT(*) FROM CARRITO WHERE CORREO = ?";

    if (sqlite3_prepare_v2(db, sqlCount, -1, &pstmtCount, NULL) == SQLITE_OK) {

        sqlite3_bind_text(pstmtCount, 1, correo, -1, SQLITE_STATIC);
        if (sqlite3_step(pstmtCount) == SQLITE_ROW) *n = sqlite3_column_int(pstmtCount, 0);
        sqlite3_finalize(pstmtCount);

    }

    if (*n == 0) return NULL;

    const char* sql = "SELECT P.ID_PR, P.NOM_PR, C.VARIANTE, C.CANT, P.PRECIO_PR, P.DESCTO_PR "
					  "FROM CARRITO C "
					  "JOIN PRODUCTO P ON C.ID_PR = P.ID_PR "
					  "WHERE C.CORREO = ? "
					  "ORDER BY P.ID_PR ASC";

    sqlite3_stmt* pstmt;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) { *n = 0; return NULL; }

    sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);

    ItemCarrito* items = malloc(sizeof(ItemCarrito) * (*n));
    if (!items) { sqlite3_finalize(pstmt); *n = 0; return NULL; }

    int i = 0;
    while (sqlite3_step(pstmt) == SQLITE_ROW && i < *n) {

        ItemCarrito* item    = &items[i++];
        item->id             = sqlite3_column_int(pstmt, 0);
        item->nombreProducto = strdup((char*)sqlite3_column_text(pstmt, 1));
        item->variante       = strdup((char*)sqlite3_column_text(pstmt, 2));
        item->cantidad       = sqlite3_column_int(pstmt, 3);
        item->precioUnitario = sqlite3_column_double(pstmt, 4);
        item->descuento      = sqlite3_column_double(pstmt, 5);

    }

    *n = i;

    sqlite3_finalize(pstmt);
    return items;

}

void liberarCarrito(ItemCarrito* items, int n) {

    if (!items) return;
    for (int i = 0; i < n; i++) {
        free(items[i].nombreProducto);
        free(items[i].variante);
    }
    free(items);

}

int getItemsCarrito(sqlite3* db, const char* correo) {

    if (!db) return -1;

    sqlite3_stmt* pstmt;
    const char* sql = "SELECT COALESCE(SUM(CANT), 0) FROM CARRITO WHERE CORREO = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);

    int total = -1;
    if (sqlite3_step(pstmt) == SQLITE_ROW) total = sqlite3_column_int(pstmt, 0);

    sqlite3_finalize(pstmt);
    return total;

}

// ESCRITURA CARRITO

int addAlCarrito(sqlite3* db, const char* correo, int idProd, const char* variante, int cantidad) {

	if (!db) return -1;

    sqlite3_stmt* pstmt;

    const char* sqlUpdate = "UPDATE CARRITO SET CANT = CANT + ? WHERE CORREO = ? AND ID_PR = ? AND VARIANTE = ?";
    if (sqlite3_prepare_v2(db, sqlUpdate, -1, &pstmt, NULL) == SQLITE_OK) {

    	sqlite3_bind_int(pstmt,  1, cantidad);
        sqlite3_bind_text(pstmt, 2, correo,   -1, SQLITE_STATIC);
        sqlite3_bind_int(pstmt,  3, idProd);
        sqlite3_bind_text(pstmt, 4, variante, -1, SQLITE_STATIC);

        sqlite3_step(pstmt);
        int filasCambiadas = sqlite3_changes(db);
        sqlite3_finalize(pstmt);

        if (filasCambiadas > 0) return 0;

    }

    const char* sqlInsert = "INSERT INTO CARRITO (CORREO, ID_PR, VARIANTE, CANT) VALUES (?, ?, ?, ?)";
    if (sqlite3_prepare_v2(db, sqlInsert, -1, &pstmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmt, 1, correo,   -1, SQLITE_STATIC);
    sqlite3_bind_int(pstmt,  2, idProd);
    sqlite3_bind_text(pstmt, 3, variante, -1, SQLITE_STATIC);
    sqlite3_bind_int(pstmt,  4, cantidad);

    int result = sqlite3_step(pstmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(pstmt);

    return result;

}

int vaciarCarrito(sqlite3* db, const char* correo) {

    if (!db) return -1;

    sqlite3_stmt* pstmt;
    const char* sql = "DELETE FROM CARRITO WHERE CORREO = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);

    int result = sqlite3_step(pstmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(pstmt);

    return result;

}

// CONSULTAS PEDIDOS

static const char* ESTADO_STR[] = {
    "EN PROCESO",   // 1 - EN_PROCESO
    "EN CAMINO",    // 2 - EN CAMINO
    "ENTREGADO",    // 3 - ENTREGADO
};

#define N_ESTADOS 5

Pedido* getPedidos(sqlite3* db, const char* correo, int pagina, int* total) {

    if (!db || !total) return NULL;
    *total = 0;

    // COUNT
    sqlite3_stmt* pstmtCount;
    const char* sqlCount = "SELECT COUNT(*) FROM PEDIDO WHERE CORREO = ?";

    if (sqlite3_prepare_v2(db, sqlCount, -1, &pstmtCount, NULL) == SQLITE_OK) {
        sqlite3_bind_text(pstmtCount, 1, correo, -1, SQLITE_STATIC);
        if (sqlite3_step(pstmtCount) == SQLITE_ROW)
            *total = sqlite3_column_int(pstmtCount, 0);
        sqlite3_finalize(pstmtCount);
    }

    if (*total == 0) return NULL;

    int offset = (pagina - 1) * PEDIDOS_POR_PAGINA;

    // JOIN con UBICACION, CIUDAD y PAIS para el resumen de dirección
    const char* sql = "SELECT PED.ID_PED, "
					  "       strftime('%d/%m/%Y %H:%M', PED.F_ENV_PED), "
					  "       PED.ID_ES, "
					  "       SUM(PP.PRECIO_COMPRA * PP.CANT), "
					  "       U.DIR_UB, C.NOM_CIU, P.NOM_PA "
					  "FROM PEDIDO PED "
					  "JOIN PRODUCTOS_PEDIDO PP ON PED.ID_PED = PP.ID_PED "
					  "JOIN UBICACION U ON PED.ID_UB = U.ID_UB "
					  "JOIN CIUDAD C ON U.ID_CIU = C.ID_CIU "
					  "JOIN PAIS P ON C.ID_PA = P.ID_PA "
					  "WHERE PED.CORREO = ? "
					  "GROUP BY PED.ID_PED "
					  "ORDER BY PED.F_ENV_PED DESC "
					  "LIMIT ? OFFSET ?";

    sqlite3_stmt* pstmt;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);
    sqlite3_bind_int(pstmt,  2, PEDIDOS_POR_PAGINA);
    sqlite3_bind_int(pstmt,  3, offset);

    Pedido* pedidos = malloc(sizeof(Pedido) * PEDIDOS_POR_PAGINA);
    if (!pedidos) { sqlite3_finalize(pstmt); return NULL; }

    int n = 0;
    while (sqlite3_step(pstmt) == SQLITE_ROW && n < PEDIDOS_POR_PAGINA) {

    	Pedido* ped = &pedidos[n++];

        ped->id = sqlite3_column_int(pstmt, 0);
        ped->fecha = strdup((char*)sqlite3_column_text(pstmt, 1));

        int estadoInt = sqlite3_column_int(pstmt, 2);
        if (estadoInt >= 0 && estadoInt < N_ESTADOS) ped->estado = strdup(ESTADO_STR[estadoInt - 1]);
        else ped->estado = strdup("DESCONOCIDO");

        ped->total = sqlite3_column_double(pstmt, 3);

        const char* direccion = (char*)sqlite3_column_text(pstmt, 4);
        const char* ciudad = (char*)sqlite3_column_text(pstmt, 5);
        const char* pais = (char*)sqlite3_column_text(pstmt, 6);
        char resumen[128];
        snprintf(resumen, sizeof(resumen), "%s (%s, %s)", direccion, ciudad, pais);
        ped->resumenDir = strdup(resumen);

    }

    sqlite3_finalize(pstmt);
    return pedidos;

}

Pedido* getPedidoPorId(sqlite3* db, int idPedido) {

    if (!db) return NULL;

    // Misma query pero filtrando exclusivamente por la Primary Key (ID_PED)
    const char* sql = "SELECT PED.ID_PED, "
                      "       strftime('%d/%m/%Y %H:%M', PED.F_ENV_PED), "
                      "       PED.ID_ES, "
                      "       SUM(PP.PRECIO_COMPRA * PP.CANT), "
                      "       U.DIR_UB, C.NOM_CIU, P.NOM_PA "
                      "FROM PEDIDO PED "
                      "JOIN PRODUCTOS_PEDIDO PP ON PED.ID_PED = PP.ID_PED "
                      "JOIN UBICACION U ON PED.ID_UB = U.ID_UB "
                      "JOIN CIUDAD C ON U.ID_CIU = C.ID_CIU "
                      "JOIN PAIS P ON C.ID_PA = P.ID_PA "
                      "WHERE PED.ID_PED = ? "
                      "GROUP BY PED.ID_PED";

    sqlite3_stmt* pstmt;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    sqlite3_bind_int(pstmt, 1, idPedido);

    Pedido* ped = NULL;

    if (sqlite3_step(pstmt) == SQLITE_ROW) {

        ped = (Pedido*)malloc(sizeof(Pedido));
        if (!ped) {
            sqlite3_finalize(pstmt);
            return NULL;
        }

        ped->id = sqlite3_column_int(pstmt, 0);

        const char* fechaTxt = (const char*)sqlite3_column_text(pstmt, 1);
        ped->fecha = fechaTxt ? strdup(fechaTxt) : strdup("Fecha Desconocida");

        int estadoInt = sqlite3_column_int(pstmt, 2);
        if (estadoInt >= 1 && estadoInt <= N_ESTADOS) {
            ped->estado = strdup(ESTADO_STR[estadoInt - 1]);
        } else {
            ped->estado = strdup("DESCONOCIDO");
        }

        ped->total = sqlite3_column_double(pstmt, 3);

        const char* direccion = (const char*)sqlite3_column_text(pstmt, 4);
        const char* ciudad = (const char*)sqlite3_column_text(pstmt, 5);
        const char* pais = (const char*)sqlite3_column_text(pstmt, 6);

        char resumen[256];
        snprintf(resumen, sizeof(resumen), "%s (%s, %s)", direccion ? direccion : "N/A", ciudad ? ciudad : "N/A", pais ? pais : "N/A");

        ped->resumenDir = strdup(resumen);

    }

    sqlite3_finalize(pstmt);

    return ped; // Retornará el pedido si lo encontró, o NULL si el ID no existe

}

void liberarPedidos(Pedido* pedidos, int n) {

    if (!pedidos) return;
    for (int i = 0; i < n; i++) {
        free(pedidos[i].fecha);
        free(pedidos[i].estado);
        free(pedidos[i].resumenDir);
    }
    free(pedidos);

}

int getPedidosCurso(sqlite3* db, const char* correo) {

    if (!db) return -1;

    sqlite3_stmt* pstmt;
    const char* sql =
        "SELECT COUNT(*) FROM PEDIDO "
        "WHERE CORREO = ? AND ID_ES IN (1, 2)";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);

    int total = -1;
    if (sqlite3_step(pstmt) == SQLITE_ROW) total = sqlite3_column_int(pstmt, 0);

    sqlite3_finalize(pstmt);
    return total;

}

// ESCRITURA PEDIDOS

int crearPedido(sqlite3* db, const char* correo, const char* nombre, const char* apellido, int idUbicacion, time_t timestampEjecucion) {

    if (!db) return -1;

    int nItems = 0;
    ItemCarrito* carrito = getCarrito(db, correo, &nItems);
    if (!carrito || nItems == 0) return -1; // Nada que comprar

    Ubicacion* ubDestino = getUbicacionPorId(db, idUbicacion);

    if (!ubDestino) {

        for(int i=0; i<nItems; i++) { free(carrito[i].nombreProducto); free(carrito[i].variante); }
        free(carrito);
        return -1;

    }

    int nAlm = 0;
    Almacen* almacenes = getAlmacenes(db, &nAlm);

    if (!almacenes || nAlm == 0) {

        free(ubDestino->direccion); free(ubDestino);
        for(int i=0; i<nItems; i++) { free(carrito[i].nombreProducto); free(carrito[i].variante); }
        free(carrito);
        return -1;

    }

    AlmCandidato* candidatos = malloc(sizeof(AlmCandidato) * nAlm);
    for (int i = 0; i < nAlm; i++) {
        candidatos[i].alm = &almacenes[i];
        candidatos[i].distancia = calcularDistancia(*ubDestino, almacenes[i].ubicacion);
    }
    qsort(candidatos, nAlm, sizeof(AlmCandidato), cmpCandidatos);

    if (sqlite3_exec(db, "BEGIN", NULL, NULL, NULL) != SQLITE_OK) goto error_memoria;

    sqlite3_stmt* pstmtCheckStock = NULL;
    sqlite3_stmt* pstmtUpdateStock = NULL;

    char* sqlCheck = "SELECT CANT FROM STOCK_ALMACEN WHERE ID_ALM = ? AND ID_PR = ? AND VARIANTE = ?";
    if (sqlite3_prepare_v2(db, sqlCheck, -1, &pstmtCheckStock, NULL) != SQLITE_OK) goto rollback;

    char* sqlUpdate = "UPDATE STOCK_ALMACEN SET CANT = CANT - ? WHERE ID_ALM = ? AND ID_PR = ? AND VARIANTE = ?";
    if (sqlite3_prepare_v2(db, sqlUpdate, -1, &pstmtUpdateStock, NULL) != SQLITE_OK) goto rollback;

    for (int i = 0; i < nItems; i++) {

        int cantRestante = carrito[i].cantidad;

        // Recorremos los almacenes (ordenados del más cercano al más lejano)
        for (int j = 0; j < nAlm && cantRestante > 0; j++) {

            int idAlmacenActual = candidatos[j].alm->id;

            // Consultar stock en este almacén
            sqlite3_bind_int(pstmtCheckStock, 1, idAlmacenActual);
            sqlite3_bind_int(pstmtCheckStock, 2, carrito[i].id);
            sqlite3_bind_text(pstmtCheckStock, 3, carrito[i].variante, -1, SQLITE_STATIC);

            int stockDisponible = 0;
            if (sqlite3_step(pstmtCheckStock) == SQLITE_ROW) {
                stockDisponible = sqlite3_column_int(pstmtCheckStock, 0);
            }
            sqlite3_reset(pstmtCheckStock);

            // Si hay stock, tomamos lo que necesitemos (o lo que haya)
            if (stockDisponible > 0) {

                int aTomar = (stockDisponible >= cantRestante) ? cantRestante : stockDisponible;

                // Restar el stock en base de datos
                sqlite3_bind_int(pstmtUpdateStock, 1, aTomar);
                sqlite3_bind_int(pstmtUpdateStock, 2, idAlmacenActual);
                sqlite3_bind_int(pstmtUpdateStock, 3, carrito[i].id);
                sqlite3_bind_text(pstmtUpdateStock, 4, carrito[i].variante, -1, SQLITE_STATIC);

                if (sqlite3_step(pstmtUpdateStock) != SQLITE_DONE) goto rollback;
                sqlite3_reset(pstmtUpdateStock);

                cantRestante -= aTomar;

            }
        }

        // Si después de mirar TODOS los almacenes sigue faltando cantidad de este producto
        if (cantRestante > 0) {

            // NO HAY STOCK SUFICIENTE
            // Cancelamos el pedido y devolvemos lo que ya habíamos restado.
            goto rollback;

        }

    }

    sqlite3_finalize(pstmtCheckStock);
    sqlite3_finalize(pstmtUpdateStock);

    sqlite3_stmt* pstmtPed;
    char* sqlPed = "INSERT INTO PEDIDO (F_ENV_PED, ID_ES, ID_UB, CORREO) VALUES (datetime('now'), 1, ?, ?)";
    if (sqlite3_prepare_v2(db, sqlPed, -1, &pstmtPed, NULL) != SQLITE_OK) goto rollback;

    sqlite3_bind_int(pstmtPed,  1, idUbicacion);
    sqlite3_bind_text(pstmtPed, 2, correo, -1, SQLITE_STATIC);

    if (sqlite3_step(pstmtPed) != SQLITE_DONE) { sqlite3_finalize(pstmtPed); goto rollback; }
    sqlite3_finalize(pstmtPed);

    int idPedido = (int)sqlite3_last_insert_rowid(db);

    sqlite3_stmt* pstmtProds;
    char* sqlProds =
        "INSERT INTO PRODUCTOS_PEDIDO (ID_PR, ID_PED, VARIANTE, CANT, PRECIO_COMPRA) "
        "SELECT C.ID_PR, ?, C.VARIANTE, C.CANT, ROUND(P.PRECIO_PR * (1.0 - P.DESCTO_PR), 2) "
        "FROM CARRITO C "
        "JOIN PRODUCTO P ON C.ID_PR = P.ID_PR "
        "WHERE C.CORREO = ?";

    if (sqlite3_prepare_v2(db, sqlProds, -1, &pstmtProds, NULL) != SQLITE_OK) goto rollback;

    sqlite3_bind_int(pstmtProds,  1, idPedido);
    sqlite3_bind_text(pstmtProds, 2, correo, -1, SQLITE_STATIC);

    if (sqlite3_step(pstmtProds) != SQLITE_DONE) { sqlite3_finalize(pstmtProds); goto rollback; }
    sqlite3_finalize(pstmtProds);

    if (vaciarCarrito(db, correo) != 0) goto rollback;
    if (sqlite3_exec(db, "COMMIT", NULL, NULL, NULL) != SQLITE_OK) goto rollback;

    char datosRestantesEnCamino[64];
    snprintf(datosRestantesEnCamino, sizeof(datosRestantesEnCamino), "%d,%d,%s,%s,%s", idPedido, 2, correo, nombre, apellido);
    time_t tiempoEjecucionEnCamino = time(NULL) + (HORAS_PROCESADO_PEDIDO * 60 * 60);

    registrarAccion(tiempoEjecucionEnCamino, "COMPRA_PEDIDO", datosRestantesEnCamino);

    char datosRestantesEntregado[64];
    snprintf(datosRestantesEntregado, sizeof(datosRestantesEntregado), "%d,%d,%s,%s,%s", idPedido, 3, correo, nombre, apellido);

    registrarAccion(timestampEjecucion, "COMPRA_PEDIDO", datosRestantesEntregado);

    // Limpiar memoria
    free(candidatos);
    free(ubDestino->direccion); free(ubDestino);
    for(int i=0; i<nAlm; i++) { free(almacenes[i].nombre); free(almacenes[i].ubicacion.direccion); }
    free(almacenes);
    for(int i=0; i<nItems; i++) { free(carrito[i].nombreProducto); free(carrito[i].variante); }
    free(carrito);

    return idPedido;

rollback:

	if (pstmtCheckStock) sqlite3_finalize(pstmtCheckStock);
    if (pstmtUpdateStock) sqlite3_finalize(pstmtUpdateStock);
    sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);

error_memoria:

    free(candidatos);
    if(ubDestino) { liberarUbicacion(ubDestino); }
    if(almacenes) {
        for(int i=0; i<nAlm; i++) { free(almacenes[i].nombre); free(almacenes[i].ubicacion.direccion); }
        free(almacenes);
    }
    if(carrito) {
        for(int i=0; i<nItems; i++) { free(carrito[i].nombreProducto); free(carrito[i].variante); }
        free(carrito);
    }

    return -1;

}

int actualizarEstadoPedido(sqlite3* db, int idPed, int nuevoIdEstado) {

    if (!db) return -1;

    sqlite3_stmt* pstmt;
    char sql[] = "UPDATE PEDIDO SET ID_ES = ? WHERE ID_PED = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int(pstmt, 1, nuevoIdEstado);
    sqlite3_bind_int(pstmt, 2, idPed);

    int res = (sqlite3_step(pstmt) == SQLITE_DONE) ? 0 : -1;
    sqlite3_finalize(pstmt);

    return res;

}

// CONSULTAS FAVORITOS

int getItemsFavoritos(sqlite3* db, const char* correo) {

    if (!db) return -1;

    sqlite3_stmt* pstmt;
    const char* sql = "SELECT COUNT(*) FROM PRODUCTOS_FAVORITOS WHERE CORREO = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);

    int total = -1;
    if (sqlite3_step(pstmt) == SQLITE_ROW) total = sqlite3_column_int(pstmt, 0);

    sqlite3_finalize(pstmt);
    return total;

}

Producto* buscarProductosFavoritos(sqlite3* db, const char* correo, FiltrosProducto f, int pagina, int* total) {

	if (!db || !total) return NULL;

	char where[512] = "WHERE F.CORREO = ?";

	if (strlen(f.nombre) > 0) strncat(where, " AND P.NOM_PR LIKE '%' || ? || '%'", sizeof(where) - strlen(where) - 1);

	if (f.idCategoria != -1) {
		char tmp[64];
		snprintf(tmp, sizeof(tmp), " AND P.ID_CAT = %d", f.idCategoria);
		strncat(where, tmp, sizeof(where) - strlen(where) - 1);
	}

	if (f.precioMin >= 0) {
		char tmp[64];
		snprintf(tmp, sizeof(tmp), " AND ROUND(P.PRECIO_PR * (1.0 - P.DESCTO_PR), 2) >= %.2f", f.precioMin);
		strncat(where, tmp, sizeof(where) - strlen(where) - 1);
	}

	if (f.precioMax >= 0) {
		char tmp[64];
		snprintf(tmp, sizeof(tmp), " AND ROUND(P.PRECIO_PR * (1.0 - P.DESCTO_PR), 2) <= %.2f", f.precioMax);
		strncat(where, tmp, sizeof(where) - strlen(where) - 1);
	}

	// Contamos

	char sqlCount[1024];
	snprintf(sqlCount, sizeof(sqlCount),
			"SELECT COUNT(*) FROM PRODUCTO P "
			"JOIN CATEGORIA C ON P.ID_CAT = C.ID_CAT "
			"JOIN PRODUCTOS_FAVORITOS F ON P.ID_PR = F.ID_PR "
			"%s", where);

	sqlite3_stmt* pstmtCount;
	*total = 0;

	if (sqlite3_prepare_v2(db, sqlCount, -1, &pstmtCount, NULL) == SQLITE_OK) {

		int bind = 1;
		sqlite3_bind_text(pstmtCount, bind++, correo, -1, SQLITE_STATIC);
		if (strlen(f.nombre) > 0) sqlite3_bind_text(pstmtCount, bind++, f.nombre, -1, SQLITE_STATIC);
        if (sqlite3_step(pstmtCount) == SQLITE_ROW) *total = sqlite3_column_int(pstmtCount, 0);
        sqlite3_finalize(pstmtCount);

	}

	if (*total == 0) return NULL;

	// Seleccionamos

	int offset = (pagina - 1) * ITEMS_POR_PAGINA;
	char sql[1400];
	snprintf(sql, sizeof(sql),
			"SELECT P.ID_PR, P.NOM_PR, P.DESCRIP_PR, P.PRECIO_PR, P.DESCTO_PR, C.ID_CAT, C.NOM_CAT, C.VARIANTES_CAT "
			"FROM PRODUCTO P "
			"JOIN CATEGORIA C ON P.ID_CAT = C.ID_CAT "
			"JOIN PRODUCTOS_FAVORITOS F ON P.ID_PR = F.ID_PR "
			"%s "
			"ORDER BY P.ID_PR ASC "
			"LIMIT %d OFFSET %d", where, ITEMS_POR_PAGINA, offset);

	sqlite3_stmt* pstmt;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    int bind = 1;
    sqlite3_bind_text(pstmt, bind++, correo, -1, SQLITE_STATIC);
    if (strlen(f.nombre) > 0) sqlite3_bind_text(pstmt, bind++, f.nombre, -1, SQLITE_STATIC);

    Producto* resultado = malloc(sizeof(Producto) * ITEMS_POR_PAGINA);

    int n = 0;
    while (sqlite3_step(pstmt) == SQLITE_ROW && n < ITEMS_POR_PAGINA) resultado[n++] = productoDB(pstmt);

    sqlite3_finalize(pstmt);
    return resultado;

}

// ESCRITURA FAVORITOS

int toggleFavorito(sqlite3* db, const char* correo, int idProd) {

    if (!db) return -1;

    // Comprobamos si ya existe
    sqlite3_stmt* pstmtCheck;
    const char* sqlCheck = "SELECT 1 FROM PRODUCTOS_FAVORITOS WHERE CORREO = ? AND ID_PR = ?";

    if (sqlite3_prepare_v2(db, sqlCheck, -1, &pstmtCheck, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmtCheck, 1, correo, -1, SQLITE_STATIC);
    sqlite3_bind_int(pstmtCheck,  2, idProd);

    int existe = (sqlite3_step(pstmtCheck) == SQLITE_ROW);
    sqlite3_finalize(pstmtCheck);

    // Según si existe: eliminamos o añadimos
    const char* sql = existe ? "DELETE FROM PRODUCTOS_FAVORITOS WHERE CORREO = ? AND ID_PR = ?" : "INSERT INTO PRODUCTOS_FAVORITOS (CORREO, ID_PR) VALUES (?, ?)";

    sqlite3_stmt* pstmt;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);
    sqlite3_bind_int(pstmt,  2, idProd);

    int ok = (sqlite3_step(pstmt) == SQLITE_DONE);
    sqlite3_finalize(pstmt);

    if (!ok) return -1;

    return existe ? 0 : 1;  // 0 = eliminado, 1 = añadido

}

// CONSULTAS RESEÑAS

double getValoracionMedia(sqlite3* db, int idProd) {

	if (!db) return -1;

    sqlite3_stmt* pstmt;

    // Media ponderada: SUM(VALORACION * PESO) / SUM(PESO)

    char sql[] =
        "SELECT CAST(SUM(VALORACION * PESO) AS REAL) / SUM(PESO) "
        "FROM RESENA WHERE ID_PR = ? AND PESO > 0";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_int(pstmt, 1, idProd);

    double val = -1;
    if (sqlite3_step(pstmt) == SQLITE_ROW && sqlite3_column_type(pstmt, 0) != SQLITE_NULL) val = sqlite3_column_double(pstmt, 0);

    sqlite3_finalize(pstmt);
    return val;

}

int getNumResenas(sqlite3* db, int idProd) {

    if (!db) return 0;

    sqlite3_stmt* pstmt;
    char sql[] = "SELECT SUM(PESO) FROM RESENA WHERE ID_PR = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return 0;

    sqlite3_bind_int(pstmt, 1, idProd);

    int n = 0;
    if (sqlite3_step(pstmt) == SQLITE_ROW && sqlite3_column_type(pstmt, 0) != SQLITE_NULL) n = sqlite3_column_int(pstmt, 0);

    sqlite3_finalize(pstmt);
    return n;

}

Resena* getResenas(sqlite3* db, int idProd, int pagina, int* total) {

	if (!db || !total) return NULL;

	sqlite3_stmt* pstmtCount;
	char* sqlCount = "SELECT COUNT(*) FROM RESENA WHERE ID_PR = ? AND CORREO != 'ADMIN'";
	*total = 0;

	if (sqlite3_prepare_v2(db, sqlCount, -1, &pstmtCount, NULL) == SQLITE_OK) {

		sqlite3_bind_int(pstmtCount, 1, idProd);
		if (sqlite3_step(pstmtCount) == SQLITE_ROW) *total = sqlite3_column_int(pstmtCount, 0);
		sqlite3_finalize(pstmtCount);

	}

	if (*total == 0) return NULL;

	int offset = (pagina - 1) * RESENAS_POR_PAGINA;

	char* sql = "SELECT CORREO, VALORACION, COMENTARIO, FECHA "
				"FROM RESENA "
				"WHERE ID_PR = ? AND CORREO != 'ADMIN' "
				"ORDER BY FECHA DESC "
				"LIMIT ? OFFSET ?";

	sqlite3_stmt* pstmt;
	if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

	sqlite3_bind_int(pstmt, 1, idProd);
	sqlite3_bind_int(pstmt, 2, RESENAS_POR_PAGINA);
	sqlite3_bind_int(pstmt, 3, offset);

	Resena* resenas = malloc(sizeof(Resena) * RESENAS_POR_PAGINA);

	int n = 0;
	while (sqlite3_step(pstmt) == SQLITE_ROW && n < RESENAS_POR_PAGINA) {

		Resena* r = &resenas[n++];
		r->correo = strdup((char*)sqlite3_column_text(pstmt, 0));
		r->valoracion = sqlite3_column_double(pstmt, 1);
		r->comentario = sqlite3_column_text(pstmt, 2) ? strdup((char*)sqlite3_column_text(pstmt, 2)) : NULL;
		r->fecha = strdup((char*)sqlite3_column_text(pstmt, 3));

	}

	sqlite3_finalize(pstmt);
	return resenas;

}

void liberarResenas(Resena* resenas, int n) {

    if (!resenas) return;
    for (int i = 0; i < n; i++) {
        free(resenas[i].correo);
        if (resenas[i].comentario) free(resenas[i].comentario);
        free(resenas[i].fecha);
    }
    free(resenas);

}

int comprobarResena(sqlite3* db, int idProd, const char* correo) {

	if (!db) return 0;

	sqlite3_stmt* pstmt;
	char* sql = "SELECT COUNT(*) FROM RESENA "
				"WHERE ID_PR = ? AND CORREO = ?";

	if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return 0;

	sqlite3_bind_int(pstmt, 1, idProd);
	sqlite3_bind_text(pstmt, 2, correo, -1, SQLITE_STATIC);

	int res = 0;
	if (sqlite3_step(pstmt) == SQLITE_ROW) res = sqlite3_column_int(pstmt, 0);

	sqlite3_finalize(pstmt);
	return res;

}

// ESCRITURA RESEÑAS

int addResena(sqlite3* db, int idProd, const char* correo, double valoracion, const char* comentario) {

	if (!db) return -1;

	sqlite3_stmt* pstmt;
	char sql[512];

	int tieneRes = comprobarResena(db, idProd, correo);

	if (!tieneRes) {

		snprintf(sql, sizeof(sql), "INSERT INTO RESENA (ID_PR, CORREO, VALORACION, COMENTARIO, FECHA, PESO) "
								   "VALUES (?, ?, ?, ?, date('now'), 1)");

	} else {

		snprintf(sql, sizeof(sql), "UPDATE RESENA SET "
								   "VALORACION = ?, COMENTARIO = ?, FECHA = date('now') "
								   "WHERE ID_PR = ? AND CORREO = ?");

	}

	if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

	if (!tieneRes) {

		sqlite3_bind_int(pstmt, 1, idProd);
		sqlite3_bind_text(pstmt, 2, correo, -1, SQLITE_STATIC);
		sqlite3_bind_double(pstmt, 3, valoracion);
		sqlite3_bind_text(pstmt, 4, comentario, -1, SQLITE_STATIC);

	} else {

		sqlite3_bind_double(pstmt, 1, valoracion);
		sqlite3_bind_text(pstmt, 2, comentario, -1, SQLITE_STATIC);
		sqlite3_bind_int(pstmt, 3, idProd);
		sqlite3_bind_text(pstmt, 4, correo, -1, SQLITE_STATIC);

	}

	int result = sqlite3_step(pstmt) == SQLITE_DONE ? 0 : -1;
	sqlite3_finalize(pstmt);

	return result;

}

int eliminarResena(sqlite3* db, int idProd, const char* correo) {

	if (!db) return -1;

	sqlite3_stmt* pstmt;
	char* sql = "DELETE FROM RESENA "
				"WHERE ID_PR = ? AND CORREO = ?";

	if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return -1;

	sqlite3_bind_int(pstmt, 1, idProd);
	sqlite3_bind_text(pstmt, 2, correo, -1, SQLITE_STATIC);

	int result = sqlite3_step(pstmt) == SQLITE_DONE ? 0 : -1;
	sqlite3_finalize(pstmt);

	return result;

}

// CONSULTAS UBICACIONES

Ubicacion* getUbicacionPorId(sqlite3* db, int idUbicacion) {

    if (!db) return NULL;

    const char* sql = "SELECT U.ID_UB, U.DIR_UB, U.LAT_UB, U.LON_UB, U.ID_CIU, C.NOM_CIU, C.ID_PA, P.NOM_PA "
                      "FROM UBICACION U "
                      "JOIN CIUDAD C ON U.ID_CIU = C.ID_CIU "
                      "JOIN PAIS P ON C.ID_PA = P.ID_PA "
                      "WHERE U.ID_UB = ?";

    sqlite3_stmt* pstmt;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) return NULL;

    sqlite3_bind_int(pstmt, 1, idUbicacion);

    Ubicacion* u = NULL;

    if (sqlite3_step(pstmt) == SQLITE_ROW) {

        u = malloc(sizeof(Ubicacion));

        if (u) {

            u->id = sqlite3_column_int(pstmt, 0);

            const char* dir = (const char*)sqlite3_column_text(pstmt, 1);
            u->direccion = dir ? strdup(dir) : strdup("");

            u->latitud = sqlite3_column_double(pstmt, 2);
            u->longitud = sqlite3_column_double(pstmt, 3);

            u->ciudad.id = sqlite3_column_int(pstmt, 4);

            const char* nomCiu = (const char*)sqlite3_column_text(pstmt, 5);
            u->ciudad.nombre = nomCiu ? strdup(nomCiu) : strdup("");

            const char* idPa = (const char*)sqlite3_column_text(pstmt, 6);
            u->ciudad.pais.id = idPa ? strdup(idPa) : strdup("");

            const char* nomPa = (const char*)sqlite3_column_text(pstmt, 7);
            u->ciudad.pais.nombre = nomPa ? strdup(nomPa) : strdup("");

        }

    }

    sqlite3_finalize(pstmt);
    return u;

}

void liberarUbicacion(Ubicacion* u) {

	free(u->direccion);
	free(u->ciudad.nombre);
	free(u->ciudad.pais.id);
	free(u->ciudad.pais.nombre);
	free(u);

}

Ubicacion* getUbicaciones(sqlite3* db, const char* correo, int* n) {

    if (!db || !n) return NULL;
    *n = 0;

    // COUNT
    sqlite3_stmt* pstmtCount;
    const char* sqlCount = "SELECT COUNT(*) FROM UBICACION "
    					   "WHERE CORREO = ? AND ACTIVO = 1";

    if (sqlite3_prepare_v2(db, sqlCount, -1, &pstmtCount, NULL) == SQLITE_OK) {

        sqlite3_bind_text(pstmtCount, 1, correo, -1, SQLITE_STATIC);
        if (sqlite3_step(pstmtCount) == SQLITE_ROW) *n = sqlite3_column_int(pstmtCount, 0);
        sqlite3_finalize(pstmtCount);

    }

    if (*n == 0) return NULL;

    const char* sql = "SELECT U.ID_UB, U.DIR_UB, U.LAT_UB, U.LON_UB, U.ID_CIU, C.NOM_CIU, C.ID_PA, P.NOM_PA "
					  "FROM UBICACION U "
					  "JOIN CIUDAD C ON U.ID_CIU = C.ID_CIU "
					  "JOIN PAIS P ON C.ID_PA = P.ID_PA "
					  "WHERE U.CORREO = ? AND U.ACTIVO = 1 "
					  "ORDER BY U.ID_UB ASC";

    sqlite3_stmt* pstmt;
    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) != SQLITE_OK) { *n = 0; return NULL; }

    sqlite3_bind_text(pstmt, 1, correo, -1, SQLITE_STATIC);

    Ubicacion* dirs = malloc(sizeof(Ubicacion) * (*n));
    if (!dirs) { sqlite3_finalize(pstmt); *n = 0; return NULL; }

    int i = 0;
    while (sqlite3_step(pstmt) == SQLITE_ROW && i < *n) {
        Ubicacion* d = &dirs[i++];
        d->id = sqlite3_column_int(pstmt, 0);
        d->direccion = strdup((char*)sqlite3_column_text(pstmt, 1));
        d->latitud = sqlite3_column_double(pstmt, 2);
        d->longitud = sqlite3_column_double(pstmt, 3);
        d->ciudad.id = sqlite3_column_int(pstmt, 4);
        d->ciudad.nombre = strdup((char*)sqlite3_column_text(pstmt, 5));
        d->ciudad.pais.id = strdup((char*)sqlite3_column_text(pstmt, 6));
        d->ciudad.pais.nombre = strdup((char*)sqlite3_column_text(pstmt, 7));
    }
    *n = i;

    sqlite3_finalize(pstmt);
    return dirs;

}

void liberarUbicaciones(Ubicacion* dirs, int n) {

    if (!dirs) return;
    for (int i = 0; i < n; i++) {
        free(dirs[i].direccion);
        free(dirs[i].ciudad.nombre);
        free(dirs[i].ciudad.pais.id);
        free(dirs[i].ciudad.pais.nombre);
    }
    free(dirs);
}

// ESCRITURA UBICACIONES

int crearUbicacion(sqlite3* db, const char* correo, const char* pais, const char* ciudad, const char* direccion, double latitud, double longitud) {

    if (!db) return -1;

    char idPais[4];
    sqlite3_stmt* pstmtComprobPais;
    char* sqlComprobPais = "SELECT ID_PA FROM PAIS WHERE NOM_PA = ?";

    if (sqlite3_prepare_v2(db, sqlComprobPais, -1, &pstmtComprobPais, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmtComprobPais, 1, pais, -1, SQLITE_STATIC);

    if (sqlite3_step(pstmtComprobPais) == SQLITE_ROW) {

    	strcpy(idPais, (char*)sqlite3_column_text(pstmtComprobPais, 0));

    } else {

        sqlite3_stmt* pstmtInsertPais;
        char* sqlInsertPais = "INSERT INTO PAIS (ID_PA, NOM_PA) VALUES (?, ?)";

        if (sqlite3_prepare_v2(db, sqlInsertPais, -1, &pstmtInsertPais, NULL) != SQLITE_OK) {
            sqlite3_finalize(pstmtComprobPais);
            return -1;
        }

        char codigo[4] = "";
        for (int i = 0 ; i < 3 && pais[i] != '\0'; i++) codigo[i] = toupper(pais[i]);
        codigo[3] = '\0';

        sqlite3_bind_text(pstmtInsertPais, 1, codigo, -1, SQLITE_STATIC);
        sqlite3_bind_text(pstmtInsertPais, 2, pais, -1, SQLITE_STATIC);

        if (sqlite3_step(pstmtInsertPais) != SQLITE_DONE) {
            sqlite3_finalize(pstmtInsertPais);
            sqlite3_finalize(pstmtComprobPais);
            return -1;
        }

        strcpy(idPais, codigo);
        sqlite3_finalize(pstmtInsertPais);

    }

    sqlite3_finalize(pstmtComprobPais);

    int idCiudad;
    sqlite3_stmt* pstmtComprobCiudad;
    char* sqlComprobCiudad = "SELECT ID_CIU FROM CIUDAD WHERE NOM_CIU = ? AND ID_PA = ?";

    if (sqlite3_prepare_v2(db, sqlComprobCiudad, -1, &pstmtComprobCiudad, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmtComprobCiudad, 1, ciudad, -1, SQLITE_STATIC);
    sqlite3_bind_text(pstmtComprobCiudad, 2, idPais, -1, SQLITE_STATIC);

    if (sqlite3_step(pstmtComprobCiudad) == SQLITE_ROW) {

        idCiudad = sqlite3_column_int(pstmtComprobCiudad, 0);

    } else {
        sqlite3_stmt* pstmtInsertCiudad;
        char* sqlInsertCiudad = "INSERT INTO CIUDAD (NOM_CIU, LAT_CIU, LON_CIU, POBLACION_CIU, ID_PA) VALUES (?, NULL, NULL, NULL, ?)";

        if (sqlite3_prepare_v2(db, sqlInsertCiudad, -1, &pstmtInsertCiudad, NULL) != SQLITE_OK) {
            sqlite3_finalize(pstmtComprobCiudad);
            return -1;
        }

        sqlite3_bind_text(pstmtInsertCiudad, 1, ciudad, -1, SQLITE_STATIC);
        sqlite3_bind_text(pstmtInsertCiudad, 2, idPais, -1, SQLITE_STATIC);

        if (sqlite3_step(pstmtInsertCiudad) != SQLITE_DONE) {
            sqlite3_finalize(pstmtInsertCiudad);
            sqlite3_finalize(pstmtComprobCiudad);
            return -1;
        }

        idCiudad = (int)sqlite3_last_insert_rowid(db);
        sqlite3_finalize(pstmtInsertCiudad);

    }

    sqlite3_finalize(pstmtComprobCiudad);

    sqlite3_stmt* pstmtUbi;
    char sqlUbi[] =
        "INSERT INTO UBICACION (DIR_UB, LAT_UB, LON_UB, ID_CIU, CORREO) "
        "VALUES (?, ?, ?, ?, ?)";

    if (sqlite3_prepare_v2(db, sqlUbi, -1, &pstmtUbi, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(pstmtUbi, 1, direccion, -1, SQLITE_STATIC);
    sqlite3_bind_double(pstmtUbi, 2, latitud);
    sqlite3_bind_double(pstmtUbi, 3, longitud);
    sqlite3_bind_int(pstmtUbi, 4, idCiudad);
    sqlite3_bind_text(pstmtUbi, 5, correo, -1, SQLITE_STATIC);

    if (sqlite3_step(pstmtUbi) != SQLITE_DONE) {
        sqlite3_finalize(pstmtUbi);
        return -1;
    }

    int idUbi = (int)sqlite3_last_insert_rowid(db);
    sqlite3_finalize(pstmtUbi);

    return idUbi;

}
