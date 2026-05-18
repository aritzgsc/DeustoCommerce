#ifndef INCLUDE_PROTOCOLO_H_
#define INCLUDE_PROTOCOLO_H_

#include "sesion.h"
#include "openssl/ssl.h"
#include "sqlite3.h"
#include <string>

// Definimos un máximo de argumentos (comando + parámetros)

#define MAX_ARGS 10

// Utilidad para separar los argumentos en un array clásico.
// Devuelve el número de tokens encontrados.

int split(const std::string& s, char delimiter, std::string args[], int maxArgs);

// Enruta la petición al handler correspondiente

void procesarPeticion(SSL* ssl, sqlite3* db, std::string peticionCruda, Sesion& sesion);

// Responder al cliente con el formato correcto

void responder(SSL* ssl, std::string mensaje);

#endif /* INCLUDE_PROTOCOLO_H_ */
