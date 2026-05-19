#ifndef UTIL_INCLUDE_LOGISTICA_H_
#define UTIL_INCLUDE_LOGISTICA_H_

#include "estructuras.h"
#include <windows.h>

#define RADIO_TIERRA_KM          6371.0
#define DEG_A_RAD(x)             ((x) * M_PI / 180.0)

#define VEL_MEDIA_KMH            75.0
#define CAPACIDAD_VEHICULO       25000.0
#define COSTE_KM_VEHICULO        1.4
#define COSTE_MIN_TRASVASE       150.0
#define HORAS_MIN_TRASVASE       2.0

#define COSTE_MANIPULACION_UND   0.05
#define MARGEN_GANANCIAS         0.5
#define HORAS_PREPARACION_EXT    8.0
#define DIAS_TRANSITO_EXT        2

#define COSTE_BASE_PAQUETERIA 	3.50
#define COSTE_EXTRA_ARTICULO 	0.50
#define COSTE_KM_PAQUETERIA 	0.015
#define HORAS_PROCESADO_PEDIDO 	18.0
#define VEL_MEDIA_RED_LOGISTICA 60.0

// DISTANCIA

// Calcula la distancia en km entre dos ubicaciones usando Haversine.
double calcularDistancia(Ubicacion u1, Ubicacion u2);

// LOGISTICA

// Calcula el coste en tiempo y dinero de un transporte de mercancia
void calcularCosteTrasvase(Ubicacion ubi1, Ubicacion ubi2, int cant, double* precio, time_t* duracion);

// Calcula el coste en tiempo y dinero de un envío
void calcularCostePaqueteria(Ubicacion origen, Ubicacion destino, int cant, double* precio, time_t* duracion);

// Calcula el coste en tiempo y dinero de una compra de mercancía a externos
void calcularCosteCompraExterno(ProdCant* prods, int nProds, double* precio, time_t* duracion);

// Calcula el coste en tiempo y dinero de un restock automático de un almacén
void calcularCosteRestock(sqlite3* db, int idAlm, double* precio, time_t* duracion);

// Calcula el coste estimado de cerrar un almacén (reubicación de todoo su stock)
void calcularCosteCierreAlmacen(sqlite3* db, int idAlm, double* precio, time_t* duracion);

// Calcula el coste estimado que tendrá un envío.
void calcularCosteEnvio(sqlite3* db, const char* correo, int idUbicacion, double* precio, time_t* duracion);

// GESTION DE FICHERO MOVIMIENTOS

void registrarAccion(time_t timestamp, const char* tipo, const char* datosRestantes);

#endif /* UTIL_INCLUDE_LOGISTICA_H_ */
