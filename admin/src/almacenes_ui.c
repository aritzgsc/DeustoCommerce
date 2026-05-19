#include <admin_ui.h>
#include <almacenes_ui.h>
#include <catalogo_ui.h>
#include "logistica.h"
#include "finanzas.h"
#include "almacenes_db.h"
#include "estructuras.h"
#include "config.h"
#include "api.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utils_ui.h>

// AUXILIARES INTERNAS

// Barra de ocupación visual

static void imprimirBarraOcupacion(int ocupacion, int capacidad) {

    if (capacidad <= 0) { printf(C_GRIS "N/A" RESET); return; }

    double pct    = (double)ocupacion / capacidad;
    int    llenas = (int)(pct * 11);
    int    vacias = 11 - llenas;

    // Color según ocupación
    if (pct >= 0.9) printf(C_ROJO);
    else if (pct >= 0.7) printf(C_AMARILLO);
    else printf(C_VERDE);

    for (int i = 0; i < llenas; i++) printf("█");
    printf(C_GRIS);
    for (int i = 0; i < vacias; i++) printf("░");
    printf(RESET " " C_BLANCO "%.1f%%" RESET, pct * 100);

}

// SELECCIONAR ALMACEN

static Columna colsSelAlm[] = {

	{ "  ID  ", 6,  1 },
	{ "                       NOMBRE                       ", 52, 0 },
	{ "   OCUPACION   ", 15, 1 },
	{ "         CAP. MAX         ", 26, 2 },

};
#define N_COLS_SEL_ALM 4

static char* cmdsSelAlm[] = {

    "SELECCIONAR", "ANTERIOR", "SIGUIENTE", "VOLVER"

};

#define N_CMDS_SEL_ALM 4

int seleccionarAlmacen(sqlite3* db, int excluirId) {

    int pagina = 1;

    while (!salir) {

        int nAlm = 0;
        Almacen* almacenes = getAlmacenes(db, &nAlm);

        // Filtramos el excluido
        Almacen** filtrados = malloc(sizeof(Almacen*) * nAlm);
        int nFilt = 0;
        for (int i = 0; i < nAlm; i++) {
            if (almacenes[i].id != excluirId)
                filtrados[nFilt++] = &almacenes[i];
        }

        int totalPags = nFilt > 0 ? (nFilt + ALMS_SEL_POR_PAGINA - 1) / ALMS_SEL_POR_PAGINA : 1;

        imprimirCabecera("SELECCIONAR ALMACEN", "Elige el almacen destino");

        if (nFilt == 0) {
        	printf("\033[A");
            imprimirWarn("No hay otros almacenes disponibles.");
            free(filtrados);
            liberarAlmacenes(almacenes, nAlm);
            return -1;
        }

        imprimirCabeceraTabla(colsSelAlm, N_COLS_SEL_ALM);

        int inicio = (pagina - 1) * ALMS_SEL_POR_PAGINA;
        int fin = inicio + ALMS_SEL_POR_PAGINA < nFilt ? inicio + ALMS_SEL_POR_PAGINA : nFilt;

        for (int i = inicio; i < fin; i++) {
            Almacen* a = filtrados[i];
            int ocup = getOcupacionAlmacen(db, a->id);

            char idStr[7], capStr[27];
            snprintf(idStr, sizeof(idStr), "%d", a->id);
            snprintf(capStr, sizeof(capStr), "%d", a->capacidad);

            char nombre[53];
            strncpy(nombre, a->nombre, 52); nombre[52] = '\0';

            char ocupStr[256] = "";
            double pct = a->capacidad > 0 ? (double)ocup / a->capacidad : 0;
            int llenas = (int)(pct * 10);

            if (pct >= 0.9) strcat(ocupStr, C_ROJO);
            else if (pct >= 0.7) strcat(ocupStr, C_AMARILLO);
            else strcat(ocupStr, C_VERDE);

            for (int j = 0; j < llenas; j++) strcat(ocupStr, "█");
            strcat(ocupStr, C_GRIS);
            for (int j = llenas; j < 10;  j++) strcat(ocupStr, "░");
            char pctStr[32];
            if ((i - inicio) % 2 == 0){

            	if (pct == 1.0) snprintf(pctStr, sizeof(pctStr), RESET " %3d%%", (int)(pct * 100));
                else snprintf(pctStr, sizeof(pctStr), RESET "  %2d%%", (int)(pct * 100));

            } else {

                if (pct == 1.0) snprintf(pctStr, sizeof(pctStr), RESET C_GRIS " %3d%%" RESET, (int)(pct * 100));
                else snprintf(pctStr, sizeof(pctStr), RESET C_GRIS "  %2d%%" RESET, (int)(pct * 100));

            }

            strcat(ocupStr, pctStr);

            char* fila[] = { idStr, nombre, ocupStr, capStr };
            imprimirFilaTabla(fila, colsSelAlm, N_COLS_SEL_ALM, (i - inicio) % 2);
        }

        imprimirPieTabla(colsSelAlm, N_COLS_SEL_ALM);
        imprimirPaginacion(pagina, totalPags, nFilt);

        imprimirSeccion("COMANDOS");
        printf("  " ESTILO_CMD "SELECCIONAR [ID]     " RESET "  Elegir almacen\n");
        printf("  " ESTILO_CMD "ANTERIOR / SIGUIENTE " RESET "  Navegar paginas\n");
        printf("  " ESTILO_CMD "VOLVER               " RESET "  Cancelar\n\n");

        Entrada e = leerComando(cmdsSelAlm, N_CMDS_SEL_ALM, ">");

        int resultado = -2; // -2 = seguir en el bucle

        if (strcmp(e.comando, "SELECCIONAR") == 0 && strlen(e.arg1) > 0) {

            int idSel = atoi(e.arg1);
            if (idSel == excluirId) {

                imprimirError("No puedes seleccionar este almacen.");
                pausar();

            } else {

                for (int i = 0; i < nFilt; i++) {

                    if (filtrados[i]->id == idSel) {

                        resultado = idSel;
                        break;

                    }

                }
                if (resultado == -2) {

                    imprimirError("ID de almacen no valido.");
                    pausar();

                }

            }

        } else if (strcmp(e.comando, "SIGUIENTE") == 0) {

            pagina = pagina < totalPags ? pagina + 1 : 1;

        } else if (strcmp(e.comando, "ANTERIOR") == 0) {

            pagina = pagina > 1 ? pagina - 1 : totalPags;

        } else if (strcmp(e.comando, "VOLVER") == 0) {

            resultado = -1;

        } else {
            imprimirError("Comando no reconocido.");
            pausar();
        }

        free(filtrados);
        liberarAlmacenes(almacenes, nAlm);

        if (resultado != -2) return resultado;

    }

    return 0;

}

// ALMACENES

static Columna colsAlmacenes[] = {

    { "  ID  ", 6,  1 },
    { "               NOMBRE               ", 36, 0 },
    { "          DIRECCION          ", 29, 0 },
    { "   OCUPACION   ", 15, 1 },
    { " CAP. MAX ", 10, 1 },

};

#define N_COLS_ALM 5

static char* cmdsAlmacenes[] = {

    "VER_ALMACEN", "NUEVO_ALMACEN", "ANTERIOR", "SIGUIENTE", "HOME", "EXIT"

};

#define N_CMDS_ALM 6

void pantallaAlmacenes(sqlite3* db) {

    int pagina = 1;

    while (!salir) {

        int nAlm = 0;
        Almacen* almacenes = getAlmacenes(db, &nAlm);
        int totalPags = nAlm > 0 ? (nAlm + ALMS_POR_PAGINA - 1) / ALMS_POR_PAGINA : 1;

        imprimirCabecera("ALMACENES", "Vision global de la red logistica");

        if (!almacenes || nAlm == 0) {

        	printf("\033[A");
            imprimirWarn("No hay almacenes registrados.");

        } else {

            // Ocupación total de la red (siempre sobre todos)
            int capTotal  = 0;
            int ocupTotal = 0;
            for (int i = 0; i < nAlm; i++) {

                capTotal  += almacenes[i].capacidad;
                ocupTotal += getOcupacionAlmacen(db, almacenes[i].id);

            }

            printf("  " C_GRIS "Red logistica: " RESET);
            imprimirBarraOcupacion(ocupTotal, capTotal);
            printf("  " C_GRIS "(%d almacenes activos)\n\n" RESET, nAlm);

            imprimirCabeceraTabla(colsAlmacenes, N_COLS_ALM);

            int inicio = (pagina - 1) * ALMS_POR_PAGINA;
            int fin = inicio + ALMS_POR_PAGINA < nAlm ? inicio + ALMS_POR_PAGINA : nAlm;

            for (int i = inicio; i < fin; i++) {

                Almacen* a = &almacenes[i];
                int ocup = getOcupacionAlmacen(db, a->id);

                char idStr[8], capStr[12];
                snprintf(idStr,  sizeof(idStr), "%d", a->id);
                snprintf(capStr, sizeof(capStr), "%d", a->capacidad);

                char dir[29];
                strncpy(dir, a->ubicacion.direccion, 28);
                dir[28] = '\0';

                char ocupStr[256] = "";
                double pct = a->capacidad > 0 ? (double)ocup / a->capacidad : 0;
                int llenas = (int)(pct * 10);

                if (pct >= 0.9) strcat(ocupStr, C_ROJO);
                else if (pct >= 0.7) strcat(ocupStr, C_AMARILLO);
                else strcat(ocupStr, C_VERDE);

                for (int j = 0; j < llenas; j++) strcat(ocupStr, "█");
                strcat(ocupStr, C_GRIS);
                for (int j = llenas; j < 10;  j++) strcat(ocupStr, "░");
                char pctStr[32];
                if ((i - inicio) % 2 == 0) {
                	if (pct >= 1.0) snprintf(pctStr, sizeof(pctStr), RESET " %3d%%", (int)(pct * 100));
                	else snprintf(pctStr, sizeof(pctStr), RESET "  %2d%%", (int)(pct * 100));
                } else {
                	if (pct >= 1.0) snprintf(pctStr, sizeof(pctStr), RESET C_GRIS " %3d%%" RESET, (int)(pct * 100));
                	else snprintf(pctStr, sizeof(pctStr), RESET C_GRIS "  %2d%%" RESET, (int)(pct * 100));
                }

                strcat(ocupStr, pctStr);

                char nombre[31];
                strncpy(nombre, a->nombre, 30);
                nombre[30] = '\0';

                char* fila[] = { idStr, nombre, dir, ocupStr, capStr };
                imprimirFilaTabla(fila, colsAlmacenes, N_COLS_ALM, (i - inicio) % 2);

            }

            imprimirPieTabla(colsAlmacenes, N_COLS_ALM);
            imprimirPaginacion(pagina, totalPags, nAlm);

            liberarAlmacenes(almacenes, nAlm);

        }

        imprimirSeccion("COMANDOS");
        printf("  " ESTILO_CMD "VER_ALMACEN [ID]       " RESET "Gestionar stock de un almacen\n");
        printf("  " ESTILO_CMD "NUEVO_ALMACEN          " RESET "Inaugurar nueva sede\n");
        printf("  " ESTILO_CMD "ANTERIOR / SIGUIENTE   " RESET "Navegar paginas\n");
        printf("  " ESTILO_CMD "HOME                   " RESET "Volver al panel\n\n");

        Entrada e = leerComando(cmdsAlmacenes, N_CMDS_ALM, ">");

        if (strcmp(e.comando, "VER_ALMACEN") == 0 && strlen(e.arg1) > 0) {

            int home = pantallaVerAlmacen(db, atoi(e.arg1));
            if (home == 1) return;

        } else if (strcmp(e.comando, "NUEVO_ALMACEN") == 0) {

            pantallaNuevoAlmacen(db);

        } else if (strcmp(e.comando, "SIGUIENTE") == 0) {

            if (nAlm > 0) {
                if (pagina < totalPags) pagina++;
                else pagina = 1;
            }

        } else if (strcmp(e.comando, "ANTERIOR") == 0) {

            if (nAlm > 0) {
                if (pagina > 1) pagina--;
                else pagina = totalPags;
            }

        } else if (strcmp(e.comando, "HOME") == 0) {

            return;

        } else if (strcmp(e.comando, "EXIT") == 0) {

        	salir = 1;
        	return;

    	} else {

            imprimirError("Comando no reconocido.");
            pausar();

        }

    }

}

// VER ALMACÉN

static Columna colsStock[] = {

    { "  ID  ", 6,  1 },
    { "                       NOMBRE                       ", 52, 0 },
    { "  VARIANTE  ", 12, 2 },
    { "CANTIDAD", 8, 1 },
    { "      ESTADO      ", 18, 1 },

};

#define N_COLS_STOCK 5

static char* cmdsVerAlmacen[] = {

    "VER_PROD", "ADD_STOCK", "MOVER_STOCK", "RESTOCK", "ELIMINAR", "ANTERIOR", "SIGUIENTE", "VOLVER", "HOME", "EXIT"

};

#define N_CMDS_VER_ALM 10

int pantallaVerAlmacen(sqlite3* db, int idAlm) {

    int pagina = 1;
    int total  = 0;

    while (!salir) {

        Almacen* a = getAlmacenPorId(db, idAlm);

        if (!a) {

            imprimirError("Almacén no encontrado.");
            pausar();
            return 0;

        }

        int ocupacion = getOcupacionAlmacen(db, idAlm);

        char subtitulo[128];
        snprintf(subtitulo, sizeof(subtitulo), "%s · %s", a->nombre, a->ubicacion.direccion);

        imprimirCabecera("GESTIÓN DE ALMACÉN", subtitulo);

        // Info del almacén
        printf("  " ESTILO_SUBTITULO "Ocupación:  " RESET);
        imprimirBarraOcupacion(ocupacion, a->capacidad);
        printf("  " C_GRIS "(%d / %d uds.)\n\n" RESET, ocupacion, a->capacidad);

        // Stock paginado
        a->productos = getStockAlmacen(db, idAlm, pagina, &total);
        int totalPags = total > 0? (total + ITEMS_POR_PAGINA - 1) / ITEMS_POR_PAGINA : 1;

        if (!a->productos || total == 0) {

        	imprimirWarn("Este almacén no tiene stock registrado.");

        } else {

            imprimirCabeceraTabla(colsStock, N_COLS_STOCK);

            int n = total < ITEMS_POR_PAGINA ? total : ITEMS_POR_PAGINA;
            for (int i = 0; i < n; i++) {

                char idStr[7], cantStr[9];
                snprintf(idStr, sizeof(idStr), "%d", a->productos[i].producto.id);
                snprintf(cantStr, sizeof(cantStr), "%d", a->productos[i].cantidad);

                char nombre[53];
                strncpy(nombre, a->productos[i].producto.nombre, 52);
                nombre[52] = '\0';

                char* estado = a->productos[i].disponible ? "✔   Disponible" : "✖   No disponible";

                char* fila[] = { idStr, nombre, a->productos[i].producto.variante, cantStr, estado };
                imprimirFilaTabla(fila, colsStock, N_COLS_STOCK, i % 2);

            }

            imprimirPieTabla(colsStock, N_COLS_STOCK);
            imprimirPaginacion(pagina, totalPags, total);
            liberarStock(a->productos, n);

        }

        imprimirSeccion("COMANDOS");
        printf("  " ESTILO_CMD "VER_PROD [ID]        " RESET "Ver detalle de producto\n");
        printf("  " ESTILO_CMD "ADD_STOCK            " RESET "Entrada manual de mercancía\n");
        printf("  " ESTILO_CMD "MOVER_STOCK          " RESET "Trasvase a otro almacén\n");
        printf("  " ESTILO_CMD "RESTOCK              " RESET "Rellenar hasta el 80%%\n");
        printf("  " ESTILO_CMD "ELIMINAR             " RESET "Cerrar almacén\n");
        printf("  " ESTILO_CMD "ANTERIOR/SIGUIENTE   " RESET "Navegar páginas\n");
        printf("  " ESTILO_CMD "VOLVER               " RESET "Volver a almacenes\n\n");

        liberarAlmacen(a);

        Entrada e = leerComando(cmdsVerAlmacen, N_CMDS_VER_ALM, ">");

        if (strcmp(e.comando, "VER_PROD") == 0 && strlen(e.arg1) > 0) {

            int home = pantallaVerProducto(db, atoi(e.arg1), idAlm);
            if (home) return 1;

        } else if (strcmp(e.comando, "ADD_STOCK") == 0) {

            pantallaAddStock(db, idAlm);

        } else if (strcmp(e.comando, "MOVER_STOCK") == 0) {

            pantallaMoverStock(db, idAlm);

        } else if (strcmp(e.comando, "RESTOCK") == 0) {

            pantallaRestock(db, idAlm);

        } else if (strcmp(e.comando, "ELIMINAR") == 0) {

            pantallaEliminarAlmacen(db, idAlm);
            return 0;

        } else if (strcmp(e.comando, "SIGUIENTE") == 0) {

            if (pagina < totalPags) pagina++;
            else pagina = 1;

        } else if (strcmp(e.comando, "ANTERIOR") == 0) {

            if (pagina > 1) pagina--;
            else pagina = totalPags;

        } else if (strcmp(e.comando, "VOLVER") == 0) {

            return 0;

        } else if (strcmp(e.comando, "HOME") == 0) {

            return 1;

        } else if (strcmp(e.comando, "EXIT") == 0) {

            salir = 1;
            return 0;

        } else {

            imprimirError("Comando no reconocido.");
            pausar();

        }

    }

    return 0;

}

// NUEVO ALMACÉN

void pantallaNuevoAlmacen(sqlite3* db) {

    imprimirCabecera("NUEVO ALMACÉN", "Introduce los datos de la nueva sede");

    Almacen a = {0};
    char nombre[256], pais[128], ciudad[128], direccion[256];

    leerTexto("Nombre del almacén:", nombre, sizeof(nombre));

    if (strlen(nombre) == 0) {

        imprimirError("El nombre no puede estar vacío.");
        pausar();
        return;

    }

    a.nombre = nombre;

    int result;

    do {

    	leerTexto("Pais:", pais, sizeof(pais));
    	leerTexto("Ciudad:", ciudad, sizeof(ciudad));
    	leerTexto("Dirección:", direccion, sizeof(direccion));

    	result = completarUbicacion(&a.ubicacion, pais, ciudad, direccion);

    	if (result) {

    		imprimirError("No se ha podido verificar la ubicación");
    		LOG_ERROR("No se ha podido verificar la ubicación: %s, %s, %s", direccion, ciudad, pais);

    	}

    } while (result);

    a.capacidad = leerEntero("Capacidad máxima (unidades):", 1000, 10000000);

    if (confirmar("¿Confirmar creación del almacén?")) {

        int id = crearAlmacen(db, a);
        if (id > 0) {

            char msg[64];
            snprintf(msg, sizeof(msg), "Almacén creado con ID #%d", id);
            imprimirExito(msg);
            LOG_INFO("Nuevo almacén creado: #%d | %s, %s con %d uds. de capacidad", id, a.ubicacion.ciudad.nombre, a.ubicacion.ciudad.pais.nombre, a.capacidad);

        } else {

            imprimirError("Error al crear el almacén.");
            LOG_ERROR("Error al crear almacén en %s, %s con %d uds. de capacidad", a.ubicacion.ciudad.nombre, a.ubicacion.ciudad.pais.nombre, a.capacidad);

        }

        pausar();

    }

    liberarUbicacionApi(&a.ubicacion);

}

// ADD STOCK

void pantallaAddStock(sqlite3* db, int idAlm) {

    char subtitulo[64];
    snprintf(subtitulo, sizeof(subtitulo), "Anadir stock al almacen #%d", idAlm);
    imprimirCabecera("ENTRADA DE MERCANCIA", subtitulo);

    Almacen* a = getAlmacenPorId(db, idAlm);
    int ocupacion = getOcupacionAlmacen(db, a->id);

    if (ocupacion >= a->capacidad) {
    	printf("\033[A");
    	imprimirWarn("El almacén ya está lleno");
    	LOG_WARN("Adición de stock imposible en #%d | %s, %s || Almacén lleno", a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre);
    	pausar();
    	liberarAlmacen(a);
    	return;
    }

    // Seleccionamos producto con el catálogo

    char variante[64] = {0};
    int idProd = seleccionarProducto(db, variante, sizeof(variante), -1);
    if (idProd == -1) {

        imprimirWarn("Operacion cancelada.");
    	LOG_WARN("Adición de stock cancelada en #%d | %s, %s", a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre);
        pausar();
        liberarAlmacen(a);
        return;

    }

    // Obtenemos el producto para saber su categoría y variantes
    Producto* p = getProductoPorId(db, idProd);
    if (!p) {

        imprimirError("Producto no encontrado.");
    	LOG_ERROR("Adición de stock cancelada: Producto #%d no encontrado", idProd);
        pausar();
        liberarAlmacen(a);
        liberarProducto(p);
        return;

    }

    char displayNom[1024] = "";
    wordWrap(displayNom, p->nombre, 13);
    char displayCat[1024] = "";
    wordWrap(displayCat, p->categoria.nombre, 13);

    // Reimprimimos cabecera con el producto seleccionado
    imprimirCabecera("ENTRADA DE MERCANCIA", subtitulo);
    printf("  " ESTILO_SUBTITULO "Producto:  " RESET C_BLANCO "#%d — %s\n" RESET, p->id, displayNom);
    printf("  " ESTILO_SUBTITULO "Categoria: " RESET C_GRIS "%s\n\n" RESET, displayCat);

    // Cantidad
    int max = a->capacidad - ocupacion;
    int cant = leerEntero("Cantidad a anadir (0: MÁXIMO):", -1, max);

    if (cant == -1) {

        imprimirWarn("Operacion cancelada.");
        LOG_WARN("Adición de stock cancelada en #%d | %s, %s", a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre);
        pausar();
        liberarAlmacen(a);
        liberarProducto(p);
        return;

    }

    if (cant == 0) cant = max;

    ProdCant prod = { *p , cant };
    double coste;
    time_t duracion;

    calcularCosteCompraExterno(&prod, 1, &coste, &duracion);

    // Coste simulado
    printf("\n  " C_GRIS "Coste estimado de aprovisionamiento: " RESET ESTILO_PRECIO "%.2f €" RESET, coste);
    printf("\n  " C_GRIS "Duración estimada de aprovisionamiento: " RESET ESTILO_PRECIO); imprimirDuracion(duracion); printf(RESET);

    // Resumen
    imprimirSeccion("RESUMEN");
    printf("  " ESTILO_SUBTITULO "Producto:  " RESET C_BLANCO "#%d — %s\n" RESET, p->id, displayNom);
    printf("  " ESTILO_SUBTITULO "Variante:  " RESET C_BLANCO "%s\n"       RESET, variante);
    printf("  " ESTILO_SUBTITULO "Cantidad:  " RESET C_BLANCO "%d uds.\n"  RESET, cant);
    printf("  " ESTILO_SUBTITULO "Coste est: " RESET ESTILO_PRECIO "%.2f € ; " RESET, coste); imprimirDuracion(duracion); printf(RESET);

    time_t timestampEjecucion = time(NULL) + duracion;

    if (confirmar("Confirmar entrada de stock?")) {
        if (addStock(db, idAlm, idProd, variante, cant, timestampEjecucion) == 0) {

            char msg[64];
            snprintf(msg, sizeof(msg), "%d unidades añadidas correctamente.", cant);
            imprimirExito(msg);
            LOG_INFO("Stock añadido correctamente: #%d — %s || %d uds. => #%d | %s, %s || %.2f €", p->id, p->nombre, cant, a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre, coste);

            char regFinancieroPath[256];
            configGet(CONFIG_PATH, "REG_FINANCIERO_PATH", regFinancieroPath, sizeof(regFinancieroPath));

            char concepto[512];
            snprintf(concepto, sizeof(concepto), "Stock añadido: #%d — %s || %d uds. => #%d | %s, %s", p->id, p->nombre, cant, a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre);
            registrarTransaccion(regFinancieroPath, "GASTO", "COMPRA_PROVEEDOR", concepto, coste);

        } else {

            imprimirError("Error al anadir stock.");
            LOG_ERROR("Error al añadir stock: #%d — %s || %d uds. => #%d | %s, %s", p->id, p->nombre, cant, a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre);

        }
        pausar();
    }

    liberarProducto(p);
    liberarAlmacen(a);

}

// MOVER STOCK

void pantallaMoverStock(sqlite3* db, int idAlm) {

    imprimirCabecera("TRASVASE DE STOCK", "Mover mercancia a otro almacen");

    // Seleccionamos almacén destino (excluimos el actual)

    Almacen* orig = getAlmacenPorId(db, idAlm);

    int idDestino = seleccionarAlmacen(db, idAlm);
    if (idDestino == -1) {

        imprimirWarn("Operacion cancelada.");
    	LOG_WARN("Operación de trasvase cancelada en #%d | %s, %s", orig->id, orig->ubicacion.ciudad.nombre, orig->ubicacion.ciudad.pais.nombre);
        pausar();
        liberarAlmacen(orig);
        return;

    }

    Almacen* dest = getAlmacenPorId(db, idDestino);
    int ocupacion = getOcupacionAlmacen(db, dest->id);

    // Seleccionamos producto

    char variante[64] = "";
    int idProd = seleccionarProducto(db, variante, sizeof(variante), idAlm);
    if (idProd == -1) {

        imprimirWarn("Operacion cancelada.");
    	LOG_WARN("Operación de trasvase cancelada: #%d | %s, %s -> #%d | %s, %s", orig->id, orig->ubicacion.ciudad.nombre, orig->ubicacion.ciudad.pais.nombre, dest->id, dest->ubicacion.ciudad.nombre, dest->ubicacion.ciudad.pais.nombre);
        pausar();
        liberarAlmacen(orig);
        liberarAlmacen(dest);
        return;

    }

    Producto* p = getProductoPorId(db, idProd);

    if (!p) {

        imprimirError("Producto no encontrado.");
        LOG_ERROR("Producto #%d no encontrado, trasvase cancelado: #%d | %s, %s -> #%d | %s, %s", idProd, orig->id, orig->ubicacion.ciudad.nombre, orig->ubicacion.ciudad.pais.nombre, dest->id, dest->ubicacion.ciudad.nombre, dest->ubicacion.ciudad.pais.nombre);
        pausar();
        liberarAlmacen(orig);
        liberarAlmacen(dest);
        liberarProducto(p);
        return;

    }

    char displayNom[1024] = "";
    wordWrap(displayNom, p->nombre, 13);

    imprimirCabecera("TRASVASE DE STOCK", "Mover mercancia a otro almacen");
    printf("  " ESTILO_SUBTITULO "Origen:    " RESET C_BLANCO "Almacen #%d\n"     RESET, idAlm);
    printf("  " ESTILO_SUBTITULO "Destino:   " RESET C_BLANCO "Almacen #%d\n"     RESET, idDestino);
    printf("  " ESTILO_SUBTITULO "Producto:  " RESET C_BLANCO "#%d — %s\n\n"      RESET, p->id, displayNom);

    // Cantidad
    int max = getStockProdAlm(db, idAlm, idProd, variante);
    max = min(max, dest->capacidad - ocupacion);
    int cant = leerEntero("Cantidad a mover (0: TODO):", -1, max);

    if (cant == -1) {

    	imprimirWarn("Operacion cancelada.");
    	LOG_WARN("Operación de trasvase cancelada: #%d | %s, %s -> #%d | %s, %s || #%d — %s", orig->id, orig->ubicacion.ciudad.nombre, orig->ubicacion.ciudad.pais.nombre, dest->id, dest->ubicacion.ciudad.nombre, dest->ubicacion.ciudad.pais.nombre, p->id, p->nombre);
        pausar();
        liberarAlmacen(orig);
        liberarAlmacen(dest);
        liberarProducto(p);
        return;

    }

    if (cant == 0) cant = max;

    // Coste del trasvase
    double coste = 0;
    time_t duracion = {0};
    calcularCosteTrasvase(orig->ubicacion, dest->ubicacion, cant, &coste, &duracion);
    printf("\n  " C_GRIS "Coste estimado del trasvase: " RESET ESTILO_PRECIO "%.2f €" RESET, coste);
    printf("\n  " C_GRIS "Duración estimada del trasvase: " RESET ESTILO_PRECIO); imprimirDuracion(duracion); printf(RESET);

    // Resumen
    imprimirSeccion("RESUMEN");
    printf("  " ESTILO_SUBTITULO "Producto:  " RESET C_BLANCO "#%d — %s\n" RESET, p->id, displayNom);
    printf("  " ESTILO_SUBTITULO "Variante:  " RESET C_BLANCO "%s\n"       RESET, variante);
    printf("  " ESTILO_SUBTITULO "Cantidad:  " RESET C_BLANCO "%d uds.\n"  RESET, cant);
    printf("  " ESTILO_SUBTITULO "Origen:    " RESET C_BLANCO "Almacen #%d\n" RESET, idAlm);
    printf("  " ESTILO_SUBTITULO "Destino:   " RESET C_BLANCO "Almacen #%d\n" RESET, idDestino);
    printf("  " ESTILO_SUBTITULO "Coste est: " RESET ESTILO_PRECIO "%.2f €\n\n" RESET, coste);


    if (confirmar("Confirmar trasvase?")) {

    	time_t timestampEjecucion = time(NULL) + duracion;

        int cod = moverStock(db, idAlm, idDestino, idProd, variante, cant, timestampEjecucion);
        if (cod == 0) {

            imprimirExito("Stock trasvasado correctamente.");
            LOG_INFO("Stock trasvasado correctamente: #%d | %s, %s -> #%d | %s, %s || %d unidades de #%d — %s || %.2f €", orig->id, orig->ubicacion.ciudad.nombre, orig->ubicacion.ciudad.pais.nombre, dest->id, dest->ubicacion.ciudad.nombre, dest->ubicacion.ciudad.pais.nombre, cant, p->id, p->nombre, coste);

            char regFinancieroPath[256];
            configGet(CONFIG_PATH, "REG_FINANCIERO_PATH", regFinancieroPath, sizeof(regFinancieroPath));

            char concepto[512];
            snprintf(concepto, sizeof(concepto), "Stock trasvasado: #%d | %s, %s -> #%d | %s, %s || %d unidades de #%d — %s", orig->id, orig->ubicacion.ciudad.nombre, orig->ubicacion.ciudad.pais.nombre, dest->id, dest->ubicacion.ciudad.nombre, dest->ubicacion.ciudad.pais.nombre, cant, p->id, p->nombre);
            registrarTransaccion(regFinancieroPath, "GASTO", "TRASVASE_INTERNO", concepto, coste);

        } else if (cod == -10) {

            imprimirError("Error: stock insuficiente en origen.");
            LOG_ERROR("Stock insuficiente de #%d — %s para trasvase en: #%d — %s, %s", p->id, p->nombre, orig->id, orig->ubicacion.ciudad.nombre, orig->ubicacion.ciudad.pais.nombre);

        } else {

        	imprimirError("Error: almacen no encontrado.");
        	LOG_ERROR("Almacén no encontrado para trasvase: #%d", idDestino);

        }
        pausar();

    }

    liberarAlmacen(orig);
    liberarAlmacen(dest);
    liberarProducto(p);

}

// RESTOCK

void pantallaRestock(sqlite3* db, int idAlm) {

	char subtitulo[128];
	snprintf(subtitulo, sizeof(subtitulo), "Rellenar almacén (%d) hasta el 80%% de capacidad", idAlm);
    imprimirCabecera("RESTOCK AUTOMÁTICO", subtitulo);

    Almacen* a = getAlmacenPorId(db, idAlm);
    if (!a) {

    	printf("\033[A");
    	imprimirError("Almacén no encontrado.");
    	LOG_ERROR("Restock de almacén cancelada: Almacén #%d no encontrado", idAlm);
    	pausar();
    	liberarAlmacen(a);
    	return;

    }

    int ocupacion = getOcupacionAlmacen(db, idAlm);
    int objetivo  = (int)(a->capacidad * 0.8);

    printf("  Ocupación actual:  ");
    imprimirBarraOcupacion(ocupacion, a->capacidad);
    printf("\n");

    if (ocupacion >= objetivo) {

        imprimirInfo("El almacén ya supera el 80% de ocupación. No es necesario restock.");
        LOG_WARN("El almacén ya supera el 80% de ocupación: #%d — %s, %s", a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre);
        pausar();
        liberarAlmacen(a);
        return;

    }

    printf("  Objetivo (80%%):    " ESTILO_PRECIO "%d uds.\n" RESET, objetivo);
    printf("  Unidades a añadir: " C_CYAN "%d uds.\n" RESET, ocupacion < objetivo ? objetivo - ocupacion : 0);

    double coste = 0.0;
    time_t duracion = 0;
    calcularCosteRestock(db, idAlm, &coste, &duracion);

    printf("\n  " C_GRIS "Coste estimado del restock: " RESET ESTILO_PRECIO "%.2f €" RESET, coste);
    printf("\n  " C_GRIS "Tiempo estimado: " RESET C_BLANCO); imprimirDuracion(duracion); printf(RESET);

    if (confirmar("¿Ejecutar restock automático?")) {

    	time_t timestampEjecucion = time(NULL) + duracion;

        int anadido = restock(db, idAlm, &coste, timestampEjecucion);
        if (anadido >= 0) {

            char msg[64];
            snprintf(msg, sizeof(msg), "Restock completado: %d unidades añadidas.", anadido);
            LOG_INFO("Restock completado en en #%d — %s, %s || %d unidades añadidas || %.2f €", a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre, anadido, coste);
            imprimirExito(msg);

            char regFinancieroPath[256];
            configGet(CONFIG_PATH, "REG_FINANCIERO_PATH", regFinancieroPath, sizeof(regFinancieroPath));

            char concepto[512];
            snprintf(concepto, sizeof(concepto), "Restock completado: #%d — %s, %s || %d unidades añadidas", a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre, anadido);
            registrarTransaccion(regFinancieroPath, "GASTO", "RESTOCK_ALMACEN", concepto, coste);

        } else {

            imprimirError("Error durante el restock.");
            LOG_ERROR("Error durante el restock en #%d — %s, %s", a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre);

        }
        pausar();

    }

    liberarAlmacen(a);

}

// ELIMINAR ALMACÉN

void pantallaEliminarAlmacen(sqlite3* db, int idAlm) {

    Almacen* a = getAlmacenPorId(db, idAlm);
    if (!a) {

    	imprimirError("Almacén no encontrado.");
    	LOG_ERROR("Eliminación de almacén cancelada: Almacén #%d no encontrado", idAlm);
    	pausar();
    	liberarAlmacen(a);
    	return;

    }

    imprimirCabecera("CERRAR ALMACÉN", a->nombre);

    int ocupacion = getOcupacionAlmacen(db, idAlm);

    printf("  " ESTILO_ERROR "Esta acción cerrará permanentemente el almacén.\n" RESET);
    printf("  El stock existente (%d uds.) será reubicado automáticamente\n", ocupacion);
    printf("  en el almacén más cercano con capacidad disponible.\n\n");

    printf("  " ESTILO_ID "Almacén: #%d — %s\n" RESET, a->id, a->nombre);
    printf("  " ESTILO_ID "Ubicación: %s\n" RESET, a->ubicacion.direccion);

    double coste = 0.0;
    time_t duracion = 0;
    calcularCosteCierreAlmacen(db, idAlm, &coste, &duracion);

    printf("  " C_GRIS "Coste estimado de reubicacion: " RESET ESTILO_PRECIO "%.2f €\n" RESET, coste);
    printf("  " C_GRIS "Tiempo estimado del operativo: " RESET C_BLANCO); imprimirDuracion(duracion); printf(RESET);

    if (confirmar("¿Confirmar cierre del almacén?")) {

        imprimirInfo("Reubicando stock... esto puede tardar unos segundos.");
        fflush(stdout);

        time_t timestampEjecucion = time(NULL) + duracion;

        if (eliminarAlmacen(db, idAlm, timestampEjecucion) == 0) {

            imprimirExito("Almacén cerrado y stock reubicado correctamente.");
            LOG_INFO("Almacén cerrado: #%d — %s, %s || Stock reubicado correctamente || %.2f €", a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre, coste);

            char regFinancieroPath[256];
            configGet(CONFIG_PATH, "REG_FINANCIERO_PATH", regFinancieroPath, sizeof(regFinancieroPath));

            char concepto[512];
            snprintf(concepto, sizeof(concepto), "Almacén cerrado: #%d — %s, %s || Stock reubicado correctamente", a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre);
            registrarTransaccion(regFinancieroPath, "GASTO", "CIERRE_ALMACEN", concepto, coste);

        } else {

            imprimirError("Error al cerrar el almacén.");
            LOG_ERROR("Error al cerrar el almacén: #%d — %s, %s", a->id, a->ubicacion.ciudad.nombre, a->ubicacion.ciudad.pais.nombre);

        }

        pausar();

    }

    liberarAlmacen(a);

}
