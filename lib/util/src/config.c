#include "config.h"
#include <stdio.h>
#include <string.h>

int configGet(const char* rutaIni, const char* clave, char* salida, int maxLen) {

	FILE* f = fopen(rutaIni, "r");
    if (!f) return 0;

    char linea[512];
    while (fgets(linea, sizeof(linea), f)) {

        // Ignoramos comentarios y secciones
        if (linea[0] == '#' || linea[0] == ';' || linea[0] == '[') continue;

        char* igual = strchr(linea, '=');
        if (!igual) continue;

        *igual = '\0';
        char* k = linea;
        char* v = igual + 1;

        // Trim espacios
        while (*k == ' ') k++;
        int klen = strlen(k);
        while (klen > 0 && (k[klen-1] == ' ' || k[klen-1] == '\t')) k[--klen] = '\0';

        while (*v == ' ') v++;
        int len = strlen(v);
        while (len > 0 && (v[len-1] == '\n' || v[len-1] == '\r' || v[len-1] == ' ')) v[--len] = '\0';

        if (strcmp(k, clave) == 0) {
            strncpy(salida, v, maxLen - 1);
            salida[maxLen - 1] = '\0';
            fclose(f);
            return 1;
        }

    }

    fclose(f);
    return 0;
}
