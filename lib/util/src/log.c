#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

// ESTADO INTERNO

static FILE* g_logFile = NULL;
static LogNivel g_nivelMin = LOG_INFO;	// Default

// Etiquetas para cada nivel
static const char* ETIQUETAS[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

// INIT / CLOSE

int logInit(const char* rutaFichero, LogNivel nivelMinimo) {

    if (g_logFile) fclose(g_logFile);

    g_logFile  = fopen(rutaFichero, "a");
    g_nivelMin = nivelMinimo;

    if (!g_logFile) {

        fprintf(stderr, "[LOG] No se pudo abrir el fichero de log: %s\n", rutaFichero);
        return -1;

    }

    // Línea separadora al iniciar

    time_t ahora = time(NULL);
    struct tm* tm = localtime(&ahora);
    char fechaStr[32];
    strftime(fechaStr, sizeof(fechaStr), "%Y-%m-%d %H:%M:%S", tm);

    fprintf(g_logFile,
            "\n──────────────────────────────────────────────────\n"
            "[%s] [INFO] Sesion iniciada\n"
            "──────────────────────────────────────────────────\n",
            fechaStr);

    fflush(g_logFile);

    return 0;

}

void logClose() {

    if (!g_logFile) return;

    time_t ahora = time(NULL);
    struct tm* tm = localtime(&ahora);
    char fechaStr[32];
    strftime(fechaStr, sizeof(fechaStr), "%Y-%m-%d %H:%M:%S", tm);

    fprintf(g_logFile,
    		"──────────────────────────────────────────────────\n"
            "[%s] [INFO] Sesion cerrada\n"
            "──────────────────────────────────────────────────\n\n",
            fechaStr);

    fclose(g_logFile);
    g_logFile = NULL;

}

// ESCRIBIR

void logEscribir(LogNivel nivel, const char* fichero, int linea, const char* fmt, ...) {

    if (!g_logFile || nivel < g_nivelMin) return;

    // Timestamp
    time_t ahora = time(NULL);
    struct tm* tm = localtime(&ahora);
    char fechaStr[32];
    strftime(fechaStr, sizeof(fechaStr), "%Y-%m-%d %H:%M:%S", tm);

    // Extraemos solo el nombre del fichero (sin la ruta completa)
    const char* nombreFich = strrchr(fichero, '\\');
    if (!nombreFich) nombreFich = strrchr(fichero, '/');
    nombreFich = nombreFich ? nombreFich + 1 : fichero;

    // Escribimos en el fichero
    if (nivel == LOG_DEBUG) fprintf(g_logFile, "[%s] [%s] (%s:%d) ", fechaStr, ETIQUETAS[nivel], nombreFich, linea);
    else fprintf(g_logFile, "[%s] [%s] ", fechaStr, ETIQUETAS[nivel]);

    va_list args;
    va_start(args, fmt);
    vfprintf(g_logFile, fmt, args);
    va_end(args);

    fprintf(g_logFile, "\n");

    fflush(g_logFile);

}
