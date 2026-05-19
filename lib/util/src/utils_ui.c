#include <utils_ui.h>

// SISTEMA (Funciones hechas y entendidas con ayuda de Claude)

void activarColores() {

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // UTF-8 para caracteres Unicode
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

}

void limpiarPantalla() {

    system("cls");

}

void pausar() {

    printf("\n" ESTILO_HINT "  Pulsa ENTER para continuar..." RESET);
    while (_getch() != TECLA_ENTER);
    printf("\n");

}

// AUXILIAR (Hechas por Claude)

// Decodificador
static unsigned int decodificarUTF8(const unsigned char** p) {
    const unsigned char* s = *p;
    if (*s == 0) return 0;

    if (s[0] < 0x80) { *p += 1; return s[0]; }

    int bytes_esperados = 0;
    if ((s[0] & 0xE0) == 0xC0) bytes_esperados = 2;
    else if ((s[0] & 0xF0) == 0xE0) bytes_esperados = 3;
    else if ((s[0] & 0xF8) == 0xF0) bytes_esperados = 4;

    if (bytes_esperados > 0) {
        int valido = 1;
        for (int i = 1; i < bytes_esperados; i++) {
            if (s[i] == 0 || (s[i] & 0xC0) != 0x80) { valido = 0; break; }
        }
        if (valido) {
            unsigned int cp = 0;
            if (bytes_esperados == 2) cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F);
            else if (bytes_esperados == 3) cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
            else cp = ((s[0] & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
            *p += bytes_esperados;
            return cp;
        }
    }

    // Si la cadena está truncada, avanzamos y nos saltamos la "basura"
    // para que cuente como UN SOLO caracter de reemplazo () de ancho 1.
    *p += 1;
    while (**p != 0 && (**p & 0xC0) == 0x80) {
        *p += 1;
    }
    return 0xFFFD;
}

// Rangos
static int anchoCodpoint(unsigned int cp) {
    if (cp == 0 || cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0xFEFF) return 0;
    if (cp < 32 || (cp >= 0x7F && cp < 0xA0)) return 0;

    if ((cp >= 0x0300 && cp <= 0x036F) ||
        (cp == 0x0901 || cp == 0x0902) ||
        (cp == 0x093C) ||
        (cp >= 0x0941 && cp <= 0x0948) ||
        (cp == 0x094D) ||
        (cp == 0x0981) ||
        (cp == 0x09BC) ||
        (cp >= 0x09C1 && cp <= 0x09C4) ||
        (cp == 0x09CD) ||
        (cp >= 0x09E2 && cp <= 0x09E3) ||
        (cp == 0x0E31) ||
        (cp >= 0x0E34 && cp <= 0x0E3A) ||
        (cp >= 0x0E47 && cp <= 0x0E4E) ||
        (cp >= 0x102D && cp <= 0x1030) ||
        (cp >= 0x1032 && cp <= 0x1037) ||
        (cp >= 0x1039 && cp <= 0x103A) ||
        (cp >= 0x103D && cp <= 0x103E) ||
        (cp >= 0x1DC0 && cp <= 0x1DFF) ||
        (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0xFE20 && cp <= 0xFE2F))
        return 0;

    if ((cp >= 0x1100 && cp <= 0x115F)  ||
        cp == 0x2329 || cp == 0x232A    ||
        (cp >= 0x2E80 && cp <= 0x303E)  ||
        (cp >= 0x3040 && cp <= 0x33FF)  ||
        (cp >= 0x3400 && cp <= 0x4DBF)  ||
        (cp >= 0x4E00 && cp <= 0x9FFF)  ||
        (cp >= 0xA000 && cp <= 0xA4CF)  ||
        (cp >= 0xA960 && cp <= 0xA97F)  ||
        (cp >= 0xAC00 && cp <= 0xD7AF)  ||
        (cp >= 0xF900 && cp <= 0xFAFF)  ||
        (cp >= 0xFE10 && cp <= 0xFE19)  ||
        (cp >= 0xFE30 && cp <= 0xFE4F)  ||
        (cp >= 0xFF00 && cp <= 0xFF60)  ||
        (cp >= 0xFFE0 && cp <= 0xFFE6)  ||
        (cp >= 0x1B000 && cp <= 0x1B0FF) ||
        (cp >= 0x1F300 && cp <= 0x1F9FF) ||
        (cp >= 0x20000 && cp <= 0x2FFFD) ||
        (cp >= 0x30000 && cp <= 0x3FFFD))
        return 2;

    return 1;
}

// Devuelve el ancho real de un str
static int anchoVisible(const char* str) {
    int visibles = 0;
    const unsigned char* p = (const unsigned char*)str;
    int modo_apilar = 0; // 1 si el próximo caracter normal debe dibujarse debajo/fusionado

    while (*p) {
        if (*p == '\033') {
            p++;
            if (*p == '[') {
                p++;
                while (*p && *p != 'm') p++;
                if (*p) p++;
            }
            continue;
        }

        unsigned int cp = decodificarUTF8(&p);
        if (cp == 0) break;

        int ancho = anchoCodpoint(cp);

        // Si el interruptor está activo y el caracter actual es de los que ocupan espacio
        if (modo_apilar && ancho == 1) {
            ancho = 0;        // Se apila, anulamos su ancho
            modo_apilar = 0;  // Apagamos el interruptor
        }
        // Si detectamos un Virama Bengalí o Stacker Birmano, encendemos el interruptor
        else if (cp == 0x09CD || cp == 0x1039) {
            modo_apilar = 1;
        }

        visibles += ancho;
    }
    return visibles;
}

void wordWrap(char* nuevo, char* texto, int padding) {

    char* src = texto;
    int lon = strlen(src);
    int pos = 0;   // posición en src
    int dst = 0;   // posición en nombre

    while (pos < lon) {

        // Cogemos hasta 72 caracteres
        int fin = pos + 108;
        if (fin >= lon) {

            // Última línea, copiamos el resto
            strcpy(&nuevo[dst], &src[pos]);
            break;

        }

        // Buscamos el último espacio antes del límite
        int corte = fin;

        while (corte > pos && src[corte] != ' ') corte--;
        if (corte == pos) corte = fin;  // sin espacio, cortamos duro

        // Copiamos la línea
        memcpy(&nuevo[dst], &src[pos], corte - pos);
        dst += corte - pos;

        // Añadimos salto de línea e indentación
        nuevo[dst++] = '\n';
        for (int i = 0 ; i < padding ; i++) nuevo[dst++] = ' ';

        pos = corte + 1;  // saltamos el espacio

    }

}

// LAYOUT

#define APPEND(...) do { \
    int _w = snprintf(temp + pos, sizeof(temp) - pos, __VA_ARGS__); \
    if (_w > 0 && pos + _w < sizeof(temp)) pos += _w; \
} while(0)

char* getLinea(char c, int ancho) {

	char* buffer = malloc(sizeof(char) * (ancho + 2));
	if (buffer == NULL) return NULL;

	int pos = 0;
	for (int i = 0; i < ancho; i++) buffer[pos++] = c;
	buffer[pos++] = '\n';
	buffer[pos] = '\0';

	return buffer;

}

void imprimirLinea(char c, int ancho) {

	char* linea = getLinea(c, ancho);
    printf("%s", linea);
    free(linea);

}

char* getCentrado(char* texto, int ancho) {

    int len = strlen(texto);

    int tamañoTotal = (ancho > len) ? ancho : len;

    char* buffer = malloc(tamañoTotal + 1);

    int pad = (ancho - len) / 2;
    int padDer = ancho - len - pad;

    int pos = 0;

    for (int i = 0; i < pad; i++) buffer[pos++] = ' ';
    for (int i = 0; i < len; i++) buffer[pos++] = texto[i];
    for (int i = 0; i < padDer; i++) buffer[pos++] = ' ';

    buffer[pos] = '\0';

    return buffer;

}

void imprimirCentrado(char* texto, int ancho) {

	char* centrado = getCentrado(texto, ancho);
	printf("%s", centrado);
	free(centrado);

}

char* getCabecera(char* titulo, char* subtitulo) {

    char temp[8192] = ""; // 8KB es espacio de sobra para este bloque
    int pos = 0;

    // Franja superior con logo
    APPEND("\n" ESTILO_TITULO
           "  ╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n"
           "  ║                                                                                                              ║\n"
           "  ║" RESET C_BLANCO NEGRITA
              "                                  DEUSTO" RESET C_CYAN NEGRITA
 			                                          "COMMERCE" RESET C_BLANCO NEGRITA
                                                               "  —  Panel de Administración                                  "RESET ESTILO_TITULO
 								                                                                                            "║\n"
           "  ║                                                                                                              ║\n"
           "  ╠══════════════════════════════════════════════════════════════════════════════════════════════════════════════╣\n");

    // Título de la pantalla actual
    APPEND("  ║  " ESTILO_SUBTITULO "%s" RESET, titulo);

    int visibles = anchoVisible(titulo);
    for (int i = visibles; i < 108; i++) APPEND(" ");

    APPEND(ESTILO_TITULO "║\n");

    // Subtítulo si existe
    if (subtitulo && strlen(subtitulo) > 0) {
    	char subtituloStr[109];
    	strncpy(subtituloStr, subtitulo, sizeof(subtituloStr));
    	subtituloStr[108] = '\0';

        APPEND("  ║  " ESTILO_HINT "%s" RESET, subtituloStr);

        visibles = anchoVisible(subtitulo);
        for (int i = visibles; i < 108; i++) APPEND(" ");

        APPEND(ESTILO_TITULO "║\n");
    }

    APPEND("  ╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n" RESET "\n");

    // Copiamos el resultado exacto a la memoria dinámica
    char* buffer = malloc(pos + 1);
    if (buffer) strcpy(buffer, temp);
    return buffer;

}

void imprimirCabecera(char* titulo, char* subtitulo) {

    limpiarPantalla();
    char* cabecera = getCabecera(titulo, subtitulo);
    printf("%s", cabecera);
    free(cabecera);

}

char* getSeccion(char* titulo) {

    char temp[2048] = "";
    int pos = 0;

    APPEND("\n" C_GRIS "  ── " ESTILO_SUBTITULO "%s " RESET, titulo);
    int len = strlen(titulo) + 6;

    for (int i = len; i < ANCHO_PANTALLA - 2; i++) {
        APPEND(C_GRIS "─" RESET);
    }

    APPEND("\n\n");

    char* buffer = malloc(pos + 1);
    if (buffer) strcpy(buffer, temp);
    return buffer;

}

void imprimirSeccion(char* titulo) {

	char* seccion = getSeccion(titulo);
	printf("%s", seccion);
	free(seccion);

}

char* getDuracion(time_t segundos) {

	int dias = segundos / (24 * 3600);
    segundos %= (24 * 3600);
    int horas = segundos / 3600;
    segundos %= 3600;
    int minutos = segundos / 60;

    char display[512] = "";
    int pos = 0;

    if (dias) pos += snprintf(display + pos, sizeof(display) - pos, "%d dias, ", dias);
    if (horas) pos += snprintf(display + pos, sizeof(display) - pos, "%d horas, ", horas);
    pos += snprintf(display + pos, sizeof(display) - pos, "%d minutos, ", minutos);

    if (pos > 0) {
        display[pos - 2] = '\0';
    }

    // Reservamos memoria para el string final (+2 por el salto de línea y el \0)
    char* buffer = malloc(strlen(display) + 2);
    if (buffer) {
        sprintf(buffer, "%s\n", display);
    }

    return buffer;

}

void imprimirDuracion(time_t segundos) {

	char* duracion = getDuracion(segundos);
	printf("%s", duracion);
	free(duracion);

}

// FEEDBACK

char* getExito(char* msg) {

    int len = strlen(msg) + 128;
    char* buffer = malloc(len);
    if (buffer) snprintf(buffer, len, "\n  " ESTILO_EXITO "✔  %s" RESET "\n", msg);
    return buffer;

}

void imprimirExito(char* msg) {
    printf("\n  " ESTILO_EXITO "✔  %s" RESET "\n", msg);
}

char* getError(char* msg) {

    int len = strlen(msg) + 128;
    char* buffer = malloc(len);
    if (buffer) snprintf(buffer, len, "\n  " ESTILO_ERROR "✖  %s" RESET "\n", msg);
    return buffer;

}

void imprimirError(char* msg) {
    printf("\n  " ESTILO_ERROR "✖  %s" RESET "\n", msg);
}

char* getWarn(char* msg) {

    int len = strlen(msg) + 128;
    char* buffer = malloc(len);
    if (buffer) snprintf(buffer, len, "\n  " ESTILO_WARN "⚠  %s" RESET "\n", msg);
    return buffer;

}

void imprimirWarn(char* msg) {
    printf("\n  " ESTILO_WARN "⚠  %s" RESET "\n", msg);
}

char* getInfo(char* msg) {

    int len = strlen(msg) + 128;
    char* buffer = malloc(len);
    if (buffer) snprintf(buffer, len, "\n  " C_CYAN "ℹ  %s" RESET "\n", msg);
    return buffer;

}

void imprimirInfo(char* msg) {
    printf("\n  " C_CYAN "ℹ  %s" RESET "\n", msg);
}

// TABLAS

char* getCabeceraTabla(Columna* cols, int nCols) {

    char temp[4096] = "";
    int pos = 0;

    APPEND("  " ESTILO_TITULO);

    // Línea superior
    APPEND("┌");
    for (int i = 0; i < nCols; i++) {
        for (int j = 0; j < cols[i].ancho + 2; j++) APPEND("─");
        APPEND("%s", i < nCols - 1 ? "┬" : "┐");
    }
    APPEND("\n");

    // Títulos
    APPEND("  │");
    for (int i = 0; i < nCols; i++) {
        APPEND(" %-*s │", cols[i].ancho, cols[i].titulo);
    }
    APPEND("\n");

    // Línea divisoria
    APPEND("  ├");
    for (int i = 0; i < nCols; i++) {
        for (int j = 0; j < cols[i].ancho + 2; j++) APPEND("─");
        APPEND("%s", i < nCols - 1 ? "┼" : "┤");
    }
    APPEND(RESET "\n");

    char* buffer = malloc(pos + 1);
    if (buffer) strcpy(buffer, temp);
    return buffer;

}

void imprimirCabeceraTabla(Columna* cols, int nCols) {

	char* cabeceraTabla = getCabeceraTabla(cols, nCols);
	printf("%s", cabeceraTabla);
	free(cabeceraTabla);

}

char* getFilaTabla(char** valores, Columna* cols, int nCols, int esImpar) {

    char temp[4096] = "";
    int pos = 0;

    APPEND(ESTILO_TITULO "  │" RESET);

    for (int i = 0; i < nCols; i++) {
        char* val = valores[i] ? valores[i] : "";
        int vis = anchoVisible(val);
        int pad = cols[i].ancho - vis;
        if (pad < 0) pad = 0;

        if (esImpar) APPEND(C_GRIS);

        if (cols[i].alineacion == 1) { // Derecha
            APPEND(" ");
            for (int j = 0; j < pad; j++) APPEND(" ");
            APPEND("%s" RESET ESTILO_TITULO " │" RESET, val);
        }
        else if (cols[i].alineacion == 2) { // Centro
            int padIzq = pad / 2;
            int padDer = pad - padIzq;
            APPEND(" ");
            for (int j = 0; j < padIzq; j++) APPEND(" ");
            APPEND("%s", val);
            for (int j = 0; j < padDer; j++) APPEND(" ");
            APPEND(RESET ESTILO_TITULO " │" RESET);
        }
        else { // Izquierda
            APPEND(" %s", val);
            for (int j = 0; j < pad; j++) APPEND(" ");
            APPEND(RESET ESTILO_TITULO " │" RESET);
        }
    }
    APPEND(RESET "\n");

    char* buffer = malloc(pos + 1);
    if (buffer) strcpy(buffer, temp);
    return buffer;

}

void imprimirFilaTabla(char** valores, Columna* cols, int nCols, int esImpar) {

	char* filaTabla = getFilaTabla(valores, cols, nCols, esImpar);
	printf("%s", filaTabla);
	free(filaTabla);

}

char* getPieTabla(Columna* cols, int nCols) {

    char temp[4096] = "";
    int pos = 0;

    APPEND("  " ESTILO_TITULO "└");
    for (int i = 0; i < nCols; i++) {
        for (int j = 0; j < cols[i].ancho + 2; j++) APPEND("─");
        APPEND("%s", i < nCols - 1 ? "┴" : "┘");
    }
    APPEND(RESET "\n");

    char* buffer = malloc(pos + 1);
    if (buffer) strcpy(buffer, temp);
    return buffer;

}

void imprimirPieTabla(Columna* cols, int nCols) {

	char* pieTabla = getPieTabla(cols, nCols);
	printf("%s", pieTabla);
	free(pieTabla);

}

char* getPaginacion(int pagActual, int totalPags, int totalItems) {

    char* buffer = malloc(512); // Tamaño suficiente para esta cadena
    if (buffer) {
        snprintf(buffer, 512,
           "\n  " ESTILO_HINT
           "Página %d de %d  ·  %d resultados  "
           "  [← ANTERIOR]  [SIGUIENTE →]"
           RESET "\n",
           pagActual, totalPags, totalItems);
    }
    return buffer;

}

void imprimirPaginacion(int pagActual, int totalPags, int totalItems) {

	char* paginacion = getPaginacion(pagActual, totalPags, totalItems);
	printf("%s", paginacion);
	free(paginacion);

}

// INPUT

// Devuelve 1 si s1 empieza por s2 (case insensitive)
static int empiezaPor(char* s1, char* s2) {
    return _strnicmp(s1, s2, strlen(s2)) == 0;
}

Entrada leerComando(char** opciones, int nOpciones, char* prompt) {

    Entrada entrada = {"", "", ""};
    char buffer[256] = "";
    int pos = 0;
    int tabCiclo = -1;   // índice de ciclo Tab
    char tabBase[256] = "";

    printf("  " ESTILO_CMD "%s" RESET " ", prompt);
    fflush(stdout);

    while (1) {

        int c = _getch();

        if (c == 3) {

        	strncpy(entrada.comando, "EXIT", sizeof(entrada.comando) - 1);
        	return entrada;

    	} else if (c == TECLA_ENTER) {

            buffer[pos] = '\0';
            printf("\n");
            break;

        } else if (c == TECLA_ESC || c == 127) {

            // ESC limpia el buffer
            for (int i = 0; i < pos; i++) printf("\b \b");
            memset(buffer, 0, sizeof(buffer));
            pos = 0;
            tabCiclo = -1;

        } else if (c == TECLA_TAB && nOpciones > 0) {

            // Si es el primer Tab del ciclo, guardamos la base

            if (tabCiclo == -1) {
                strncpy(tabBase, buffer, sizeof(tabBase) - 1);
            }

            // Buscamos coincidencias SIEMPRE sobre tabBase, no sobre buffer
            char* coincidencias[64];
            int nCoinc = 0;
            for (int i = 0; i < nOpciones && nCoinc < 64; i++) {

                if (empiezaPor(opciones[i], tabBase)) {
                    coincidencias[nCoinc++] = opciones[i];
                }

            }

            if (nCoinc == 0) {

                printf("\a");
                fflush(stdout);

            } else {

                tabCiclo = (tabCiclo + 1) % nCoinc;
                char* sugerencia = coincidencias[tabCiclo];

                for (int i = 0; i < pos; i++) printf("\b \b");
                printf(ESTILO_CMD "%s" RESET, sugerencia);
                strncpy(buffer, sugerencia, sizeof(buffer) - 1);
                pos = strlen(buffer);

            }

            fflush(stdout);

        } else if (c == TECLA_BACKSPACE && pos > 0) {

            pos--;
            buffer[pos] = '\0';
            printf("\b \b");
            tabCiclo = -1;
            memset(tabBase, 0, sizeof(tabBase));
            fflush(stdout);


        } else if (c == TECLA_FLECHA) {

            // Consumimos el segundo byte de la flecha

            int c2 = _getch();  // segundo byte de la flecha

            // Buscamos si ANTERIOR o SIGUIENTE están entre las opciones

            int tieneAnterior = 0, tieneSiguiente = 0;
            for (int i = 0; i < nOpciones; i++) {

                if (_stricmp(opciones[i], "ANTERIOR") == 0) tieneAnterior  = 1;
                if (_stricmp(opciones[i], "SIGUIENTE") == 0) tieneSiguiente = 1;

            }

            char* cmd = NULL;
            if ((c2 == TECLA_ABAJO || c2 == TECLA_IZQUIERDA) && tieneAnterior) cmd = "ANTERIOR";   // 75 = flecha izquierda
            if ((c2 == TECLA_ARRIBA || c2 == TECLA_DERECHA) && tieneSiguiente) cmd = "SIGUIENTE";  // 77 = flecha derecha

            if (cmd) {

                strncpy(entrada.comando, cmd, sizeof(entrada.comando) - 1);
                return entrada;

            }

        } else if (c >= 32 && c < 127 && pos < 255 && c != '|') {

            // Carácter imprimible (excepto '|')
            buffer[pos++] = (char)c;
            buffer[pos]   = '\0';
            printf(ESTILO_CMD "%c" RESET, c);
            tabCiclo = -1;
            memset(tabBase, 0, sizeof(tabBase));
            fflush(stdout);

        } else if (c == '|') {

        	printf("\a");
        	fflush(stdout);

        }

    }

    // Parseamos buffer en comando + argumentos
    // Formato esperado: "COMANDO [ARG1] [ARG2]"

    char tmp[256];
    strncpy(tmp, buffer, sizeof(tmp));

    char* token = strtok(tmp, " ");
    if (token) {

        strncpy(entrada.comando, token, sizeof(entrada.comando) - 1);
        // Convertir a mayúsculas
        for (int i = 0; entrada.comando[i]; i++) entrada.comando[i] = toupper((unsigned char)entrada.comando[i]);

    }

    token = strtok(NULL, " ");
    if (token) strncpy(entrada.arg1, token, sizeof(entrada.arg1) - 1);

    token = strtok(NULL, " ");
    if (token) strncpy(entrada.arg2, token, sizeof(entrada.arg2) - 1);

    return entrada;

}

int leerEntero(char* prompt, int min, int max) {

    int val;
    char buf[32];

    while (1) {

        printf("  " ESTILO_SUBTITULO "%s" RESET " ", prompt);
        fgets(buf, sizeof(buf), stdin);
        if (sscanf(buf, "%d", &val) == 1 && val >= min && val <= max) return val;
        imprimirError("Valor fuera de rango o inválido.");
        printf("  " ESTILO_HINT "Introduce un número entre %d y %d" RESET "\n", min, max);

    }

}

double leerDouble(char* prompt, double min, double max) {

    double val;
    char buf[32];

    while (1) {

        printf("  " ESTILO_SUBTITULO "%s" RESET " ", prompt);
        fgets(buf, sizeof(buf), stdin);
        if (sscanf(buf, "%lf", &val) == 1 && val >= min && val <= max) return val;
        imprimirError("Valor fuera de rango o inválido.");

    }

}

void leerTexto(char* prompt, char* buffer, int maxLen) {

	while (1) {

		printf("  " ESTILO_SUBTITULO "%s" RESET " ", prompt);

		// Si fgets falla o se corta la entrada, aseguramos un string vacío y salimos
		if (fgets(buffer, maxLen, stdin) == NULL) {
			buffer[0] = '\0';
			break;
		}

	    // Limpieza espacios finales
	    int len = strlen(buffer);
	    while (len > 0 && (buffer[len - 1] == '\n' || isspace((unsigned char)buffer[len - 1]))) {
	        buffer[len - 1] = '\0';
	        len--;
	    }

	    // Limpieza espacios iniciales
	    int start = 0;
	    while (buffer[start] != '\0' && isspace((unsigned char)buffer[start])) {
	        start++;
	    }

	    // Si había espacios al principio, movemos el bloque completo hacia el inicio
	    if (start > 0) {
	        memmove(buffer, &buffer[start], len - start + 1);
	    }

	    // Control de seguridad (Anti '|')
	    if (strchr(buffer, '|') != NULL) {
	        imprimirError("El caracter '|' no esta permitido por motivos de seguridad.");
	        continue; // Volvemos a pedir el input sin salir de la función
	    }

	    break; // Input 100% limpio y válido, salimos del bucle

	}

}

void leerContrasena(char* prompt, char* buffer, int maxLen) {

    while (1) {

        printf("  " ESTILO_SUBTITULO "%s" RESET " ", prompt);
        fflush(stdout); // Forzamos que se imprima el prompt antes de apagar el eco

        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        DWORD mode;
        GetConsoleMode(hStdin, &mode);
        // Apagamos el ECHO
        SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));

        fgets(buffer, maxLen, stdin);

        // Restauramos el ECHO
        SetConsoleMode(hStdin, mode);
        printf("\n"); // Metemos un salto de línea porque el Enter del usuario no se pintó

        // Quitamos el \n (y el \r si estamos en Windows) final
        int len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        if (len > 0 && buffer[len - 1] == '\r') {
            buffer[len - 1] = '\0';
        }

        // Tus validaciones de seguridad
        if (strchr(buffer, '|') != NULL) {
            imprimirError("El caracter '|' no esta permitido por motivos de seguridad.");
            continue; // Volvemos a pedir el input
        }

        break; // Input válido, salimos del bucle

    }

}

int confirmar(char* msg) {

    printf("\n  " ESTILO_WARN "%s " RESET
           C_CYAN "[S/N]" RESET " ", msg);

    while (1) {

        int c = toupper(_getch());
        if (c == 'S') { printf(ESTILO_EXITO "S\n" RESET); return 1; }
        if (c == 'N') { printf(ESTILO_ERROR "N\n" RESET); return 0; }

    }

}
