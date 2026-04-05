#ifndef LOG_INCLUDE_LOG_H_
#define LOG_INCLUDE_LOG_H_

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <time.h>

// NIVELES DE LOG

typedef enum {
    LOG_DEBUG = 0,  // Información detallada para depuración
    LOG_INFO  = 1,  // Operaciones normales
    LOG_WARN  = 2,  // Situaciones inesperadas pero no críticas
    LOG_ERROR = 3,  // Errores recuperables
    LOG_FATAL = 4,  // Errores irrecuperables
} LogNivel;

// CONFIGURACIÓN

// Inicializa el sistema de log.
// rutaFichero: ruta al fichero .log (p.ej. "../../data/logs/admin.log")
// nivelMinimo: solo se registran mensajes de este nivel en adelante
// Devuelve 0 si OK, -1 si falla.
int  logInit(const char* rutaFichero, LogNivel nivelMinimo);

// Cierra el fichero de log correctamente.
void logClose();

// FUNCIONES DE LOG

// Función base (usada internamente por las macros)
void logEscribir(LogNivel nivel, const char* fichero, int linea, const char* fmt, ...);

// CONVENIENCIA

#define LOG_DEBUG(...) logEscribir(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) logEscribir(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) logEscribir(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) logEscribir(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) logEscribir(LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

// Macro para loguear errores de SQLite directamente
#define LOG_SQL_ERROR(db, contexto) \
    logEscribir(LOG_ERROR, __FILE__, __LINE__, \
                "[SQL] %s: %s", contexto, sqlite3_errmsg(db))

#endif

#endif /* LOG_INCLUDE_LOG_H_ */
