#ifndef INCLUDE_ESTRUCTURAS_H_
#define INCLUDE_ESTRUCTURAS_H_

#include <time.h>

// USUARIOS

typedef struct {

	char* correo;
	char* nombre;
	char* apellido;
	char* contrasena;

} Usuario;

// UBICACIONES

typedef struct {

	char* id;
	char* nombre;

} Pais;

typedef struct {

	int id;
	char* nombre;
	double latitud;
	double longitud;
	int poblacion;
	Pais pais;

} Ciudad;

typedef struct {

	int id;
	char* direccion;
	double latitud;
	double longitud;
	Ciudad ciudad;
	char* correo;
	int activo;

} Ubicacion;

// PRODUCTOS

typedef struct {

	int id;
	char* nombre;
	char** variantes;
	int nVariantes;

} Categoria;

typedef struct {

	int id;
	char* nombre;
	char* descripcion;
	char* variante;
	double precio;
	double descuento;
	Categoria categoria;

} Producto;

typedef enum {

	NO_PEDIDO, EN_PROCESO, ENVIADO, ENTREGADO, CANCELADO

} Estado;

typedef struct {

	Producto producto;
	int cantidad;

} ProdCant;

typedef struct {

	ProdCant *productos;
	int nProductosDiferentes;
	Estado estado;
	double precioCompra;
	Ubicacion destino;
	time_t fechaEnvio;
	time_t fechaRecibo;
	Usuario usuario;

} Pedido;

// ALMACENES

typedef struct {

	Producto producto;
	int cantidad;
	int disponible;

} StockProd;

typedef struct Almacen {

	int id;
	char* nombre;
	int capacidad;
	Ubicacion ubicacion;
	StockProd* productos;
	int nProductosDiferentes;

} Almacen;

#endif /* INCLUDE_ESTRUCTURAS_H_ */
