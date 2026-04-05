#include "ui_utils.h"

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

void imprimirLinea(char c, int ancho) {

    for (int i = 0; i < ancho; i++) putchar(c);
    putchar('\n');

}

void imprimirCentrado(char* texto, int ancho) {

    int len = strlen(texto);
    int pad = (ancho - len) / 2;
    int padDer = ancho - len - pad;

    for (int i = 0; i < pad; i++) putchar(' ');

    printf("%s", texto);

    for (int i = 0; i < padDer; i++) putchar(' ');

}

void imprimirCabecera(char* titulo, char* subtitulo) {

    limpiarPantalla();

    // Franja superior con logo
    printf(ESTILO_TITULO
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
    printf("  ║  ");
    printf(ESTILO_SUBTITULO "%s" RESET, titulo);

    int visibles = anchoVisible(titulo);
    for (int i = visibles ; i < 108 ; i++) putchar(' ');

    printf(ESTILO_TITULO "║\n");

    // Subtítulo si existe
    if (subtitulo && strlen(subtitulo) > 0) {

    	char subtituloStr[109];
    	strncpy(subtituloStr, subtitulo, sizeof(subtituloStr));
    	subtituloStr[108] = '\0';

        printf("  ║  ");
        printf(ESTILO_HINT "%s" RESET, subtituloStr);

        visibles = anchoVisible(subtitulo);
        for (int i = visibles ; i < 108 ; i++) putchar(' ');

        printf(ESTILO_TITULO "║\n");

    }

    printf("  ╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
    printf(RESET "\n");

}

void imprimirSeccion(char* titulo) {

    printf("\n" C_GRIS "  ── " ESTILO_SUBTITULO "%s " RESET, titulo);
    int len = strlen(titulo) + 6;

    for (int i = len; i < ANCHO_PANTALLA - 2; i++) printf(C_GRIS "─" RESET);

    printf("\n\n");

}

void imprimirDuracion(time_t segundos) {

    int dias = segundos / (24 * 3600);
    segundos %= (24 * 3600);
    int horas = segundos / 3600;
    segundos %= 3600;
    int minutos = segundos / 60;

    char display[512] = "";
    int pos = 0;

    if (dias) pos += snprintf(display + pos, sizeof(display), "%d dias, ", dias);
    if (horas) pos += snprintf(display + pos, sizeof(display), "%d horas, ", horas);
    if (minutos) pos += snprintf(display + pos, sizeof(display), "%d minutos, ", minutos);

    if (pos > 0) {
        display[pos - 2] = '\0';
    }

    printf("%s\n", display);

}

// FEEDBACK

void imprimirExito(char* msg) {
    printf("\n  " ESTILO_EXITO "✔  %s" RESET "\n", msg);
}

void imprimirError(char* msg) {
    printf("\n  " ESTILO_ERROR "✖  %s" RESET "\n", msg);
}

void imprimirWarn(char* msg) {
    printf("\n  " ESTILO_WARN "⚠  %s" RESET "\n", msg);
}

void imprimirInfo(char* msg) {
    printf("\n  " C_CYAN "ℹ  %s" RESET "\n", msg);
}

// TABLAS

void imprimirCabeceraTabla(Columna* cols, int nCols) {

    printf("  " ESTILO_TITULO);

    // Línea superior
    printf("┌");
    for (int i = 0; i < nCols; i++) {

        for (int j = 0; j < cols[i].ancho + 2; j++) printf("─");
        printf(i < nCols - 1 ? "┬" : "┐");

    }
    printf("\n");

    // Títulos
    printf("  │");
    for (int i = 0; i < nCols; i++) {

        printf(" %-*s │", cols[i].ancho, cols[i].titulo);

    }
    printf("\n");

    // Línea divisoria
    printf("  ├");
    for (int i = 0; i < nCols; i++) {

        for (int j = 0; j < cols[i].ancho + 2; j++) printf("─");
        printf(i < nCols - 1 ? "┼" : "┤");

    }
    printf(RESET "\n");

}

void imprimirFilaTabla(char** valores, Columna* cols, int nCols, int esImpar) {

    printf(ESTILO_TITULO "  │" RESET);

    for (int i = 0; i < nCols; i++) {

        char* val = valores[i] ? valores[i] : "";
        int vis = anchoVisible(val);
        int pad = cols[i].ancho - vis;  // espacios que faltan hasta el borde
        if (pad < 0) pad = 0;

        if (esImpar) printf(C_GRIS);

        if (cols[i].alineacion == 1) {

            // Derecha: pad a la izquierda
            printf(" ");
            for (int j = 0; j < pad; j++) putchar(' ');
            printf("%s" RESET ESTILO_TITULO " │" RESET, val);

        } else if (cols[i].alineacion == 2) {

            // Centro: pad repartido a ambos lados
            int padIzq = pad / 2;
            int padDer = pad - padIzq;
            printf(" ");
            for (int j = 0; j < padIzq; j++) putchar(' ');
            printf("%s", val);
            for (int j = 0; j < padDer; j++) putchar(' ');
            printf(RESET ESTILO_TITULO " │" RESET);

        } else {

            // Izquierda: pad a la derecha
            printf(" %s", val);
            for (int j = 0; j < pad; j++) putchar(' ');
            printf(RESET ESTILO_TITULO " │" RESET);

        }
    }
    printf(RESET "\n");
}

void imprimirPieTabla(Columna* cols, int nCols) {

    printf("  " ESTILO_TITULO "└");
    for (int i = 0; i < nCols; i++) {

        for (int j = 0; j < cols[i].ancho + 2; j++) printf("─");
        printf(i < nCols - 1 ? "┴" : "┘");

    }
    printf(RESET "\n");

}

void imprimirPaginacion(int pagActual, int totalPags, int totalItems) {

    printf("\n  " ESTILO_HINT
           "Página %d de %d  ·  %d resultados  "
           "  [← ANTERIOR]  [SIGUIENTE →]"
           RESET "\n",
           pagActual, totalPags, totalItems);

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

        if (c == TECLA_ENTER) {

            buffer[pos] = '\0';
            printf("\n");
            break;

        } else if (c == TECLA_ESC) {

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

        } else if (c >= 32 && c < 127 && pos < 255) {

            // Carácter imprimible
            buffer[pos++] = (char)c;
            buffer[pos]   = '\0';
            printf(ESTILO_CMD "%c" RESET, c);
            tabCiclo = -1;
            memset(tabBase, 0, sizeof(tabBase));
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
        if (sscanf(buf, "%lf", &val) == 1 && val >= min && val <= max)
            return val;
        imprimirError("Valor fuera de rango o inválido.");

    }

}

void leerTexto(char* prompt, char* buffer, int maxLen) {

    printf("  " ESTILO_SUBTITULO "%s" RESET " ", prompt);
    fgets(buffer, maxLen, stdin);

    // Quitamos el \n final
    int len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') buffer[len - 1] = '\0';

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
