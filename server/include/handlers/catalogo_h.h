#ifndef INCLUDE_HANDLERS_CATALOGO_H_H_
#define INCLUDE_HANDLERS_CATALOGO_H_H_

#include <string>
#include <openssl/ssl.h>
#include "sesion.h"

extern "C" {
    #include "sqlite3.h"
}

void handleGetCategorias (SSL* ssl, sqlite3* db, std::string args[]);
void handleGetVariantes (SSL* ssl, sqlite3* db, std::string args[]);
void handleGetCatalogo (SSL* ssl, sqlite3* db, std::string args[]);
void handleBuscar (SSL* ssl, sqlite3* db, std::string args[], Sesion& sesion);
void handleGetProdDetalle (SSL* ssl, sqlite3* db, std::string args[]);
void handleAddCarrito (SSL* ssl, sqlite3* db, std::string args[], Sesion& sesion);
void handleToggleFavorito (SSL* ssl, sqlite3* db, std::string args[], Sesion& sesion);
void handleGetResenas (SSL* ssl, sqlite3* db, std::string args[], Sesion& sesion);
void handleAddResena (SSL* ssl, sqlite3* db, std::string args[], Sesion& sesion);
void handleEliminarResena (SSL* ssl, sqlite3* db, std::string args[], Sesion& sesion);

#endif /* INCLUDE_HANDLERS_CATALOGO_H_H_ */
