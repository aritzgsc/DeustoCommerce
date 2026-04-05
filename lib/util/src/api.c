#include "api.h"
#include "estructuras.h"
#include "curl/curl.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

// Funciones de llamada a API genéricas (Hechas con Claude)

static size_t escribirRespuesta(void* ptr, size_t size, size_t nmemb, Respuesta* resp) {

    size_t bytes = size * nmemb;
    resp->data = realloc(resp->data, resp->size + bytes + 1);
    if (!resp->data) return 0;
    memcpy(resp->data + resp->size, ptr, bytes);
    resp->size += bytes;
    resp->data[resp->size] = '\0';
    return bytes;

}

Respuesta* llamarApi(const char* url) {

	CURL* curl = curl_easy_init();
    if (!curl) return NULL;

    Respuesta* resp = malloc(sizeof(Respuesta));
    resp->data = malloc(1);
    resp->size = 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, escribirRespuesta);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "DeustoCommerce/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "Error API: %s\n", curl_easy_strerror(res));
        liberarRespuesta(resp);
        return NULL;
    }

    return resp;

}

void liberarRespuesta(Respuesta* resp) {
    if (!resp) return;
    free(resp->data);
    free(resp);
}

// Funciones específicas de la aplicación

// Función que llama a OverpassAPI (API de geocodificación) y que a partir de una ciudad, devuelve una lista de calles. Nos quedaremos con una calle con un número aleatorio (buscaremos su ubicación exacta y más información sobre ella con la NominatimAPI)

Ubicacion* calleAleatoria(Ciudad ciudad) {

	// Intentamos primero con 3km, si falla con 10km
	int radios[] = {3000, 10000};
	cJSON* elements = NULL;
	cJSON* root = NULL;

	for (int r = 0 ; r < 2 ; r++) {

		// Construimos la query de Overpass

		char query[512];
		snprintf(query, sizeof(query), "[out:json];way[\"highway\"][\"name\"](around:%d,%.6f,%.6f);out body 20;", radios[r], ciudad.latitud, ciudad.longitud);

		// Codificamos la query para la URL

		CURL* curl_tmp = curl_easy_init();
		char* query_encoded = curl_easy_escape(curl_tmp, query, 0);

		char url[1024];
		snprintf(url, sizeof(url), "https://overpass-api.de/api/interpreter?data=%s", query_encoded);

		curl_free(query_encoded);
		curl_easy_cleanup(curl_tmp);

		// Llamamos a Overpass

		Respuesta* resp = llamarApi(url);
		if (!resp) continue;

		if (root) cJSON_Delete(root);
		root = cJSON_Parse(resp->data);
		liberarRespuesta(resp);
		if (!root) continue;

		elements = cJSON_GetObjectItem(root, "elements");
		if (elements && cJSON_GetArraySize(elements) > 0) {
			printf("Calles encontradas con radio %dm en %s\n", radios[r], ciudad.nombre);
			break;
		}

		printf("Sin calles con radio %dm en %s, reintentando...\n", radios[r], ciudad.nombre);

		Sleep(1000);

	}

	if (!elements || cJSON_GetArraySize(elements) == 0) {
		printf("Sin cobertura OSM para %s, saltando\n", ciudad.nombre);
		if (root) cJSON_Delete(root);
		return NULL;
	}

	// Intentamos hasta 5 calles distintas hasta que Nominatim devuelva resultado

	int n = cJSON_GetArraySize(elements);
	int maxIntentos = 5;

	for (int intento = 0 ; intento < maxIntentos ; intento++) {

		// Elegimos calle aleatoria

		cJSON* calle = cJSON_GetArrayItem(elements, rand() % n);
		cJSON* tags = cJSON_GetObjectItem(calle, "tags");
		cJSON* nombre = cJSON_GetObjectItem(tags, "name");
		if (!nombre) continue;

		char nombreCalle[256];
		strncpy(nombreCalle, nombre->valuestring, sizeof(nombreCalle));

		// Codificamos y construimos la URL de Nominatim

		CURL* curl_tmp2 = curl_easy_init();
		char* calle_encoded = curl_easy_escape(curl_tmp2, nombreCalle, 0);
		char* ciudad_encoded = curl_easy_escape(curl_tmp2, ciudad.nombre, 0);

		char urlNominatim[1024];
		snprintf(urlNominatim, sizeof(urlNominatim), "https://nominatim.openstreetmap.org/search?street=%s&city=%s&format=json&addressdetails=1&limit=1", calle_encoded, ciudad_encoded);

		curl_free(calle_encoded);
		curl_free(ciudad_encoded);
		curl_easy_cleanup(curl_tmp2);

		// Llamamos a Nominatim

		Respuesta* respNominatim = llamarApi(urlNominatim);
		if (!respNominatim) continue;

		cJSON* rootNominatim = cJSON_Parse(respNominatim->data);
		liberarRespuesta(respNominatim);
		if (!rootNominatim) continue;

		// Nominatim devuelve un array, cogemos el primer resultado

		cJSON* resultado = cJSON_GetArrayItem(rootNominatim, 0);
		if (!resultado) {
			printf("Nominatim vacío para %s en %s, reintentando...\n", nombreCalle, ciudad.nombre);
			cJSON_Delete(rootNominatim);
			continue;
		}

		// Extraemos coordenadas y dirección

		cJSON* lat = cJSON_GetObjectItem(resultado, "lat");
		cJSON* lon = cJSON_GetObjectItem(resultado, "lon");
		cJSON* address = cJSON_GetObjectItem(resultado, "address");
		cJSON* road = cJSON_GetObjectItem(address, "road");
		cJSON* distrito = cJSON_GetObjectItem(address, "city_district");
		cJSON* number = cJSON_GetObjectItem(address, "house_number");

		if (!lat || !lon) {
			cJSON_Delete(rootNominatim);
			continue;
		}

		// Construimos la dirección con estrategia de fallback

		char direccion[256];

		// Calle + número (lo más específico)

		if (road && number) {
			snprintf(direccion, sizeof(direccion), "%s, %s", road->valuestring, number->valuestring);
		}

		// Solo calle

		else if (road) {
			snprintf(direccion, sizeof(direccion), "%s", road->valuestring);
		}

		// Distrito

		else if (distrito) {
			snprintf(direccion, sizeof(direccion), "%s", distrito->valuestring);
		}

		// Barrio

		else {
			cJSON* barrio = cJSON_GetObjectItem(address, "suburb");
			if (barrio) {
				snprintf(direccion, sizeof(direccion), "%s", barrio->valuestring);
			}

			// Municipio

			else {
				cJSON* municipio = cJSON_GetObjectItem(address, "municipality");
				if (municipio) {
					snprintf(direccion, sizeof(direccion), "%s", municipio->valuestring);
				}

				// Display_name recortado (último recurso)

				else {
					cJSON* display = cJSON_GetObjectItem(resultado, "display_name");
					if (!display) {
						cJSON_Delete(rootNominatim);
						continue;
					}
					strncpy(direccion, display->valuestring, sizeof(direccion));
					char* coma = strchr(direccion, ',');
					if (coma) *coma = '\0';
				}
			}
		}

		// Creamos la ubicación

		Ubicacion* ubi = malloc(sizeof(Ubicacion));
		ubi->direccion = strdup(direccion);
		ubi->latitud = atof(lat->valuestring);
		ubi->longitud = atof(lon->valuestring);
		ubi->ciudad = ciudad;
		ubi->correo = NULL;
		ubi->activo = 1;

		cJSON_Delete(rootNominatim);
		if (root) cJSON_Delete(root);

		printf("Ubicación creada: %s en %s\n", ubi->direccion, ciudad.nombre);

		Sleep(1000);

		return ubi;

	}

	// Si llegamos aquí ningún intento funcionó

	printf("Sin resultado en Nominatim para %s tras %d intentos, saltando\n", ciudad.nombre, maxIntentos);
	if (root) cJSON_Delete(root);
	return NULL;

}

// Función que a partir de un país, ciudad y dirección, devuelve un puntero a ubicación correspondiente

int completarUbicacion(Ubicacion* ubi, char* pais, char* ciudad, char* direccion) {

	// Codificamos y construimos la URL de Nominatim

	CURL* curlTmp = curl_easy_init();
	char* paisEncoded = curl_easy_escape(curlTmp, pais, 0);
	char* ciudadEncoded = curl_easy_escape(curlTmp, ciudad, 0);
	char* direccionEncoded = curl_easy_escape(curlTmp, direccion, 0);

	char urlNominatim[1024];
	snprintf(urlNominatim, sizeof(urlNominatim), "https://nominatim.openstreetmap.org/search?q=%s+%s+%s&format=jsonv2", paisEncoded, ciudadEncoded, direccionEncoded);

	curl_free(paisEncoded);
	curl_free(ciudadEncoded);
	curl_free(direccionEncoded);
	curl_easy_cleanup(curlTmp);

	// Llamamos a Nominatim

	Respuesta* respNominatim = llamarApi(urlNominatim);
	if (!respNominatim) return 1;

	cJSON* rootNominatim = cJSON_Parse(respNominatim->data);
	liberarRespuesta(respNominatim);
	if (!rootNominatim) return 1;

	// Nominatim devuelve un array, cogemos el primer resultado

	cJSON* resultado = cJSON_GetArrayItem(rootNominatim, 0);
	if (!resultado) {
		printf("Nominatim vacío para %s en %s %s\n", direccion, ciudad, pais);
		cJSON_Delete(rootNominatim);
		return 1;
	}

	// Extraemos coordenadas

	cJSON* lat = cJSON_GetObjectItem(resultado, "lat");
	cJSON* lon = cJSON_GetObjectItem(resultado, "lon");

	// Recogemos la información que nos interesa

	ubi->ciudad.pais.nombre = strdup(pais);
	ubi->ciudad.nombre = strdup(ciudad);
	ubi->direccion = strdup(direccion);
	ubi->latitud = atof(lat->valuestring);
	ubi->longitud = atof(lon->valuestring);

	cJSON_Delete(rootNominatim);

	return 0;

}
