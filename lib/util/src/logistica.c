#include "sqlite3.h"
#include "logistica.h"
#include "log.h"
#include "almacenes_db.h"
#include "estructuras.h"
#include <math.h>
#include <time.h>
#include <stdlib.h>

#define RADIO_TIERRA_KM          6371.0
#define DEG_A_RAD(x)             ((x) * M_PI / 180.0)

#define VEL_MEDIA_KMH            75.0
#define CAPACIDAD_VEHICULO       25000.0
#define COSTE_KM_VEHICULO        1.4
#define COSTE_MIN_TRASVASE       150.0
#define HORAS_MIN_TRASVASE       2.0

#define COSTE_MANIPULACION_UND   0.05
#define MARGEN_GANANCIAS         0.5
#define HORAS_PREPARACION_EXT    8.0
#define DIAS_TRANSITO_EXT        2

// HAVERSINE

double calcularDistancia(Ubicacion u1, Ubicacion u2) {
    double lat1 = DEG_A_RAD(u1.latitud);
    double lat2 = DEG_A_RAD(u2.latitud);
    double dLat = DEG_A_RAD(u2.latitud - u1.latitud);
    double dLon = DEG_A_RAD(u2.longitud - u1.longitud);

    double a = sin(dLat / 2) * sin(dLat / 2) + cos(lat1) * cos(lat2) * sin(dLon / 2) * sin(dLon / 2);

    return RADIO_TIERRA_KM * 2.0 * asin(sqrt(a));
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
