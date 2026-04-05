#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "curl/curl.h"
#include "cJSON.h"
#include "sqlite3.h"
#include "estructuras.h"
#include "api.h"

static char* SEED_PAISES = "../data/db/seeds/paises.csv";
static char* SEED_CIUDADES = "../data/db/seeds/cities15000.txt";
static char* SEED_CATEGORIAS = "../data/db/seeds/categorias.csv";
static char* SEED_PRODUCTOS = "../data/db/seeds/productos.csv";

// Split genérico por cualquier delimitador, respetando campos vacíos

int split(char* linea, char** campos, int max, char delimitador) {

	int i = 0;
    int enComillas = 0;
    campos[0] = linea;

    while (*linea && i < max - 1) {

        if (*linea == '"') {
            enComillas = !enComillas;  // entramos o salimos de comillas
        }

        else if (*linea == delimitador && !enComillas) {

            *linea = '\0';
            campos[++i] = linea + 1;

        }

        linea++;

    }

    return i + 1;

}

// Quita comillas dobles del principio y final de un campo
// También quita el \n y \r si los hubiera

void limpiarCampo(char* str) {

    if (!str) return;

    // Quitar \r y \n del final
    int len = strlen(str);

    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[--len] = '\0';
    }

    // Quitar comilla del final
    if (len > 0 && str[len - 1] == '"') str[--len] = '\0';

    // Quitar comilla del principio desplazando
    if (len > 0 && str[0] == '"') {
        memmove(str, str + 1, len);
    }

}

// Función que devuelve 0 / 1 dependiendo de si un array contiene un elemento especificado

int containsInt(int* array, int size, int n) {

	for (int i = 0 ; i < size ; i++) if (array[i] == n) return 1;

	return 0;

}

// Función para poblar la db con todos los paises

int seedPaises(sqlite3* db) {

    sqlite3_stmt* pstmt;
    char* sql = "INSERT INTO PAIS (ID_PA, NOM_PA) VALUES (?, ?)";
    int result = sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL);

    if (result != SQLITE_OK) {
        fprintf(stderr, "Error preparando statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    FILE* paises = fopen(SEED_PAISES, "r");
    if (!paises) {
        fprintf(stderr, "Error abriendo %s\n", SEED_PAISES);
        sqlite3_finalize(pstmt);
        return -1;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    char linea[512];
    int contador = 0;

    while (fgets(linea, sizeof(linea), paises) != NULL) {

        // Saltamos cabecera nombre, name, nom, iso2, iso3, phone_code, continente

        if (contador == 0) { contador++; continue; }

        char** campos = malloc(sizeof(char*) * 7);
        split(linea, campos, 7, ',');

        // Limpiamos campos que usamos

        limpiarCampo(campos[0]);
        limpiarCampo(campos[3]);

        result = sqlite3_bind_text(pstmt, 1, campos[3], -1, SQLITE_STATIC);
        result = sqlite3_bind_text(pstmt, 2, campos[0], -1, SQLITE_STATIC);

        result = sqlite3_step(pstmt);
        if (result != SQLITE_DONE) {
            fprintf(stderr, "Error insertando país %s: %s\n", campos[0], sqlite3_errmsg(db));
        }
        sqlite3_reset(pstmt);

        free(campos);
        contador++;

    }

	sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    fclose(paises);
    sqlite3_finalize(pstmt);

    printf("Se han insertado %d países en la BD\n", contador - 1);
    return 0;

}

// Función para poblar la db con todas las ciudades de +15k habitantes

int seedCiudades(sqlite3* db) {

    sqlite3_stmt* pstmt;
    char* sql = "INSERT INTO CIUDAD (NOM_CIU, LAT_CIU, LON_CIU, POBLACION_CIU, ID_PA) VALUES (?, ?, ?, ?, ?)";
    int result = sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL);

    if (result != SQLITE_OK) {
        fprintf(stderr, "Error preparando statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    FILE* ciudades = fopen(SEED_CIUDADES, "r");
    if (!ciudades) {
        fprintf(stderr, "Error abriendo %s\n", SEED_CIUDADES);
        sqlite3_finalize(pstmt);
        return -1;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    char linea[4096];
    int contador = 0;

    while (fgets(linea, sizeof(linea), ciudades) != NULL) {

        char** campos = malloc(sizeof(char*) * 19);
        split(linea, campos, 19, '\t');

        // Limpiamos los campos que usamos
        limpiarCampo(campos[1]);   // nombre
        limpiarCampo(campos[4]);   // latitud
        limpiarCampo(campos[5]);   // longitud
        limpiarCampo(campos[8]);   // código país
        limpiarCampo(campos[14]);  // población

        result = sqlite3_bind_text(pstmt, 1, campos[1], -1, SQLITE_STATIC);
        result = sqlite3_bind_double(pstmt, 2, atof(campos[4]));
        result = sqlite3_bind_double(pstmt, 3, atof(campos[5]));
        result = sqlite3_bind_int(pstmt, 4, atoi(campos[14]));
        result = sqlite3_bind_text(pstmt, 5, campos[8], -1, SQLITE_STATIC);

        result = sqlite3_step(pstmt);
        if (result != SQLITE_DONE) {
            fprintf(stderr, "Error insertando ciudad %s: %s\n", campos[1], sqlite3_errmsg(db));
        }
        sqlite3_reset(pstmt);

        free(campos);
        contador++;

    }

	sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    fclose(ciudades);
    sqlite3_finalize(pstmt);

    printf("Se han insertado %d ciudades en la BD\n", contador);
    return 0;
}

// Función que rellena la BD con almacenes de la siguiente manera: cantidad de almacenes y tamaño de cada uno dependen del tamaño de la ciudad

int seedAlmacenes(sqlite3* db) {

    // Obtenemos todas las ciudades con +100K habitantes
    sqlite3_stmt* pstmtCiudades;

    char* sqlCiudades = "SELECT ID_CIU, NOM_CIU, LAT_CIU, LON_CIU, POBLACION_CIU, ID_PA FROM CIUDAD WHERE POBLACION_CIU >= 200000";

    int result = sqlite3_prepare_v2(db, sqlCiudades, -1, &pstmtCiudades, NULL);
    if (result != SQLITE_OK) {
        fprintf(stderr, "Error preparando ciudades: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    // Preparamos los INSERT
    sqlite3_stmt* pstmtUbi;
    sqlite3_stmt* pstmtAlm;

    char* sqlUbi = "INSERT INTO UBICACION (DIR_UB, LAT_UB, LON_UB, ID_CIU) VALUES (?, ?, ?, ?)";
    char* sqlAlm = "INSERT INTO ALMACEN (NOM_ALM, CAP_MAX, ID_UB) VALUES (?, ?, ?)";

    sqlite3_prepare_v2(db, sqlUbi, -1, &pstmtUbi, NULL);
    sqlite3_prepare_v2(db, sqlAlm, -1, &pstmtAlm, NULL);

    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    int totalAlmacenes = 0;

    while (sqlite3_step(pstmtCiudades) == SQLITE_ROW) {

        Ciudad ciudad;

        ciudad.id = sqlite3_column_int(pstmtCiudades, 0);
        ciudad.nombre = strdup((char*)sqlite3_column_text(pstmtCiudades, 1));
        ciudad.latitud = sqlite3_column_double(pstmtCiudades, 2);
        ciudad.longitud = sqlite3_column_double(pstmtCiudades, 3);
        ciudad.poblacion = sqlite3_column_int(pstmtCiudades, 4);

        // Factor de población entre 0 y 1

        double factor = min(1.0, (double)(ciudad.poblacion - 200000) / (6000000 - 200000));

        int almacenesCiudad = 0;

        // Intentamos crear hasta 2 almacenes

        for (int i = 0; i < 2; i++) {

            double prob = (double)rand() / RAND_MAX;

            if (prob > factor) continue; // no se crea almacén

            // Capacidad entre 50000 y 5000000

            int capacidad = 10000 * (int)(factor * 495) + 50000;
            int variacion = 10000 * (int)(((double)rand() / RAND_MAX) * 21);
            capacidad = capacidad - variacion;
            capacidad = max(50000, capacidad);

            // Obtenemos ubicación real

            Ubicacion* ubi = calleAleatoria(ciudad);
            if (!ubi) break;

            // Insertamos la ubicación

            sqlite3_bind_text(pstmtUbi, 1, ubi->direccion, -1, SQLITE_STATIC);
            sqlite3_bind_double(pstmtUbi, 2, ubi->latitud);
            sqlite3_bind_double(pstmtUbi, 3, ubi->longitud);
            sqlite3_bind_int(pstmtUbi, 4, ciudad.id);
            sqlite3_step(pstmtUbi);
            sqlite3_reset(pstmtUbi);

            // Obtenemos el ID de la ubicación recién insertada

            int idUbi = (int) sqlite3_last_insert_rowid(db);

            // Nombre del almacén

            char nombre[256];
            snprintf(nombre, sizeof(nombre), "%s - Centro logístico %d", ciudad.nombre, almacenesCiudad + 1);

            // Insertamos el almacén

            sqlite3_bind_text(pstmtAlm, 1, nombre, -1, SQLITE_STATIC);
            sqlite3_bind_int(pstmtAlm, 2, capacidad);
            sqlite3_bind_int(pstmtAlm, 3, idUbi);
            sqlite3_step(pstmtAlm);
            sqlite3_reset(pstmtAlm);

            free(ubi->direccion);
            free(ubi);

            almacenesCiudad++;
            totalAlmacenes++;

            printf("Se ha insertado el almacén %s, con %d de capacidad\n", nombre, capacidad);

        }

        free(ciudad.nombre);

    }

	sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    sqlite3_finalize(pstmtCiudades);
    sqlite3_finalize(pstmtUbi);
    sqlite3_finalize(pstmtAlm);

    printf("Se han insertado %d almacenes en la BD\n", totalAlmacenes);

    return 0;

}

// Función para poblar la db con todas las categorías (las tallas posibles de cada categoría se meterán a mano)

int seedCategorias(sqlite3* db) {

    sqlite3_stmt* pstmt;
    char* sql = "INSERT INTO CATEGORIA (NOM_CAT, VARIANTES_CAT) VALUES (?, ?)";
    int result = sqlite3_prepare_v2(db, sql, -1, &pstmt, NULL);

    if (result != SQLITE_OK) {
        fprintf(stderr, "Error preparando statement: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    FILE* categorias = fopen(SEED_CATEGORIAS, "r");
    if (!categorias) {
        fprintf(stderr, "Error abriendo %s\n", SEED_CATEGORIAS);
        sqlite3_finalize(pstmt);
        return -1;
    }

    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    char linea[1024];
    int contador = 0;

    while (fgets(linea, sizeof(linea), categorias) != NULL) {

        // Saltamos cabecera "id";"nombre"
        if (contador == 0) { contador++; continue; }

        char** campos = malloc(sizeof(char*) * 3);
        split(linea, campos, 3, ';');

        // Limpiamos los campos que usamos

        limpiarCampo(campos[1]);
        limpiarCampo(campos[2]);

        result = sqlite3_bind_text(pstmt, 1, campos[1], -1, SQLITE_STATIC);
        result = sqlite3_bind_text(pstmt, 2, campos[2], -1, SQLITE_STATIC);
        result = sqlite3_step(pstmt);
        if (result != SQLITE_DONE) {
            fprintf(stderr, "Error insertando categoria %s: %s\n", campos[1], sqlite3_errmsg(db));
        }
        sqlite3_reset(pstmt);

        free(campos);
        contador++;
    }

	sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

    fclose(categorias);
    sqlite3_finalize(pstmt);

    printf("Se han insertado %d categorias en la BD\n", contador - 1);

    return 0;

}

// Función para poblar la db con todos los productos

int seedProductos(sqlite3* db) {

	sqlite3_stmt* pstmtProducto;
	sqlite3_stmt* pstmtResena;

	char* sqlProducto = "INSERT INTO PRODUCTO (NOM_PR, DESCRIP_PR, PRECIO_PR, DESCTO_PR, ID_CAT) VALUES (?, ?, ?, ?, ?)";
	char* sqlResena = "INSERT INTO RESENA (ID_PR, CORREO, VALORACION, FECHA, PESO) VALUES (?, 'ADMIN', ?, '2026-01-01', ?)";

	int result = sqlite3_prepare_v2(db, sqlProducto, -1, &pstmtProducto, NULL);

	if (result != SQLITE_OK) {
	    fprintf(stderr, "Error preparando PRODUCTO: %s\n", sqlite3_errmsg(db));
	    return -1;
	}

	result = sqlite3_prepare_v2(db, sqlResena, -1, &pstmtResena, NULL);

	if (result != SQLITE_OK) {
	    fprintf(stderr, "Error preparando RESEÑA: %s\n", sqlite3_errmsg(db));
	    return -1;
	}

	FILE* productos = fopen(SEED_PRODUCTOS, "r");
	if (!productos) {
	    fprintf(stderr, "Error abriendo %s\n", SEED_PRODUCTOS);
	    sqlite3_finalize(pstmtProducto);
	    sqlite3_finalize(pstmtResena);
	    return -1;
	}

	char linea[4096];
	int contador = 0;

	while (fgets(linea, sizeof(linea), productos) != NULL) {

		// Saltamos cabecera "nombre";"descripcion";"precio";"descuento";"id_categoria";"stars";"reviews"

		if (contador == 0) { contador++; continue; }

		char** campos = malloc(sizeof(char*) * 7);
		split(linea, campos, 7, ';');

		// Limpiamos los campos que usamos

		limpiarCampo(campos[0]);
		limpiarCampo(campos[1]);
		limpiarCampo(campos[2]);
		limpiarCampo(campos[3]);
		limpiarCampo(campos[4]);
		limpiarCampo(campos[5]);
		limpiarCampo(campos[6]);

		result = sqlite3_bind_text(pstmtProducto, 1, campos[0], -1, SQLITE_STATIC);
		result = sqlite3_bind_text(pstmtProducto, 2, campos[1], -1, SQLITE_STATIC);
		result = sqlite3_bind_double(pstmtProducto, 3, atof(campos[2]));
		result = sqlite3_bind_double(pstmtProducto, 4, atof(campos[3]));
		result = sqlite3_bind_int(pstmtProducto, 5, atoi(campos[4]));

		result = sqlite3_step(pstmtProducto);
		if (result != SQLITE_DONE) {
		    fprintf(stderr, "Error insertando producto %s: %s\n", campos[0], sqlite3_errmsg(db));
		    sqlite3_reset(pstmtProducto);
		    free(campos);
		    contador++;
		    continue;
		}

		if (atoi(campos[6])) {

			int idProd = sqlite3_last_insert_rowid(db);

			result = sqlite3_bind_int(pstmtResena, 1, idProd);
			result = sqlite3_bind_double(pstmtResena, 2, atof(campos[5]));
			result = sqlite3_bind_int(pstmtResena, 3, atoi(campos[6]));

			result = sqlite3_step(pstmtResena);
			if (result != SQLITE_DONE) {
				fprintf(stderr, "Error insertando reseña para producto con ID = %d: %s\n", idProd, sqlite3_errmsg(db));
			}

			sqlite3_reset(pstmtResena);

		}

		sqlite3_reset(pstmtProducto);

		free(campos);
		contador++;

	}

	sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

	fclose(productos);
	sqlite3_finalize(pstmtProducto);
	sqlite3_finalize(pstmtResena);

	printf("Se han insertado %d productos y reseñas en la BD\n", contador - 1);

	return 0;

}

// Función para poblar los almacenes con productos

int seedStock(sqlite3* db) {

	// Preparamos el conteo de productos (para saber cuantos productos hay en total - para el random)

	sqlite3_stmt* pstmtCount;
	char* sqlCount = "SELECT COUNT(*) FROM PRODUCTO";

	int result = sqlite3_prepare_v2(db, sqlCount, -1, &pstmtCount, NULL);
	if (result != SQLITE_OK) {
		fprintf(stderr, "Error preparando conteo: %s\n", sqlite3_errmsg(db));
		return -1;
	}

	int nProds;

	if (sqlite3_step(pstmtCount) == SQLITE_ROW) nProds = sqlite3_column_int(pstmtCount, 0);

	// Preparamos la búsqueda de almacenes (para obtener la cap_max de cada uno)

	sqlite3_stmt* pstmtAlmacenes;
	char* sqlAlmacenes = "SELECT ID_ALM, CAP_MAX FROM ALMACEN ORDER BY ID_ALM ASC";

	result = sqlite3_prepare_v2(db, sqlAlmacenes, -1, &pstmtAlmacenes, NULL);
	if (result != SQLITE_OK) {
		fprintf(stderr, "Error preparando almacenes: %s\n", sqlite3_errmsg(db));
		return -1;
	}

	// Preparamos la búsqueda de producto con su categoría para obtener las variantes que puede tener

	sqlite3_stmt* pstmtProducto;
	char* sqlProducto = "SELECT C.VARIANTES_CAT FROM PRODUCTO P, CATEGORIA C WHERE P.ID_CAT = C.ID_CAT AND P.ID_PR = ?";

	result = sqlite3_prepare_v2(db, sqlProducto, -1, &pstmtProducto, NULL);
	if (result != SQLITE_OK) {
		fprintf(stderr, "Error preparando variantes de producto a seleccionar: %s\n", sqlite3_errmsg(db));
		return -1;
	}

	// Preparamos la inserción de stock

	sqlite3_stmt* pstmtInsert;
	char* sqlInsert = "INSERT INTO STOCK_ALMACEN (ID_PR, ID_ALM, VARIANTE, DISPONIBLE, CANT) VALUES (?, ?, ?, 1, ?)";

	result = sqlite3_prepare_v2(db, sqlInsert, -1, &pstmtInsert, NULL);
	if (result != SQLITE_OK) {
		fprintf(stderr, "Error preparando la inserción: %s\n", sqlite3_errmsg(db));
		return -1;
	}

	sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

	// Recorremos todos los almacenes

	int totalAlmacenes = 0;
	int totalProductosInsertados = 0;

	while (sqlite3_step(pstmtAlmacenes) == SQLITE_ROW) {

		// Obtenemos los datos del almacén

		int idAlm = sqlite3_column_int(pstmtAlmacenes, 0);
		int capMax = sqlite3_column_int(pstmtAlmacenes, 1);

		// Calculamos cuanto queremos rellenar del almacén (aproximadamente)

		int aInsertarTotal = (int) ((((double)rand() / RAND_MAX) * 0.5 + 0.4) * capMax); 		// Rellenar el almacen un 40-90%
		int productosInsertados = 0;

		// Reservamos memoria para los productos insertados

		int* idsProductosSeleccionados = malloc(sizeof(int) * 4000);
		int productosSeleccionados = 0;

		while (productosInsertados < aInsertarTotal) {

			// Seleccionamos un producto al azar que no esté en la lista

			int idProdSeleccionado;

			do {
				idProdSeleccionado = (int)(((double)(rand() * 32768 + rand()) / (32768.0 * 32768.0)) * nProds + 1);
			} while (containsInt(idsProductosSeleccionados, productosSeleccionados, idProdSeleccionado));

			idsProductosSeleccionados[productosSeleccionados] = idProdSeleccionado;
			productosSeleccionados++;

			// Buscamos las variantes a insertar del producto

			result = sqlite3_bind_int(pstmtProducto, 1, idProdSeleccionado);

			char variantesJunto[2048];
			if (sqlite3_step(pstmtProducto) == SQLITE_ROW) strcpy(variantesJunto, (char*) sqlite3_column_text(pstmtProducto, 0));

			char** variantesSeparado = malloc(sizeof(char*) * 8);
			int nVariantes = split(variantesJunto, variantesSeparado, 8, ',');

			// Calculamos cuantos ejemplares de dicho producto vamos a insertar en este almacén

			int aInsertarProd = (int) ((((double)rand() / RAND_MAX) * 0.0008 + 0.0001) * capMax);
			int aInsertarVariante = (int) (aInsertarProd / nVariantes);

			double centro = (nVariantes - 1) / 2.0;

			for (int i = 0 ; i < nVariantes ; i++) {

				double pesoBase = 1;

				if (nVariantes > 1) {

				    // Esto va de -1 (borde izquierdo) a 1 (borde derecho), pasando por 0 (centro)
				    double distNormalizada = (i - centro) / centro;

				    // En el centro restará 0 (da 1.25). En los bordes restará 0.5 (da 0.75).
				    pesoBase = 1.25 - (0.5 * (distNormalizada * distNormalizada));

				}

				double ruido = (((double)rand() / RAND_MAX) * 0.30) - 0.15;

				double multiplicador = pesoBase + ruido;

				int aInsertarVarianteRng = (int) (multiplicador * aInsertarVariante);

				result = sqlite3_bind_int(pstmtInsert, 1, idProdSeleccionado);
				result = sqlite3_bind_int(pstmtInsert, 2, idAlm);
				result = sqlite3_bind_text(pstmtInsert, 3, variantesSeparado[i], -1, SQLITE_STATIC);
				result = sqlite3_bind_int(pstmtInsert, 4, aInsertarVarianteRng);		// Se añade una cantidad diferente de cada variante en función de como de común sea cada una (Es más dificil encontrar una talla 46 que una 42, pero no demasiado)

				result = sqlite3_step(pstmtInsert);
				if (result != SQLITE_DONE) {
					fprintf(stderr, "Error insertando stock para almacén con ID=%d: %s\n", idAlm, sqlite3_errmsg(db));
				}

				productosInsertados += aInsertarVarianteRng;

				sqlite3_reset(pstmtInsert);

			}

			sqlite3_reset(pstmtProducto);

			free(variantesSeparado);

		}

		totalAlmacenes++;
		totalProductosInsertados += productosInsertados;

		// Liberamos memoria de la lista de ids de productos insertados

		free(idsProductosSeleccionados);

	}

	sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);

	sqlite3_finalize(pstmtCount);
	sqlite3_finalize(pstmtAlmacenes);
	sqlite3_finalize(pstmtProducto);
	sqlite3_finalize(pstmtInsert);

	printf("Se han insertado %d productos en %d almacenes\n", totalProductosInsertados, totalAlmacenes);

	return 0;

}
