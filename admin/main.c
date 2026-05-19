#include <admin_ui.h>
#include "sqlite3.h"
#include "log.h"
#include "config.h"
#include <stdio.h>
#include <windows.h>

int main() {

	char dbPath[256];
	char logPath[256];

	configGet(CONFIG_PATH, "DB_PATH", dbPath, sizeof(dbPath));
	configGet(CONFIG_PATH, "LOG_ADMIN_PATH", logPath, sizeof(logPath));

    // Abrimos el log
    logInit(logPath, LOG_DEBUG);

    // Abrimos la BD
    sqlite3* db;
    int result = sqlite3_open(dbPath, &db);

    if (result != SQLITE_OK) {
        LOG_FATAL("Error abriendo la BD: %s", sqlite3_errmsg(db));
        fprintf(stderr, "Error abriendo la base de datos: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        logClose();
        return 1;
    }

    // Activamos claves foráneas
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);

    // Lanzamos el bucle principal
    bucleAdmin(db);

    // Cerramos todoo correctamente
    sqlite3_close(db);
    logClose();

    return 0;

}
