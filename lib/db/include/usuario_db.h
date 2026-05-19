#ifndef DB_INCLUDE_USUARIO_DB_H_
#define DB_INCLUDE_USUARIO_DB_H_

#include "sqlite3.h"
#include "catalogo_db.h"

// CARRITO

// Vista plana de un ítem del carrito para el handler
typedef struct {

    int id;               // ID del producto
    char* nombreProducto;
    char* variante;
    int cantidad;
    double precioUnitario;   // precio base, sin descuento
    double descuento;        // 0.0 – 0.9

} ItemCarrito;

// Devuelve el array de ítems del carrito. Liberar con liberarCarrito().
// n se pone a 0 si el carrito está vacío (no error).
ItemCarrito* getCarrito(sqlite3* db, const char* correo, int* n);
void liberarCarrito(ItemCarrito* items, int n);

// Devuelve la suma de cantidades de todos los ítems del carrito, o -1 en error.
int getItemsCarrito(sqlite3* db, const char* correo);

// Añade 'cantidad' unidades de idProd+variante al carrito.
// Si el par (idProd, variante) ya existe, incrementa la cantidad (upsert).
// Devuelve 0 en éxito, -1 en error.
int addAlCarrito(sqlite3* db, const char* correo, int idProd, const char* variante, int cantidad);

// Elimina todos los ítems del carrito del usuario. Devuelve 0 en éxito, -1 en error.
int vaciarCarrito(sqlite3* db, const char* correo);

// PEDIDOS

// Devuelve una página del historial de pedidos del usuario, del más reciente al más antiguo.
// Liberar con liberarPedidos().
Pedido* getPedidos (sqlite3* db, const char* correo, int pagina, int* total);
Pedido* getPedidoPorId(sqlite3* db, int idPedido);	// Devuelve un pedido concreto
void liberarPedidos (Pedido* pedidos, int n);

// Devuelve el número de pedidos EN_PROCESO o ENVIADO (en curso), o -1 en error.
int getPedidosCurso(sqlite3* db, const char* correo);


// Crea un pedido a partir del carrito actual del usuario con la dirección indicada.
// Operación atómica (transacción): copia el carrito a PEDIDO_PROD y lo vacía.
// Devuelve el id del pedido creado.
int crearPedido(sqlite3* db, const char* correo, const char* nombre, const char* apellido, int idUbicacion, time_t timestampEjecucion);

// Actualiza el estado de un pedido al nuevo estado.
int actualizarEstadoPedido(sqlite3* db, int idPed, int nuevoIdEstado);

// FAVORITOS

// Alterna el estado de favorito de un producto para el usuario.
// Devuelve: 1 si se añadió, 0 si se eliminó, -1 en error de BD.
int toggleFavorito(sqlite3* db, const char* correo, int idProd);

// Devuelve el número de productos favoritos del usuario, o -1 en error.
int getItemsFavoritos(sqlite3* db, const char* correo);

// Como buscarProductos() pero limitado a los favoritos del usuario.
// Aplica los mismos filtros de FiltrosProducto sobre el subconjunto de favoritos.
// El llamador libera el array con liberarProducto() + free() igual que buscarProductos().
Producto* buscarProductosFavoritos(sqlite3* db, const char* correo, FiltrosProducto f, int pagina, int* total);

// RESEÑAS

// Devuelve la valoración media ponderada de un producto.
// Usa el campo PESO de la reseña para ponderar.
// Devuelve -1 si no hay reseñas.
double getValoracionMedia(sqlite3* db, int idProd);

// Devuelve el número total de reseñas de un producto.
int getNumResenas(sqlite3* db, int idProd);

// Devuelve una página de reseñas del producto. El llamador libera con liberarResenas().
Resena* getResenas(sqlite3* db, int idProd, int pagina, int* total);
void liberarResenas(Resena* resenas, int n);

// Comprueba si un usuario ha hecho una reseña sobre un producto.
int comprobarResena(sqlite3* db, int idProd, const char* correo);

// Inserta una nueva reseña. Devuelve 0 en éxito, -1 en error. La cambia si ya existe.
int addResena(sqlite3* db, int idProd, const char* correo, double valoracion, const char* comentario);

// Elimina una reseña. Devuelve 0 en éxito, -1 en error.
int eliminarResena(sqlite3* db, int idProd, const char* correo);

// UBICACIONES

// Devuelve un puntero a la ubicación del ID especificado.
Ubicacion* getUbicacionPorId(sqlite3* db, int idUbicacion);
void liberarUbicacion(Ubicacion* u);

// Devuelve las direcciones activas del usuario. n=0 si no tiene (no error).
// El llamador libera con liberarUbicaciones().
Ubicacion* getUbicaciones(sqlite3* db, const char* correo, int* n);
void liberarUbicaciones(Ubicacion* dirs, int n);

// Crea una nueva dirección para el usuario en la ciudad indicada.
// Devuelve el ID de la nueva ubicación (> 0), o -1 en error.
int crearUbicacion(sqlite3* db, const char* correo, const char* pais, const char* ciudad, const char* direccion, double latitud, double longitud);

#endif /* DB_INCLUDE_USUARIO_DB_H_ */
