#ifndef INCLUDE_HANDLERS_AUTH_H_H_
#define INCLUDE_HANDLERS_AUTH_H_H_

#include "sesion.h"
#include "openssl/ssl.h"
#include "sqlite3.h"
#include <string>

// AUXILIARES

std::string generarToken();

// AUTH HANDLERS

void handleAutoLogin(SSL* ssl, sqlite3* db, std::string args[], Sesion& sesion);
void handleLogin(SSL* ssl, sqlite3* db, std::string args[], Sesion& sesion);
void handleRegistro(SSL* ssl, sqlite3* db, std::string args[], Sesion& sesion);
void handleLogout(SSL* ssl, sqlite3* db, Sesion& sesion);

#endif /* INCLUDE_HANDLERS_AUTH_H_H_ */
