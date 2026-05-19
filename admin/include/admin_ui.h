#ifndef INCLUDE_ADMIN_UI_H_
#define INCLUDE_ADMIN_UI_H_

#include "sqlite3.h"

#define CONFIG_PATH "../data/config/server_config.ini"

// Flag para EXIT
extern int salir;

// Bucle principal del administrador.
// Abre conexión con BD, muestra el HOME y gestiona la navegación.
void bucleAdmin(sqlite3* db);

// Pantalla HOME: panel de administración principal.
void pantallaHome(sqlite3* db);

#endif /* INCLUDE_ADMIN_UI_H_ */
