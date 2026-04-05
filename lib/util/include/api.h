#ifndef UTIL_INCLUDE_API_H_
#define UTIL_INCLUDE_API_H_

#include <stdlib.h>
#include "estructuras.h"

typedef struct {
    char* data;
    size_t size;
} Respuesta;

// Llamada genérica a las APIs que vamos a utilizar.
// Devuelve la respuesta en el formato de la estructura utilizando cJSON.
Respuesta* llamarApi(const char* url);
void liberarRespuesta(Respuesta* resp);

// Devuelve una ubicación aleatoria de una ciudad.
// Utilizada solamente para rellenar la BD de forma más realista.
Ubicacion* calleAleatoria(Ciudad ciudad);

// Actualiza la ubicación con las coordenadas precisas obtenidas llamando a la API
int completarUbicacion(Ubicacion* ubi, char* pais, char* ciudad, char* direccion);

#endif /* UTIL_INCLUDE_API_H_ */
