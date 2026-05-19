#ifndef INCLUDE_CATALOGO_UI_H_
#define INCLUDE_CATALOGO_UI_H_

#include "client.h"

// SELECCIONAR CATEGORIA

int seleccionarCategoria(Client& cliente);

// SELECCIONAR VARIANTE

std::string seleccionarVariante(Client& cliente, int idProd);

// CATALOGO

void pantallaCatalogo(Client& cliente);

// BUSCAR

void pantallaBuscar(Client& cliente, bool soloFavoritos = false);
void pantallaFavoritos(Client& cliente);	// Llama a pantallaBuscar con soloFavoritos = true

// VER PRODUCTO

int pantallaVerProducto(Client& cliente, int idProd);

// RESEÑA

int pantallaResenas(Client& cliente, int idProd);

#endif /* INCLUDE_CATALOGO_UI_H_ */
