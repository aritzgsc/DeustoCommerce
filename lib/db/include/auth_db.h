#ifndef DB_INCLUDE_AUTH_DB_H_
#define DB_INCLUDE_AUTH_DB_H_

#include "estructuras.h"
#include "sqlite3.h"

// CONSULTAS

int getUsuarioPorToken(sqlite3* db, const char* token, Usuario* u);
int getUsuarioPorCorreo(sqlite3* db, const char* correo, Usuario* u);
int existeUsuario(sqlite3* db, const char* correo);

// ACTUALIZACIONES

int crearUsuario(sqlite3* db, const char* correo, const char* nom, const char* ap, const char* hash);
int actualizarToken(sqlite3* db, const char* correo, const char* nuevoToken);
int invalidarToken(sqlite3* db, const char* correo);

#endif /* DB_INCLUDE_AUTH_DB_H_ */
