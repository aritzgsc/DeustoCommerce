#include <admin_ui.h>
#include <almacenes_ui.h>
#include <catalogo_db.h>
#include <catalogo_ui.h>
#include <informes_ui.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <utils_ui.h>

int salir = 0;

// COMANDOS DEL HOME

static char* cmdsHome[] = {

    "CATALOGO", "BUSCAR", "ALMACENES", "INFORME", "EXIT"

};

#define N_CMDS_HOME 5

// AUXILIARES

#define FILA_ESTADO(etiqueta, ...) \
		printf("  " C_GRIS "%-24s" RESET, etiqueta); \
		printf(__VA_ARGS__)

// Imprime el estado general del sistema en el HOME
static void imprimirEstadoSistema(sqlite3* db) {

    imprimirSeccion("ESTADO DEL SISTEMA");

    // Timestamp actual
    time_t ahora = time(NULL);
    struct tm* tm = localtime(&ahora);
    char fechaStr[32];
    strftime(fechaStr, sizeof(fechaStr), "%d/%m/%Y  %H:%M", tm);

    // Estado del sistema: Información general

    int catalogo = 0;
    int redLogis = 0;
    int pedidosPend = 0;
    int productosSinStock = 0;
    double ocupacionRed = 0.0;

	getEstadoSistema(db, &catalogo, &redLogis, &pedidosPend, &productosSinStock, &ocupacionRed);

	// Fecha y hora

	FILA_ESTADO("Fecha y hora:", ESTILO_HINT " %s\n" RESET, fechaStr);

	// Catálogo

	FILA_ESTADO("Catálogo:", C_BLANCO "  %d productos\n" RESET, catalogo);

    // Red logística

	FILA_ESTADO("Red logística:", C_BLANCO "  %d almacenes activos\n" RESET, redLogis);

    // Pedidos pendientes

	FILA_ESTADO("Pedidos pendientes:", pedidosPend > 0 ? ESTILO_WARN " %d pedidos en proceso\n" RESET : ESTILO_EXITO " Sin pedidos pendientes\n" RESET, pedidosPend);

	// Productos sin stock

	FILA_ESTADO("Productos sin stock:", productosSinStock > 0 ? ESTILO_ERROR " %d productos sin stock\n" RESET : ESTILO_EXITO " Todos los productos con stock\n" RESET, productosSinStock);

    // Ocupación de la red

    int llenas  = (int)(ocupacionRed / 10);
    int vacias  = 10 - llenas;

    char colorSelec[16] = "";
    char ocupacionStr[256] = "";

    if (ocupacionRed >= 90) strcpy(colorSelec, C_ROJO);
    else if (ocupacionRed >= 70) strcpy(colorSelec, C_AMARILLO);
    else strcpy(colorSelec, C_VERDE);

    strcat(ocupacionStr, "  ");
    strcat(ocupacionStr, colorSelec);

    for (int i = 0; i < llenas; i++) strcat(ocupacionStr, "█");
    strcat(ocupacionStr, C_GRIS);
    for (int i = 0; i < vacias; i++) strcat(ocupacionStr, "░");
    strcat(ocupacionStr, RESET C_BLANCO " %.1f%%\n" RESET);

    FILA_ESTADO("Ocupación red:", ocupacionStr, ocupacionRed);

}

// HOME

void pantallaHome(sqlite3* db) {

    limpiarPantalla();

    // Logo completo en el HOME

    printf("\n");
    printf(ESTILO_TITULO
           "  ╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n"
           "  ║                                                                                                              ║\n"
           "  ║" RESET C_BLANCO NEGRITA
              "                                  DEUSTO" RESET C_CYAN NEGRITA
			                                          "COMMERCE" RESET C_BLANCO NEGRITA
                                                              "  —  Panel de Administración                                  "RESET ESTILO_TITULO
								                                                                                            "║\n"
           "  ║                                                                                                              ║\n"
           "  ╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n"
           RESET);

    imprimirEstadoSistema(db);

    imprimirSeccion("COMANDOS DISPONIBLES");

    printf("  " ESTILO_CMD "CATALOGO  " RESET "  Gestión de productos y catálogo\n");
    printf("  " ESTILO_CMD "BUSCAR    " RESET "  Búsqueda filtrada de productos\n");
    printf("  " ESTILO_CMD "ALMACENES " RESET "  Gestión logística y distribución\n");
    printf("  " ESTILO_CMD "INFORME   " RESET "  Reportes financieros y análisis\n");
    printf("  " ESTILO_CMD "EXIT      " RESET "  Cerrar consola de administración\n\n");

}

// BUCLE PRINCIPAL

void bucleAdmin(sqlite3* db) {

    activarColores();

    while (!salir) {

        pantallaHome(db);

        Entrada e = leerComando(cmdsHome, N_CMDS_HOME, ">");

        if (strcmp(e.comando, "CATALOGO") == 0) {

            pantallaCatalogo(db);

        } else if (strcmp(e.comando, "BUSCAR") == 0) {

            pantallaBuscar(db);

        } else if (strcmp(e.comando, "ALMACENES") == 0) {

            pantallaAlmacenes(db);

        } else if (strcmp(e.comando, "INFORME") == 0) {

            pantallaInformes(db);

        } else if (strcmp(e.comando, "EXIT") == 0) {

            salir = 1;

        } else {

            imprimirError("Comando no reconocido.");
            pausar();

        }

    }

    limpiarPantalla();
    printf("\n  " ESTILO_SUBTITULO "Cerrando DeustoCommerce Admin...\n\n" RESET);

}
