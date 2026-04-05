#include "include/seeder.h"
#include <stdio.h>

static char* DB_PATH = "../data/db/DeustoCommerce.db";

// Main para rellenar la BD, descomentar las lineas de abajo solo si hay que rellenar la BD y sabes lo que estás haciendo

int main() {

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	sqlite3* db;
	int result = sqlite3_open(DB_PATH, &db);

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
