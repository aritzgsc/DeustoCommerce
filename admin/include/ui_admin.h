#ifndef INCLUDE_UI_ADMIN_H_
#define INCLUDE_UI_ADMIN_H_

#include "sqlite3.h"

// Flag para EXIT
extern int salir;

// Server config path

extern char* CONFIG_PATH;

// Bucle principal del administrador.
// Abre conexión con BD, muestra el HOME y gestiona la navegación.
void bucleAdmin(sqlite3* db);

// Pantalla HOME: panel de administración principal.
void pantallaHome(sqlite3* db);

#endif /* INCLUDE_UI_ADMIN_H_ */
