#include "include/seeder.h"
#include "config.h"
#include <stdio.h>

#define CONFIG_PATH "../data/config/server_config.ini"

// Main para rellenar la BD, descomentar las lineas de abajo solo si hay que rellenar la BD y sabes lo que estás haciendo

int main() {

	char dbPath[256];

	configGet(CONFIG_PATH, "DB_PATH", dbPath, sizeof(dbPath));

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	sqlite3* db;
	int result = sqlite3_open(dbPath, &db);

	if (result != SQLITE_OK) {
		fprintf(stderr, "Error al abrir la BD: %s", sqlite3_errmsg(db));
		return -1;
	}

	result = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);

//	seedPaises(db);
//	seedCiudades(db);

//	seedAlmacenes(db);

//	seedCategorias(db);
//	seedProductos(db);

//	seedStock(db);

	sqlite3_close(db);

	return 0;

}
