#include "ui_admin.h"
#include "ui_informes.h"
#include "ui_utils.h"
#include "informes_db.h"
#include "finanzas.h"
#include "config.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// AUXILIARES INTERNAS

static time_t parsearFecha(char* str) {

    struct tm tm = {0};
    if (sscanf(str, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) != 3)  return 0;
    tm.tm_year -= 1900;
    tm.tm_mon  -= 1;
    return mktime(&tm);

}

// INFORMES HOME

static char* cmdsInformes[] = {

    "BALANCE", "DEAD_STOCK", "TOP_VENTAS", "HOME", "EXIT"

};

#define N_CMDS_INF 5

void pantallaInformes(sqlite3* db) {

	char rutaCsv[256];
	char rutaReports[256];

	configGet(CONFIG_PATH, "REG_FINANCIERO_PATH", rutaCsv, sizeof(rutaCsv));
	configGet(CONFIG_PATH, "REPORTS_PATH", rutaReports, sizeof(rutaReports));

    while (!salir) {

        imprimirCabecera("INFORMES", "Generación de reportes y análisis");

        imprimirSeccion("INFORMES DISPONIBLES");
        printf("  " ESTILO_CMD "BALANCE    " RESET "  Informe financiero (Ingresos vs Gastos) → .xlsx\n");
        printf("  " ESTILO_CMD "DEAD_STOCK " RESET "  Detección de productos estancados → .csv\n");
        printf("  " ESTILO_CMD "TOP_VENTAS " RESET "  Ranking histórico de ventas → .csv\n");
        printf("  " ESTILO_CMD "HOME       " RESET "  Volver al panel\n\n");

        Entrada e = leerComando(cmdsInformes, N_CMDS_INF, ">");

        if (strcmp(e.comando, "BALANCE") == 0) {

            int home = pantallaBalance(db, rutaCsv, rutaReports);
            if (home) return;

        } else if (strcmp(e.comando, "DEAD_STOCK") == 0) {

            int home = pantallaDeadStock(db, rutaReports);
            if (home) return;

        } else if (strcmp(e.comando, "TOP_VENTAS") == 0) {

            int home = pantallaTopVentas(db, rutaReports);
            if (home) return;

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

// BALANCE

static char* cmdsBalance[] = {

    "EXPORTAR", "VOLVER", "HOME", "EXIT"

};

#define N_CMDS_BAL 4

int pantallaBalance(sqlite3* db, char* rutaCsv, char* rutaReports) {

    char fechaIniStr[16] = {0};
    char fechaFinStr[16] = {0};

    printf("  " ESTILO_HINT "Deja en blanco para incluir todo el historial.\n\n" RESET);
    leerTexto("Fecha inicio (YYYY-MM-DD):", fechaIniStr, sizeof(fechaIniStr));
    leerTexto("Fecha fin    (YYYY-MM-DD):", fechaFinStr, sizeof(fechaFinStr));

    time_t fIni = strlen(fechaIniStr) > 0 ? parsearFecha(fechaIniStr) : 0;
    time_t fFin = strlen(fechaFinStr) > 0 ? parsearFecha(fechaFinStr) : 0;

    int n = 0;
    BalanceItem* items = getBalance(rutaCsv, fIni, fFin, &n);

    imprimirCabecera("BALANCE FINANCIERO", "Resumen del periodo");

    if (!items || n == 0) {

        imprimirWarn("No hay registros en el periodo seleccionado.");
        pausar();
        return 0;

    }

    double totalIngresos = 0, totalGastos = 0;
    calcularTotalesBalance(items, n, &totalIngresos, &totalGastos);
    double beneficio = totalIngresos - totalGastos;

    imprimirSeccion("RESUMEN");
    printf("  " ESTILO_SUBTITULO "Periodo:         " RESET C_BLANCO "%s  ->  %s\n" RESET, strlen(fechaIniStr) > 0 ? fechaIniStr : "inicio", strlen(fechaFinStr) > 0 ? fechaFinStr : "hoy");
    printf("  " ESTILO_SUBTITULO "Registros:       " RESET C_BLANCO "%d transacciones\n" RESET, n);
    printf("  " ESTILO_SUBTITULO "Total ingresos:  " RESET ESTILO_EXITO "%.2f EUR\n" RESET, totalIngresos);
    printf("  " ESTILO_SUBTITULO "Total gastos:    " RESET ESTILO_ERROR "%.2f EUR\n" RESET, totalGastos);
    printf("  " ESTILO_SUBTITULO "Beneficio neto:  " RESET);
    if (beneficio >= 0) printf(ESTILO_EXITO "%.2f EUR\n" RESET, beneficio);
    else printf(ESTILO_ERROR "%.2f EUR\n" RESET, beneficio);

    imprimirSeccion("ULTIMAS TRANSACCIONES");
    int inicio = n > 10 ? n - 10 : 0;
    for (int i = inicio; i < n; i++) {
        int esIngreso = strncmp(items[i].tipo, "INGRESO", 7) == 0;
        printf("  " C_GRIS "%s" RESET "  %s%-10s" RESET "  %-35s  %s%.2f EUR\n" RESET, items[i].fecha, esIngreso ? C_VERDE : C_ROJO, items[i].tipo, items[i].concepto, esIngreso ? ESTILO_EXITO : ESTILO_ERROR, items[i].importe);
    }

    imprimirSeccion("COMANDOS");
    printf("  " ESTILO_CMD "EXPORTAR  " RESET "  Guardar informe en .xlsx\n");
    printf("  " ESTILO_CMD "VOLVER    " RESET "  Volver a informes\n\n");

    while (1) {

        Entrada e = leerComando(cmdsBalance, N_CMDS_BAL, ">");

        if (strcmp(e.comando, "EXPORTAR") == 0) {

            time_t ahora = time(NULL);
            struct tm* tm = localtime(&ahora);
            char nombreFich[256];
            snprintf(nombreFich, sizeof(nombreFich), "%sbalance_%04d%02d%02d.xlsx", rutaReports, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

            imprimirInfo("Generando Excel...");
            if (generarExcelBalance(items, n, totalIngresos, totalGastos, nombreFich) == 0) {

                char msg[512];
                snprintf(msg, sizeof(msg), "Exportado a: %s", nombreFich);
                imprimirExito(msg);
                LOG_INFO("Balance exportado a: %s (ingresos=%.2f, gastos=%.2f)", nombreFich, totalIngresos, totalGastos);
                pausar();
                break;

            } else {

                imprimirError("Error al generar el fichero Excel.");
                LOG_ERROR("Fallo al generar Excel de balance: %s", nombreFich);

            }

        } else if (strcmp(e.comando, "VOLVER") == 0) {

            break;

        } else if (strcmp(e.comando, "HOME") == 0) {

            free(items);
            return 1;

        } else if (strcmp(e.comando, "EXIT") == 0) {

            free(items);
            salir = 1;
            return 1;

        } else {

            imprimirError("Comando no reconocido.");
            pausar();

        }
    }

    free(items);
    return 0;

}

// DEAD STOCK

static char* cmdsDeadStock[] = {

    "EXPORTAR", "VOLVER", "HOME", "EXIT", "ANTERIOR", "SIGUIENTE"
};

#define N_CMDS_DS 6

static Columna colsDeadStock[] = {

    { "  ID  ",         6,  1 },
	{ "              PRODUCTO              ", 36, 0 },
    { "          ALMACEN          ",   27,  0 },
    { " VARIANTE ",  10,  0 },
    { "CANTIDAD",      8,  1 },

};

#define N_COLS_DS 5

int pantallaDeadStock(sqlite3* db, char* rutaReports) {

    int idAlm = -1;  // -1 = todos los almacenes

    // Preguntamos si filtrar por almacén

    printf("\n  " ESTILO_HINT "¿Analizar un almacén concreto? (0: TODOS): " RESET);
    idAlm = leerEntero("", 0, 999999);
    if (idAlm == 0) idAlm = -1;

    int n = 0;
    ProductoDeadStock* items = getDeadStock(db, idAlm, &n);

    // Paginación

    int pagina = 1;
    int totalPags = (n + ITEMS_POR_PAGINA - 1) / ITEMS_POR_PAGINA;

    while (1) {

    	imprimirCabecera("DEAD STOCK", idAlm == -1 ? "Todos los almacenes" : "Almacén específico");

		if (!items || n == 0) {

			imprimirExito("¡No hay dead stock! Todos los productos tienen ventas registradas.");
			pausar();
			return 0;

		}

		char subtitulo[64];
		snprintf(subtitulo, sizeof(subtitulo), "%d productos sin movimiento detectados", n);
		imprimirWarn(subtitulo);

        imprimirCabeceraTabla(colsDeadStock, N_COLS_DS);

        int inicio = (pagina - 1) * ITEMS_POR_PAGINA;
        int fin = inicio + ITEMS_POR_PAGINA < n ? inicio + ITEMS_POR_PAGINA : n;

        for (int i = inicio; i < fin; i++) {

            char idStr[8], cantStr[8];
            snprintf(idStr,  sizeof(idStr), "%d", items[i].idProd);
            snprintf(cantStr, sizeof(cantStr), "%d", items[i].cantStock);

            char nombre[37];
            strncpy(nombre, items[i].nombreProd, 36);
            nombre[36] = '\0';

            char almacen[28];
            strncpy(almacen, items[i].nombreAlm, 27);
            almacen[27] = '\0';

            char* fila[] = { idStr, nombre, almacen, items[i].variante, cantStr };
            imprimirFilaTabla(fila, colsDeadStock, N_COLS_DS, i % 2);

        }

        imprimirPieTabla(colsDeadStock, N_COLS_DS);
        imprimirPaginacion(pagina, totalPags, n);

        imprimirSeccion("COMANDOS");
        printf("  " ESTILO_CMD "EXPORTAR           " RESET "  Guardar en .csv\n");
        printf("  " ESTILO_CMD "ANTERIOR/SIGUIENTE " RESET "  Navegar páginas\n");
        printf("  " ESTILO_CMD "VOLVER             " RESET "  Volver a informes\n\n");

        Entrada e = leerComando(cmdsDeadStock, N_CMDS_DS, ">");

        if (strcmp(e.comando, "EXPORTAR") == 0) {

            time_t ahora = time(NULL);
            struct tm* tm = localtime(&ahora);
            char nombreFich[256];
            snprintf(nombreFich, sizeof(nombreFich), "%sdead_stock_%04d%02d%02d.csv", rutaReports, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

            if (exportarDeadStockCsv(items, n, nombreFich) == 0) {

                char msg[512];
                snprintf(msg, sizeof(msg), "Exportado a: %s", nombreFich);
                imprimirExito(msg);
                LOG_INFO("Dead stock exportado a: %s (%d productos)", nombreFich, n);
                pausar();
                break;

            } else {

                imprimirError("Error al exportar el CSV.");
                LOG_ERROR("Fallo al exportar dead stock: %s", nombreFich);

            }

        } else if (strcmp(e.comando, "SIGUIENTE") == 0) {

            if (pagina < totalPags) pagina++;
            else pagina = 1;

        } else if (strcmp(e.comando, "ANTERIOR") == 0) {

        	if (pagina > 1) pagina--;
            else pagina = totalPags;

        } else if (strcmp(e.comando, "VOLVER") == 0) {

            break;

        } else if (strcmp(e.comando, "HOME") == 0) {

        	liberarDeadStock(items, n);
            return 1;

        } else if (strcmp(e.comando, "EXIT") == 0) {

        	liberarDeadStock(items, n);
            salir = 1;
            return 1;

        } else {

            imprimirError("Comando no reconocido.");
            pausar();

        }

    }

    liberarDeadStock(items, n);
    return 0;

}

// TOP VENTAS

static char* cmdsTopVentas[] = {

    "EXPORTAR", "VOLVER", "HOME", "EXIT"

};

#define N_CMDS_TV 4

// Columnas para top ventas
static Columna colsTopVentas[] = {

    { "  ID  ",          6,  1 },
	{ "              PRODUCTO              ", 36, 0 },
	{ "       CATEGORIA       ", 23, 0 },
    { " VENDIDAS ",  10,  1 },
    { "   INGRESOS   ",  14,  1 },

};

#define N_COLS_TV 5

int pantallaTopVentas(sqlite3* db, char* rutaReports) {

    printf("\n");
    int limit = leerEntero("Numero de productos en el ranking (1-100):", 1, 100);

    int n = 0;
    ProductoVenta* items = getTopVentas(db, limit, &n);

    imprimirCabecera("TOP VENTAS", "Ranking historico de productos mas vendidos");

    if (!items || n == 0) {

        imprimirWarn("No hay datos de ventas registrados.");
        pausar();
        return 0;

    }

    imprimirCabeceraTabla(colsTopVentas, N_COLS_TV);

    for (int i = 0; i < n; i++) {

        char posStr[16], vendStr[12], ingStr[16];
        snprintf(posStr,  sizeof(posStr),  "%d",   i + 1);
        snprintf(vendStr, sizeof(vendStr), "%d",    items[i].totalVendido);
        snprintf(ingStr,  sizeof(ingStr),  "%.2f",  items[i].totalIngresos);

        char nombre[33];
        strncpy(nombre, items[i].nombreProd, 32);
        nombre[32] = '\0';

        char categoria[24];
        strncpy(categoria, items[i].nombreCat, 23);
        categoria[23] = '\0';

        char* fila[] = { posStr, nombre, categoria, vendStr, ingStr };
        imprimirFilaTabla(fila, colsTopVentas, N_COLS_TV, i % 2);

    }

    imprimirPieTabla(colsTopVentas, N_COLS_TV);

    int    totalUds = 0;
    double totalIng = 0;
    for (int i = 0; i < n; i++) {

        totalUds += items[i].totalVendido;
        totalIng += items[i].totalIngresos;

    }
    printf("\n  " C_GRIS "Total unidades vendidas: " RESET C_BLANCO "%d" RESET "   " C_GRIS "Ingresos totales: " RESET ESTILO_PRECIO "%.2f EUR\n" RESET, totalUds, totalIng);

    imprimirSeccion("COMANDOS");
    printf("  " ESTILO_CMD "EXPORTAR  " RESET "  Guardar ranking en .csv\n");
    printf("  " ESTILO_CMD "VOLVER    " RESET "  Volver a informes\n\n");

    while (1) {

        Entrada e = leerComando(cmdsTopVentas, N_CMDS_TV, ">");

        if (strcmp(e.comando, "EXPORTAR") == 0) {

            time_t ahora = time(NULL);
            struct tm* tm = localtime(&ahora);
            char nombreFich[256];
            snprintf(nombreFich, sizeof(nombreFich), "%stop_ventas_%04d%02d%02d.csv", rutaReports, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

            FILE* f = fopen(nombreFich, "w");
            if (f) {

                fprintf(f, "POSICION;ID_PR;NOMBRE;CATEGORIA;" "UNIDADES_VENDIDAS;INGRESOS\n");
                for (int i = 0; i < n; i++) fprintf(f, "%d;%d;\"%s\";\"%s\";%d;%.2f\n", i + 1, items[i].idProd, items[i].nombreProd, items[i].nombreCat, items[i].totalVendido, items[i].totalIngresos);

                fclose(f);
                char msg[512];
                snprintf(msg, sizeof(msg), "Exportado a: %s", nombreFich);
                imprimirExito(msg);
                LOG_INFO("Top ventas exportado a: %s (limit=%d)", nombreFich, limit);
                pausar();
                break;

            } else {

                imprimirError("Error al crear el fichero CSV.");
                LOG_ERROR("Fallo al exportar top ventas: %s", nombreFich);

            }

        } else if (strcmp(e.comando, "VOLVER") == 0) {

            break;

        } else if (strcmp(e.comando, "HOME") == 0) {

            liberarTopVentas(items, n);
            return 1;

        } else if (strcmp(e.comando, "EXIT") == 0) {

            liberarTopVentas(items, n);
            salir = 1;
            return 1;

        } else {

            imprimirError("Comando no reconocido.");
            pausar();

        }
    }

    liberarTopVentas(items, n);
    return 0;

}
