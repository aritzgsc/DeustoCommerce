#ifndef INCLUDE_CLIENT_UI_H_
#define INCLUDE_CLIENT_UI_H_

#include "client.h"

// Flag para EXIT

extern bool salir;

// SELECCIONAR DIRECCION

int seleccionarDireccion(Client& cliente);

// HOME

void pantallaHome(Client& cliente);

// CARRITO

void pantallaCarrito(Client& cliente);
bool pantallaPagarCarrito(Client& cliente);

// PEDIDOS

void pantallaPedidos(Client& cliente);

// BUCLE PRINCIPAL

void bucleCliente(Client& cliente);

#endif /* INCLUDE_CLIENT_UI_H_ */
