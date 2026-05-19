#include "sqlite3.h"
#include "logistica.h"
#include "config.h"
#include "log.h"
#include "usuario_db.h"
#include "almacenes_db.h"
#include "estructuras.h"
#include <math.h>
#include <time.h>
#include <stdlib.h>

// HAVERSINE

double calcularDistancia(Ubicacion u1, Ubicacion u2) {
    double lat1 = DEG_A_RAD(u1.latitud);
    double lat2 = DEG_A_RAD(u2.latitud);
    double dLat = DEG_A_RAD(u2.latitud - u1.latitud);
    double dLon = DEG_A_RAD(u2.longitud - u1.longitud);

    double a = sin(dLat / 2) * sin(dLat / 2) + cos(lat1) * cos(lat2) * sin(dLon / 2) * sin(dLon / 2);

    return RADIO_TIERRA_KM * 2.0 * asin(sqrt(a)) * 1.2;
}

// COSTE TRASVASE ENTRE ALMACENES

// precio  = max(COSTE_MIN_TRASVASE, (dist_km * COSTE_KM_VEHICULO * ceil(cant / CAPAC_VEHICULO)) + (cant * COSTE_MANIP_UND))
// tiempo  = max(HORAS_MIN_TRASVASE, dist_km / VEL_MEDIA_KMH + overhead_carga_log)
void calcularCosteTrasvase(Ubicacion ubi1, Ubicacion ubi2, int cant, double* precio, time_t* duracion) {

    if (!precio || !duracion) return;

    double dist = calcularDistancia(ubi1, ubi2);

    // Calculamos camiones necesarios (ej: 500 uds por camión)
    int viajesNecesarios = (int)ceil((double)cant / CAPACIDAD_VEHICULO);
    if (viajesNecesarios == 0) viajesNecesarios = 1;

    // Coste de mandar X camiones + sueldo por empaquetar X unidades
    double costeBase = (dist * COSTE_KM_VEHICULO * viajesNecesarios) + (cant * COSTE_MANIPULACION_UND);

    *precio = costeBase > COSTE_MIN_TRASVASE ? costeBase : COSTE_MIN_TRASVASE;

    // Tiempo: Conducir (no se multiplica por camiones) + Cargar (logarítmico)
    double horasCond = dist / VEL_MEDIA_KMH;
    double horasCarga = 0.5 * log10(1.0 + cant / 100.0);

    double horasTotal = horasCond + horasCarga;
    if (horasTotal < HORAS_MIN_TRASVASE) horasTotal = HORAS_MIN_TRASVASE;

    *duracion = (time_t)(horasTotal * 3600.0);

}

// COSTE PAQUETERÍA

// precio  = COSTE_BASE_PAQUETERIA + (max(0, cant - 1) * COSTE_EXTRA_ARTICULO) + (dist_km * COSTE_KM_PAQUETERIA)
// tiempo  = HORAS_PROCESADO_PEDIDO + (dist_km / VEL_MEDIA_RED_LOGISTICA)
void calcularCostePaqueteria(Ubicacion origen, Ubicacion destino, int cant, double* precio, time_t* duracion) {

    if (!precio || !duracion) return;

    double dist = calcularDistancia(origen, destino);

    // Cobramos el extra de manipulación/peso a partir del segundo artículo
    int articulosExtra = (cant > 1) ? (cant - 1) : 0;

    *precio = COSTE_BASE_PAQUETERIA + (articulosExtra * COSTE_EXTRA_ARTICULO) + (dist * COSTE_KM_PAQUETERIA);

    // Tiempo en almacén + tiempo viajando por la red de transporte
    double horasTransito = dist / VEL_MEDIA_RED_LOGISTICA;
    double horasTotal = HORAS_PROCESADO_PEDIDO + horasTransito;

    // Convertimos las horas totales a segundos para el time_t
    *duracion = (time_t)(horasTotal * 3600.0);

}

// COSTE COMPRA EXTERNA (proveedor)
//
// precio   = sum(precio_venta * (1-margen) * cant + manipulacion)
// duracion = 8h preparacion + 2 dias transito

void calcularCosteCompraExterno(ProdCant* prods, int nProds, double* precio, time_t* duracion) {

	if (!precio || !duracion || !prods) return;

    *precio = 0.0;
    for (int i = 0; i < nProds; i++) {
        double costeUnd = prods[i].producto.precio * (1.0 - MARGEN_GANANCIAS);
        double manip = prods[i].cantidad * COSTE_MANIPULACION_UND;
        *precio += costeUnd * prods[i].cantidad + manip;
    }

    double horasTotal = HORAS_PREPARACION_EXT + DIAS_TRANSITO_EXT * 24.0;
    *duracion = (time_t)(horasTotal * 3600.0);

}

// COSTE RESTOCK

void calcularCosteRestock(sqlite3* db, int idAlm, double* precio, time_t* duracion) {

    if (!precio || !duracion || !db) return;

    *precio = 0.0;
    *duracion = 0;

    Almacen* a = getAlmacenPorId(db, idAlm);
    if (!a) return;

    int ocupacion = getOcupacionAlmacen(db, idAlm);
    int objetivo = (int)(a->capacidad * 0.8);

    int udsNecesarias = objetivo - ocupacion;

    liberarAlmacen(a);

    if (ocupacion >= objetivo) return; // Ya está al 80% o más

    // Contar productos actuales
    sqlite3_stmt* pstmtCount;
    int nProdsActuales = 0;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(DISTINCT ID_PR) FROM STOCK_ALMACEN WHERE ID_ALM = ?", -1, &pstmtCount, NULL) == SQLITE_OK) {

    	sqlite3_bind_int(pstmtCount, 1, idAlm);
        if (sqlite3_step(pstmtCount) == SQLITE_ROW) nProdsActuales = sqlite3_column_int(pstmtCount, 0);
        sqlite3_finalize(pstmtCount);

    }

    // Estimamos un target promedio de 1300
    int targetProdsDistintos = 1300;
    int faltan = targetProdsDistintos - nProdsActuales;
    if (faltan < 0) faltan = 0;

    int nProdsFuturos = nProdsActuales + faltan;
    if (nProdsFuturos == 0) return;

    int targetBasePorProd = objetivo / nProdsFuturos;

    int totalUdsExistentes = 0;
    double costeCompraExistentes = 0.0;

    // Calcular coste EXACTO para los productos EXISTENTES
    sqlite3_stmt* pstmt;
    char sql[] =
        "SELECT SA.CANT, NV.N_VARIANTES, P.PRECIO_PR "
        "FROM STOCK_ALMACEN SA "
        "JOIN PRODUCTO P ON SA.ID_PR = P.ID_PR "
        "JOIN (SELECT ID_PR, COUNT(*) AS N_VARIANTES "
        "      FROM STOCK_ALMACEN WHERE ID_ALM = ? AND DISPONIBLE = 1 GROUP BY ID_PR) NV "
        "ON SA.ID_PR = NV.ID_PR "
        "WHERE SA.ID_ALM = ? AND SA.DISPONIBLE = 1";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(pstmt, 1, idAlm);
        sqlite3_bind_int(pstmt, 2, idAlm);

        while (sqlite3_step(pstmt) == SQLITE_ROW) {
            int cantActual = sqlite3_column_int(pstmt, 0);
            int nVariantes = sqlite3_column_int(pstmt, 1);
            double precioProd = sqlite3_column_double(pstmt, 2);

            int targetBasePorVariante = nVariantes > 1 ? (targetBasePorProd / nVariantes) + 1 : targetBasePorProd;

            int targetEstimado = targetBasePorVariante;
            int deficit = targetEstimado - cantActual;

            if (deficit > 0) {
                totalUdsExistentes += deficit;
                costeCompraExistentes += (deficit * precioProd);
            }
        }
        sqlite3_finalize(pstmt);
    }

    // Calcular coste estimado para los productos NUEVOS
    int totalUdsNuevos = 0;
    double costeCompraNuevos = 0.0;

    if (faltan > 0) {

        // Obtenemos el precio medio de todoo el catálogo
        double precioMedioCatalogo = 0.0; // Valor por defecto de seguridad
        sqlite3_stmt* pstmtPM;
        if (sqlite3_prepare_v2(db, "SELECT AVG(PRECIO_PR) FROM PRODUCTO", -1, &pstmtPM, NULL) == SQLITE_OK) {

        	if (sqlite3_step(pstmtPM) == SQLITE_ROW) precioMedioCatalogo = sqlite3_column_double(pstmtPM, 0);
            sqlite3_finalize(pstmtPM);

        }

        // Estimamos cuántas unidades totales añadirán los productos nuevos

        totalUdsNuevos = faltan * targetBasePorProd;
        costeCompraNuevos = totalUdsNuevos * precioMedioCatalogo;

    }

    // Unir todoo y calcular el precio final

    int unidadesIdealesTotales = totalUdsExistentes + totalUdsNuevos;

    if (unidadesIdealesTotales > 0) {

        // Sacamos el precio medio perfecto según tu modelo híbrido
        double costeCompraIdeal = costeCompraExistentes + costeCompraNuevos;
        double precioMedioPonderado = costeCompraIdeal / unidadesIdealesTotales;

        // Aplicamos ese precio SOLO a las unidades que realmente vamos a añadir
        double costeCompra = udsNecesarias * precioMedioPonderado * (1.0 - MARGEN_GANANCIAS);
        double manip = udsNecesarias * COSTE_MANIPULACION_UND;
        *precio = costeCompra + manip;

        double horasTotal = HORAS_PREPARACION_EXT + DIAS_TRANSITO_EXT * 24.0;
        *duracion = (time_t)(horasTotal * 3600.0);

    }

}

// COSTE ELIMINACIÓN ALMACEN

void calcularCosteCierreAlmacen(sqlite3* db, int idAlm, double* precio, time_t* duracion) {

    if (!precio || !duracion || !db) return;

    *precio = 0.0;
    *duracion = 0;

    Almacen* aElim = getAlmacenPorId(db, idAlm);
    if (!aElim) return;

    int nAlm = 0;
    Almacen* todos = getAlmacenes(db, &nAlm);

    AlmCandidato* candidatos = malloc(sizeof(AlmCandidato) * nAlm);
    int nCand = 0;

    for (int i = 0; i < nAlm; i++) {

        if (todos[i].id == idAlm) continue;

        int ocup = getOcupacionAlmacen(db, todos[i].id);
        candidatos[nCand].alm = &todos[i];
        candidatos[nCand].distancia = calcularDistancia(aElim->ubicacion, todos[i].ubicacion);
        candidatos[nCand].espacioLibre = todos[i].capacidad - ocup;
        candidatos[nCand].cantAEnviar = 0;
        nCand++;

    }

    if (nCand > 0) qsort(candidatos, nCand, sizeof(AlmCandidato), cmpCandidatos);

    sqlite3_stmt* pstmt;
    char sql[] =
        "SELECT ID_PR, VARIANTE, CANT FROM STOCK_ALMACEN "
        "WHERE ID_ALM = ? AND DISPONIBLE = 1";

    if (sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL) == SQLITE_OK) {

        sqlite3_bind_int(pstmt, 1, idAlm);

        while (sqlite3_step(pstmt) == SQLITE_ROW) {

            int cant = sqlite3_column_int(pstmt, 2);
            int cantRestante = cant;

            for (int i = 0; i < nCand && cantRestante > 0; i++) {

                if (candidatos[i].espacioLibre > 0) {

                    int aMover = (candidatos[i].espacioLibre >= cantRestante) ? cantRestante : candidatos[i].espacioLibre;

                    candidatos[i].espacioLibre -= aMover;
                    candidatos[i].cantAEnviar += aMover;
                    cantRestante -= aMover;

                }

            }

        }

        sqlite3_finalize(pstmt);

    }

    // Calculamos el coste UNA SOLA VEZ por cada almacén destino
    for (int i = 0; i < nCand; i++) {

        if (candidatos[i].cantAEnviar > 0) {

            double costeDestino = 0.0;
            time_t durDestino = 0;

            calcularCosteTrasvase(aElim->ubicacion, candidatos[i].alm->ubicacion, candidatos[i].cantAEnviar, &costeDestino, &durDestino);

            *precio += costeDestino;
            if (durDestino > *duracion) *duracion = durDestino;

        }

    }

    free(candidatos);

    for (int i = 0; i < nAlm; i++) {
        free(todos[i].nombre);
        free(todos[i].ubicacion.direccion);
    }

    free(todos);
    free(aElim->nombre);
    free(aElim->ubicacion.direccion);
    free(aElim);

}

void calcularCosteEnvio(sqlite3* db, const char* correo, int idUbicacion, double* precio, time_t* duracion) {

    if (!precio || !duracion || !db || !correo) return;

    *precio = 0.0;
    *duracion = 0;

    int nItems = 0;
    ItemCarrito* carrito = getCarrito(db, correo, &nItems);
    if (!carrito || nItems == 0) return; // Nada en el carrito

    Ubicacion* ubDestino = getUbicacionPorId(db, idUbicacion);
    if (!ubDestino) {
        for(int i=0; i<nItems; i++) { free(carrito[i].nombreProducto); free(carrito[i].variante); }
        free(carrito);
        return;
    }

    int nAlm = 0;
    Almacen* almacenes = getAlmacenes(db, &nAlm);
    if (!almacenes || nAlm == 0) {
        liberarUbicacion(ubDestino);
        for(int i=0; i<nItems; i++) { free(carrito[i].nombreProducto); free(carrito[i].variante); }
        free(carrito);
        return;
    }

    AlmCandidato* candidatos = malloc(sizeof(AlmCandidato) * nAlm);
    for (int i = 0; i < nAlm; i++) {
        candidatos[i].alm = &almacenes[i];
        candidatos[i].distancia = calcularDistancia(*ubDestino, almacenes[i].ubicacion);
        candidatos[i].cantAEnviar = 0;
    }

    // Ordenamos por distancia (del más cercano al más lejano)
    qsort(candidatos, nAlm, sizeof(AlmCandidato), cmpCandidatos);

    sqlite3_stmt* pstmtCheckStock = NULL;
    char* sqlCheck = "SELECT CANT FROM STOCK_ALMACEN WHERE ID_ALM = ? AND ID_PR = ? AND VARIANTE = ? AND DISPONIBLE = 1";

    if (sqlite3_prepare_v2(db, sqlCheck, -1, &pstmtCheckStock, NULL) != SQLITE_OK) {
        goto error_limpieza;
    }

    int stockSuficiente = 1;

    for (int i = 0; i < nItems; i++) {

        int cantRestante = carrito[i].cantidad;

        for (int j = 0; j < nAlm && cantRestante > 0; j++) {

            sqlite3_bind_int(pstmtCheckStock, 1, candidatos[j].alm->id);
            sqlite3_bind_int(pstmtCheckStock, 2, carrito[i].id);
            sqlite3_bind_text(pstmtCheckStock, 3, carrito[i].variante, -1, SQLITE_STATIC);

            int stockDisponible = 0;
            if (sqlite3_step(pstmtCheckStock) == SQLITE_ROW) {
                stockDisponible = sqlite3_column_int(pstmtCheckStock, 0);
            }
            sqlite3_reset(pstmtCheckStock);

            if (stockDisponible > 0) {

                int aTomar = (stockDisponible >= cantRestante) ? cantRestante : stockDisponible;

                // Acumulamos cuántos productos enviará este almacén en total
                candidatos[j].cantAEnviar += aTomar;
                cantRestante -= aTomar;

            }

        }

        if (cantRestante > 0) {
            stockSuficiente = 0;
            break; // No hay stock para este producto
        }

    }

    sqlite3_finalize(pstmtCheckStock);

    if (stockSuficiente) {

        for (int i = 0; i < nAlm; i++) {

            if (candidatos[i].cantAEnviar > 0) {

                double costeOrigen = 0.0;
                time_t durOrigen = 0;

                calcularCostePaqueteria(candidatos[i].alm->ubicacion, *ubDestino, candidatos[i].cantAEnviar, &costeOrigen, &durOrigen);
                *precio += costeOrigen;

                // El envío total tarda lo que tarde el almacén más lento en llegar
                if (durOrigen > *duracion) {
                    *duracion = durOrigen;
                }

            }

        }

    } else {
        // Si no hay stock, marcamos con -1 para que la capa superior lo gestione
        *precio = -1.0;
    }

error_limpieza:
    // Liberación de toda la memoria alojada dinámicamente
    free(candidatos);
    liberarUbicacion(ubDestino);

    for(int i = 0; i < nAlm; i++) {
        free(almacenes[i].nombre);
        free(almacenes[i].ubicacion.direccion);
    }
    free(almacenes);

    for(int i = 0; i < nItems; i++) {
        free(carrito[i].nombreProducto);
        free(carrito[i].variante);
    }
    free(carrito);
}

// GESTION DE FICHERO MOVIMIENTOS

#ifndef CONFIG_PATH
#define CONFIG_PATH "../data/config/server_config.ini"
#endif

void registrarAccion(time_t timestamp, const char* tipo, const char* datosRestantes) {

	char csvPath[256];
	configGet(CONFIG_PATH, "ACCIONES_PENDIENTES_PATH", csvPath, sizeof(csvPath));

    HANDLE hMutex = CreateMutexA(NULL, FALSE, "Global\\DeustoCommerce_CSV_Mutex");
    if (hMutex == NULL) return;

    // Esperamos a que esté libre (bloquea a cualquier otro hilo o programa)
    WaitForSingleObject(hMutex, INFINITE);

    FILE* file = fopen(csvPath, "a");
    if (file != NULL) {
        fprintf(file, "%lld;%s;%s\n", timestamp, tipo, datosRestantes);
        fclose(file);
    }

    ReleaseMutex(hMutex);
    CloseHandle(hMutex);

}
