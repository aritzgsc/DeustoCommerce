#ifndef UTIL_INCLUDE_FINANZAS_H_
#define UTIL_INCLUDE_FINANZAS_H_

#include "xlsxwriter.h"
#include "informes_db.h"
#include <time.h>

// PALETA DE COLORES BI

#define COLOR_BG_DASHBOARD  0xF8FAFC
#define COLOR_SLATE_DARK    0x1E293B
#define COLOR_TEAL_INCOME   0x0D9488
#define COLOR_CORAL_EXPENSE 0xE11D48
#define COLOR_WHITE         0xFFFFFF
#define COLOR_TEXT_MUTED    0x64748B

// ESTRUCTURAS AUXILIARES

typedef struct {
    lxw_format* dash_bg;
    lxw_format* dash_title;
    lxw_format* kpi_label;
    lxw_format* kpi_value_inc;
    lxw_format* kpi_value_exp;
    lxw_format* kpi_value_net;
    lxw_format* kpi_value_pct;

    lxw_format* cabecera;
    lxw_format* fila_ingreso;
    lxw_format* fila_gasto;
} Estilos;

typedef struct {
    int year;
    int month;
    double ingresos;
    double gastos;
} MesAgrupado;

// GENERACIÓN DE EXCEL CON LIBXLSXWRITER

// Genera un informe de balance en formato .xlsx.
// items: array de BalanceItem del período.
// n: número de items.
// rutaSalida: ruta del fichero .xlsx a generar.
// Devuelve 0 si OK, -1 si falla.
int generarExcelBalance(BalanceItem* items, int n, double totalIngresos, double totalGastos, char* rutaSalida);

// Genera el anuario financiero anual (una pestaña por mes + consolidado).
// ano: año a generar.
// rutaCsv: ruta al reg_financiero.csv.
// rutaSalida: ruta del .xlsx a generar.
// Devuelve 0 si OK.
int generarAnuarioFinanciero(int ano, char* rutaCsv, char* rutaSalida);

// Registra una transacción en reg_financiero.csv.
// tipo: "INGRESO" o "GASTO"
// concepto: descripción (p.ej. "Venta pedido #123")
// importe: cantidad en euros
// rutaCsv: ruta al fichero
// Devuelve 0 si OK.
int registrarTransaccion(const char* rutaCsv, const char* tipo, const char* concepto, double importe);

#endif /* UTIL_INCLUDE_FINANZAS_H_ */
