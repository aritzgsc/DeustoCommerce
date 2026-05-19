#ifndef INCLUDE_HANDLERS_CLIENT_H_H_
#define INCLUDE_HANDLERS_CLIENT_H_H_

#include <string>
#include <openssl/ssl.h>
#include "sesion.h"

extern "C" {
    #include "sqlite3.h"
}

void handleGetEstadoCliente (SSL* ssl, sqlite3* db, Sesion& sesion);
void handleGetDirecciones (SSL* ssl, sqlite3* db, Sesion& sesion);
void handleGetCarrito (SSL* ssl, sqlite3* db, Sesion& sesion);
void handleVaciarCarrito (SSL* ssl, sqlite3* db, Sesion& sesion);
void handlePagarCarrito (SSL* ssl, sqlite3* db, std::string args[], Sesion& sesion);
void handleGetPedidos (SSL* ssl, sqlite3* db, std::string args[], Sesion& sesion);

#endif /* INCLUDE_HANDLERS_CLIENT_H_H_ */
