#include "auth_h.h"
#include "catalogo_h.h"
#include "client_h.h"
#include "protocolo.h"
#include <iostream>
#include <sstream>

using namespace std;

int split(const string& s, char delimiter, string args[], int maxArgs) {

	int count = 0;
    string token;
    istringstream tokenStream(s);

    while (getline(tokenStream, token, delimiter) && count < maxArgs) {
        args[count] = token;
        count++;
    }

    return count;

}

void procesarPeticion(SSL* ssl, sqlite3* db, string peticionCruda, Sesion& sesion) {

    string args[MAX_ARGS];
    int count = split(peticionCruda, '|', args, MAX_ARGS);
    if (count == 0) return;

    string comando = args[0];

    // AUTH

    if (comando == "AUTOLOGIN") handleAutoLogin(ssl, db, args, sesion);
    else if (comando == "LOGIN") handleLogin(ssl, db, args, sesion);
    else if (comando == "REGISTRO") handleRegistro(ssl, db, args, sesion);
    else if (comando == "LOGOUT") handleLogout(ssl, db, sesion);

    // CATALOGO

    else if (comando == "GET_CATEGORIAS") handleGetCategorias(ssl, db, args);
    else if (comando == "GET_VARIANTES") handleGetVariantes(ssl, db, args);
    else if (comando == "GET_CATALOGO") handleGetCatalogo(ssl, db, args);
    else if (comando == "BUSCAR") handleBuscar(ssl, db, args, sesion);
    else if (comando == "GET_PROD_DETALLE") handleGetProdDetalle(ssl, db, args);
    else if (comando == "ADD_CARRITO") handleAddCarrito(ssl, db, args, sesion);
    else if (comando == "TOGGLE_FAVORITO") handleToggleFavorito(ssl, db, args, sesion);
    else if (comando == "GET_RESENAS") handleGetResenas(ssl, db, args, sesion);
    else if (comando == "ADD_RESENA") handleAddResena(ssl, db, args, sesion);
    else if (comando == "ELIMINAR_RESENA") handleEliminarResena(ssl, db, args, sesion);

    // CLIENTE

    else if (comando == "GET_ESTADO_CLIENTE") handleGetEstadoCliente(ssl, db, sesion);
    else if (comando == "GET_DIRECCIONES") handleGetDirecciones(ssl, db, sesion);
    else if (comando == "GET_CARRITO") handleGetCarrito(ssl, db, sesion);
    else if (comando == "VACIAR_CARRITO") handleVaciarCarrito(ssl, db, sesion);
    else if (comando == "PAGAR_CARRITO") handlePagarCarrito(ssl, db, args, sesion);
    else if (comando == "GET_PEDIDOS") handleGetPedidos(ssl, db, args, sesion);

    else {
        // Si el comando no existe, enviamos un ERR de forma segura
        string error = "ERR|Comando no reconocido.";
        responder(ssl, error);
    }

}

void responder(SSL* ssl, string mensaje) {
    SSL_write(ssl, mensaje.c_str(), (int)mensaje.length());
}
