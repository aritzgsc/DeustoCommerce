#ifndef INCLUDE_UI_UTILS_H_
#define INCLUDE_UI_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>      // _getch() por Windows
#include <windows.h>    // activarColores()

// PALETA DE COLORES (Creada con Claude)

#define RESET           "\033[0m"
#define NEGRITA         "\033[1m"
#define DIM             "\033[2m"

// Colores de texto

#define C_ROJO          "\033[91m"   // errores
#define C_VERDE         "\033[92m"   // éxito / disponible
#define C_AMARILLO      "\033[93m"   // advertencias / precios
#define C_AZUL          "\033[94m"   // primario (headers, títulos)
#define C_MAGENTA       "\033[95m"   // acciones especiales
#define C_CYAN          "\033[96m"   // elementos interactivos / hints
#define C_BLANCO        "\033[97m"   // texto normal
#define C_GRIS          "\033[90m"   // texto secundario / IDs

// Colores de fondo

#define BG_AZUL         "\033[44m"
#define BG_GRIS_OSCURO  "\033[100m"

// Combinaciones semánticas listas para usar

#define ESTILO_TITULO    C_AZUL NEGRITA
#define ESTILO_SUBTITULO C_CYAN
#define ESTILO_EXITO     C_VERDE NEGRITA
#define ESTILO_ERROR     C_ROJO NEGRITA
#define ESTILO_WARN      C_AMARILLO
#define ESTILO_ID        C_GRIS
#define ESTILO_PRECIO    C_AMARILLO NEGRITA
#define ESTILO_CMD       C_CYAN NEGRITA
#define ESTILO_HINT      DIM C_BLANCO

// CONSTANTES DE LAYOUT

#define ANCHO_PANTALLA      116
#define ITEMS_POR_PAGINA    20
#define CATS_POR_PAGINA     20
#define ALMS_POR_PAGINA     20
#define ALMS_SEL_POR_PAGINA 20
#define RESENAS_POR_PAGINA  20
#define PEDIDOS_POR_PAGINA  20

// TECLAS ESPECIALES

#define TECLA_ENTER         13
#define TECLA_TAB           9
#define TECLA_BACKSPACE     8
#define TECLA_ESC           27
#define TECLA_FLECHA		224
#define TECLA_ARRIBA        72
#define TECLA_ABAJO         80
#define TECLA_IZQUIERDA     75
#define TECLA_DERECHA       77

// ESTRUCTURAS

// Columna para imprimir tablas

typedef struct {
    char* titulo;
    int ancho;
    int alineacion;    // 0 = izquierda, 1 = derecha, 2 = centro
} Columna;

// Resultado de leerComando

typedef struct {
    char comando[64];
    char arg1[128];
    char arg2[128];
} Entrada;

// FUNCIONES DE SISTEMA

void activarColores();
void limpiarPantalla();
void pausar();                          // "Pulsa ENTER para continuar..."

// FUNCIONES AUXILIARES

void quitarTildes(char* str);
void wordWrap(char* nuevo, char* texto, int padding);

// FUNCIONES DE LAYOUT

char* getLinea(char c, int ancho);  // línea de separación
char* getCentrado(char* texto, int ancho);
char* getCabecera(char* titulo, char* subtitulo); // cabecera de pantalla con logo
char* getSeccion(char* titulo);     // separador de sección interno
char* getDuracion(time_t segundos);
void imprimirLinea(char c, int ancho);  // línea de separación
void imprimirCentrado(char* texto, int ancho);
void imprimirCabecera(char* titulo, char* subtitulo); // cabecera de pantalla con logo
void imprimirSeccion(char* titulo);     // separador de sección interno
void imprimirDuracion(time_t segundos);

// FUNCIONES DE FEEDBACK

char* getExito(char* msg);
char* getError(char* msg);
char* getWarn(char* msg);
char* getInfo(char* msg);
void imprimirExito(char* msg);
void imprimirError(char* msg);
void imprimirWarn(char* msg);
void imprimirInfo(char* msg);

// FUNCIONES DE TABLAS

char* getCabeceraTabla(Columna* cols, int nCols);
char* getFilaTabla(char** valores, Columna* cols, int nCols, int esImpar);
char* getPieTabla(Columna* cols, int nCols);
char* getPaginacion(int pagActual, int totalPags, int totalItems);
void imprimirCabeceraTabla(Columna* cols, int nCols);
void imprimirFilaTabla(char** valores, Columna* cols, int nCols, int esImpar);
void imprimirPieTabla(Columna* cols, int nCols);
void imprimirPaginacion(int pagActual, int totalPags, int totalItems);

// FUNCIONES DE INPUT

// Lee un comando con autocompletado por Tab
// opciones: array de strings con los comandos posibles
// nOpciones: número de opciones
// prompt: texto del prompt
Entrada leerComando(char** opciones, int nOpciones, char* prompt);

// Inputs simples
int leerEntero(char* prompt, int min, int max);
double leerDouble(char* prompt, double min, double max);
void leerTexto(char* prompt, char* buffer, int maxLen);
void leerContrasena(char* prompt, char* buffer, int maxLen);
int confirmar(char* msg);            // devuelve 1 si S, 0 si N

#endif /* INCLUDE_UI_UTILS_H_ */
