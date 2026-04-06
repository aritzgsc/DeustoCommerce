#include "finanzas.h"
#include "xlsxwriter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// CONFIGURACIÓN DE ESTILOS (Diseñado con Gemini)

static void configurarFormatoKPI(lxw_format* f, int32_t color) {

    format_set_bold(f);
    format_set_font_size(f, 15);
    format_set_bg_color(f, COLOR_WHITE);
    format_set_align(f, LXW_ALIGN_CENTER);
    format_set_align(f, LXW_ALIGN_VERTICAL_CENTER);
    format_set_num_format(f, "#,##0.00\" €\"");
    format_set_font_name(f, "Segoe UI");
    format_set_bottom(f, LXW_BORDER_THIN);
    format_set_left(f, LXW_BORDER_THIN);
    format_set_right(f, LXW_BORDER_THIN);
    format_set_font_color(f, color);

}

static Estilos crearEstilos(lxw_workbook* wb) {
    Estilos e = {0};

    e.dash_bg = workbook_add_format(wb);
    format_set_bg_color(e.dash_bg, COLOR_BG_DASHBOARD);

    e.dash_title = workbook_add_format(wb);
    format_set_bold(e.dash_title);
    format_set_font_size(e.dash_title, 18);
    format_set_font_color(e.dash_title, COLOR_WHITE);
    format_set_bg_color(e.dash_title, COLOR_SLATE_DARK);
    format_set_align(e.dash_title, LXW_ALIGN_VERTICAL_CENTER);
    format_set_align(e.dash_title, LXW_ALIGN_CENTER);
    format_set_font_name(e.dash_title, "Segoe UI");

    e.kpi_label = workbook_add_format(wb);
    format_set_bold(e.kpi_label);
    format_set_font_size(e.kpi_label, 9);
    format_set_font_color(e.kpi_label, COLOR_TEXT_MUTED);
    format_set_bg_color(e.kpi_label, COLOR_WHITE);
    format_set_align(e.kpi_label, LXW_ALIGN_CENTER);
    format_set_align(e.kpi_label, LXW_ALIGN_VERTICAL_CENTER);
    format_set_font_name(e.kpi_label, "Segoe UI");
    format_set_top(e.kpi_label, LXW_BORDER_THIN);
    format_set_left(e.kpi_label, LXW_BORDER_THIN);
    format_set_right(e.kpi_label, LXW_BORDER_THIN);

    e.kpi_value_inc = workbook_add_format(wb);
    configurarFormatoKPI(e.kpi_value_inc, COLOR_TEAL_INCOME);

    e.kpi_value_exp = workbook_add_format(wb);
    configurarFormatoKPI(e.kpi_value_exp, COLOR_CORAL_EXPENSE);

    e.kpi_value_net = workbook_add_format(wb);
    configurarFormatoKPI(e.kpi_value_net, COLOR_SLATE_DARK);

    e.kpi_value_pct = workbook_add_format(wb);
    configurarFormatoKPI(e.kpi_value_pct, COLOR_SLATE_DARK);
    format_set_num_format(e.kpi_value_pct, "0.00\"%\"");

    e.cabecera = workbook_add_format(wb);
    format_set_bold(e.cabecera);
    format_set_bg_color(e.cabecera, COLOR_SLATE_DARK);
    format_set_font_color(e.cabecera, COLOR_WHITE);
    format_set_font_name(e.cabecera, "Segoe UI");
    format_set_font_size(e.cabecera, 10);

    e.fila_ingreso = workbook_add_format(wb);
    format_set_font_name(e.fila_ingreso, "Segoe UI");
    format_set_font_size(e.fila_ingreso, 9);
    format_set_font_color(e.fila_ingreso, COLOR_TEAL_INCOME);
    format_set_bottom(e.fila_ingreso, LXW_BORDER_HAIR);

    e.fila_gasto = workbook_add_format(wb);
    format_set_font_name(e.fila_gasto, "Segoe UI");
    format_set_font_size(e.fila_gasto, 9);
    format_set_font_color(e.fila_gasto, COLOR_CORAL_EXPENSE);
    format_set_bottom(e.fila_gasto, LXW_BORDER_HAIR);

    return e;

}

// FUNCIONES AUXILIARES

static void escribirTransacciones(lxw_worksheet* ws, Estilos* e, BalanceItem* items, int n, int filaInicio) {

	for (int i = 0; i < n; i++) {
        int fila = filaInicio + i;
        int esIng = (strncmp(items[i].tipo, "INGRESO", 7) == 0);
        lxw_format* f = esIng ? e->fila_ingreso : e->fila_gasto;

        worksheet_write_string(ws, fila, 0, items[i].fecha, f);
        worksheet_write_string(ws, fila, 1, items[i].tipo, f);
        worksheet_write_string(ws, fila, 2, items[i].concepto, f);
        worksheet_write_number(ws, fila, 3, items[i].importe, f);
    }

}

static int obtenerDiaSemana(const char* fecha) {

    int y, m, d;
    if (sscanf(fecha, "%d-%d-%d", &y, &m, &d) != 3) return 0;

    struct tm time_in = {0};
    time_in.tm_year = y - 1900;
    time_in.tm_mon  = m - 1;
    time_in.tm_mday = d;
    mktime(&time_in);

    int wday = time_in.tm_wday;
    return (wday == 0) ? 6 : wday - 1;

}

// DIBUJAR DASHBOARD Y HOJA OCULTA

static void dibujarDashboardBI(lxw_workbook* wb, lxw_worksheet* ws, Estilos* e,
                               double totIng, double totGas,
                               MesAgrupado* arrayMeses, int numMeses,
                               double* ingresosDia) {

    // CREAR HOJA DE DATOS OCULTA
    lxw_worksheet* wsData = workbook_add_worksheet(wb, "SysData");
    worksheet_hide(wsData);

    char* nombresMes[] = {"", "Ene", "Feb", "Mar", "Abr", "May", "Jun", "Jul", "Ago", "Sep", "Oct", "Nov", "Dic"};

    for(int i = 0; i < numMeses; i++) {
        char label[32];
        sprintf(label, "%s '%02d", nombresMes[arrayMeses[i].month], arrayMeses[i].year % 100);
        worksheet_write_string(wsData, i, 0, label, NULL);
        worksheet_write_number(wsData, i, 1, arrayMeses[i].ingresos, NULL);
        worksheet_write_number(wsData, i, 2, arrayMeses[i].gastos, NULL);
    }

    char* diasSemana[] = {"Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado", "Domingo"};
    for (int d = 0; d < 7; d++) {
        worksheet_write_string(wsData, d, 4, diasSemana[d], NULL);
        worksheet_write_number(wsData, d, 5, ingresosDia[d], NULL);
    }

    worksheet_write_string(wsData, 0, 7, "Ingresos", NULL);
    worksheet_write_number(wsData, 0, 8, totIng, NULL);
    worksheet_write_string(wsData, 1, 7, "Gastos", NULL);
    worksheet_write_number(wsData, 1, 8, totGas, NULL);

    // DASHBOARD BI
    worksheet_gridlines(ws, 2);
    worksheet_set_column(ws, 0, 0, 3, e->dash_bg);
    for(int i = 1; i <= 12; i++) {
        worksheet_set_column(ws, i, i, 12.5, e->dash_bg);
    }
    worksheet_set_column(ws, 13, 20, 12.5, e->dash_bg);

    worksheet_merge_range(ws, 1, 1, 1, 12, "DASHBOARD FINANCIERO EJECUTIVO", e->dash_title);

    double beneficio = totIng - totGas;
    double margen = totIng > 0 ? (beneficio / totIng) * 100.0 : 0;

    worksheet_merge_range(ws, 3, 1, 3, 3, "INGRESOS TOTALES", e->kpi_label);
    worksheet_merge_range(ws, 4, 1, 5, 3, "", e->kpi_value_inc);
    worksheet_write_number(ws, 4, 1, totIng, e->kpi_value_inc);

    worksheet_merge_range(ws, 3, 4, 3, 6, "GASTOS TOTALES", e->kpi_label);
    worksheet_merge_range(ws, 4, 4, 5, 6, "", e->kpi_value_exp);
    worksheet_write_number(ws, 4, 4, totGas, e->kpi_value_exp);

    worksheet_merge_range(ws, 3, 7, 3, 9, "BENEFICIO NETO", e->kpi_label);
    worksheet_merge_range(ws, 4, 7, 5, 9, "", e->kpi_value_net);
    worksheet_write_number(ws, 4, 7, beneficio, e->kpi_value_net);

    worksheet_merge_range(ws, 3, 10, 3, 12, "MARGEN DE BENEFICIO", e->kpi_label);
    worksheet_merge_range(ws, 4, 10, 5, 12, "", e->kpi_value_pct);
    worksheet_write_number(ws, 4, 10, margen, e->kpi_value_pct);

    lxw_chart_options opt_ancha = {.x_scale = 2.325, .y_scale = 1.35};
    lxw_chart_options opt_mitad_izq = {.x_scale = 1.13, .y_scale = 1.20};
    lxw_chart_options opt_mitad_der = {.x_scale = 1.13, .y_scale = 1.20, .x_offset = 15};

    // GRÁFICO 1
    if (numMeses > 1) {
        lxw_chart* cLinea = workbook_add_chart(wb, LXW_CHART_LINE);

        lxw_chart_series* sI = chart_add_series(cLinea, NULL, NULL);
        chart_series_set_name(sI, "Ingresos");
        chart_series_set_categories(sI, "SysData", 0, 0, numMeses - 1, 0);
        chart_series_set_values(sI, "SysData", 0, 1, numMeses - 1, 1);
        chart_series_set_line(sI, &(lxw_chart_line){.color = COLOR_TEAL_INCOME, .width = 2.5});
        chart_series_set_smooth(sI, 1);

        lxw_chart_series* sG = chart_add_series(cLinea, NULL, NULL);
        chart_series_set_name(sG, "Gastos");
        chart_series_set_categories(sG, "SysData", 0, 0, numMeses - 1, 0);
        chart_series_set_values(sG, "SysData", 0, 2, numMeses - 1, 2);
        chart_series_set_line(sG, &(lxw_chart_line){.color = COLOR_CORAL_EXPENSE, .width = 2.5});
        chart_series_set_smooth(sG, 1);

        chart_title_set_name(cLinea, "Evolución de Tendencia (Meses Activos)");
        chart_legend_set_position(cLinea, LXW_CHART_LEGEND_BOTTOM);
        chart_axis_set_name(cLinea->x_axis, "Período");

        worksheet_insert_chart_opt(ws, 7, 1, cLinea, &opt_ancha);

    } else if (numMeses == 1) {

    	lxw_chart* cCol = workbook_add_chart(wb, LXW_CHART_COLUMN);

        lxw_chart_series* sI = chart_add_series(cCol, NULL, NULL);
        chart_series_set_name(sI, "Ingresos");
        chart_series_set_categories(sI, "SysData", 0, 0, 0, 0);
        chart_series_set_values(sI, "SysData", 0, 1, 0, 1);
        chart_series_set_fill(sI, &(lxw_chart_fill){.color = COLOR_TEAL_INCOME});

        lxw_chart_series* sG = chart_add_series(cCol, NULL, NULL);
        chart_series_set_name(sG, "Gastos");
        chart_series_set_categories(sG, "SysData", 0, 0, 0, 0);
        chart_series_set_values(sG, "SysData", 0, 2, 0, 2);
        chart_series_set_fill(sG, &(lxw_chart_fill){.color = COLOR_CORAL_EXPENSE});

        chart_title_set_name(cCol, "Evolución de Tendencia (Mes Único)");
        chart_legend_set_position(cCol, LXW_CHART_LEGEND_BOTTOM);
        chart_axis_set_name(cCol->x_axis, "Período");

        worksheet_insert_chart_opt(ws, 7, 1, cCol, &opt_ancha);

    }

    // GRÁFICO 2
    lxw_chart* cDias = workbook_add_chart(wb, LXW_CHART_COLUMN);
    lxw_chart_series* sD = chart_add_series(cDias, NULL, NULL);
    chart_series_set_categories(sD, "SysData", 0, 4, 6, 4);
    chart_series_set_values(sD, "SysData", 0, 5, 6, 5);
    chart_series_set_fill(sD, &(lxw_chart_fill){.color = COLOR_SLATE_DARK});

    chart_title_set_name(cDias, "Ingresos por Día Semanal");
    chart_legend_set_position(cDias, LXW_CHART_LEGEND_NONE);

    worksheet_insert_chart_opt(ws, 28, 1, cDias, &opt_mitad_izq);

    // GRÁFICO 3
    lxw_chart* cDough = workbook_add_chart(wb, LXW_CHART_DOUGHNUT);
    lxw_chart_series* sP = chart_add_series(cDough, NULL, NULL);
    chart_series_set_categories(sP, "SysData", 0, 7, 1, 7);
    chart_series_set_values(sP, "SysData", 0, 8, 1, 8);

    lxw_chart_fill fill_inc = {.color = COLOR_TEAL_INCOME};
    lxw_chart_fill fill_exp = {.color = COLOR_CORAL_EXPENSE};
    lxw_chart_point p_inc = {.fill = &fill_inc};
    lxw_chart_point p_exp = {.fill = &fill_exp};
    lxw_chart_point* p_colors[] = {&p_inc, &p_exp, NULL};

    chart_series_set_points(sP, p_colors);
    chart_title_set_name(cDough, "Proporción de Capital");

    worksheet_insert_chart_opt(ws, 28, 7, cDough, &opt_mitad_der);

}

// PROCESAMIENTO DINÁMICO DE MESES

static void agruparTransaccionesPorMes(BalanceItem* items, int n, MesAgrupado* arrayMeses, int* numMeses, double* ingresosDia) {
    *numMeses = 0;

    for (int i = 0; i < n; i++) {
        int y = 0, m = 0, d = 0;
        sscanf(items[i].fecha, "%d-%d-%d", &y, &m, &d);
        if(y == 0 || m == 0 || m > 12) continue;

        int idx = -1;
        for(int j = 0; j < *numMeses; j++) {
            if(arrayMeses[j].year == y && arrayMeses[j].month == m) {
                idx = j;
                break;
            }
        }

        if(idx == -1) {
            arrayMeses[*numMeses].year = y;
            arrayMeses[*numMeses].month = m;
            arrayMeses[*numMeses].ingresos = 0;
            arrayMeses[*numMeses].gastos = 0;
            idx = *numMeses;
            (*numMeses)++;
        }

        if (strncmp(items[i].tipo, "INGRESO", 7) == 0) {
            arrayMeses[idx].ingresos += items[i].importe;
            int dia = obtenerDiaSemana(items[i].fecha);
            ingresosDia[dia] += items[i].importe;
        } else {
            arrayMeses[idx].gastos += items[i].importe;
        }
    }

    for(int i = 0; i < *numMeses - 1; i++) {
        for(int j = i + 1; j < *numMeses; j++) {
            if(arrayMeses[i].year > arrayMeses[j].year ||
              (arrayMeses[i].year == arrayMeses[j].year && arrayMeses[i].month > arrayMeses[j].month)) {
                MesAgrupado temp = arrayMeses[i];
                arrayMeses[i] = arrayMeses[j];
                arrayMeses[j] = temp;
            }
        }
    }
}

// GENERAR EXCEL BALANCE

int generarExcelBalance(BalanceItem* items, int n, double totalIngresos, double totalGastos, char* rutaSalida) {

	if (!items || n == 0 || !rutaSalida) return -1;

    lxw_workbook* wb = workbook_new(rutaSalida);
    Estilos e = crearEstilos(wb);

    MesAgrupado arrayMeses[120];
    int numMeses = 0;
    double ingresosDia[7] = {0};

    agruparTransaccionesPorMes(items, n, arrayMeses, &numMeses, ingresosDia);

    // Creamos explícitamente el Dashboard primero
    lxw_worksheet* wsDash = workbook_add_worksheet(wb, "Dashboard BI");
    dibujarDashboardBI(wb, wsDash, &e, totalIngresos, totalGastos, arrayMeses, numMeses, ingresosDia);

    lxw_worksheet* wsDet = workbook_add_worksheet(wb, "Data Cruda");
    worksheet_set_column(wsDet, 0, 0, 15, NULL);
    worksheet_set_column(wsDet, 1, 1, 12, NULL);
    worksheet_set_column(wsDet, 2, 2, 45, NULL);
    worksheet_set_column(wsDet, 3, 3, 16, NULL);

    worksheet_write_string(wsDet, 0, 0, "FECHA",    e.cabecera);
    worksheet_write_string(wsDet, 0, 1, "TIPO",     e.cabecera);
    worksheet_write_string(wsDet, 0, 2, "CONCEPTO", e.cabecera);
    worksheet_write_string(wsDet, 0, 3, "IMPORTE",  e.cabecera);

    escribirTransacciones(wsDet, &e, items, n, 1);
    worksheet_autofilter(wsDet, 0, 0, n, 3);
    worksheet_freeze_panes(wsDet, 1, 0);

    return workbook_close(wb) == LXW_NO_ERROR ? 0 : -1;
}

// GENERAR ANUARIO FINANCIERO

int generarAnuarioFinanciero(int ano, char* rutaCsv, char* rutaSalida) {
    if (!rutaCsv || !rutaSalida) return -1;

    lxw_workbook* wb = workbook_new(rutaSalida);
    Estilos e = crearEstilos(wb);

    char* meses[] = {
        "Enero","Febrero","Marzo","Abril","Mayo","Junio",
        "Julio","Agosto","Septiembre","Octubre","Noviembre","Diciembre"
    };

    MesAgrupado arrayMeses[12];
    int numMeses = 0;
    double ingresosDia[7]  = {0};
    double totIngAnual = 0, totGasAnual = 0;

    // Creamos la pestaña Dashboard como la primera, para forzar el foco inicial de forma segura
    lxw_worksheet* wsDash = workbook_add_worksheet(wb, "Dashboard BI");
    worksheet_activate(wsDash);

    for (int mes = 0; mes < 12; mes++) {
        struct tm tIni = {0}, tFin = {0};
        tIni.tm_year = ano - 1900; tIni.tm_mon = mes;     tIni.tm_mday = 1;
        tFin.tm_year = ano - 1900; tFin.tm_mon = mes + 1; tFin.tm_mday = 0;
        time_t fIni = mktime(&tIni);
        time_t fFin = mktime(&tFin);

        int n = 0;
        BalanceItem* items = getBalance(rutaCsv, fIni, fFin, &n);

        if (!items || n == 0) {
            if(items) free(items);
            continue;
        }

        lxw_worksheet* ws = workbook_add_worksheet(wb, meses[mes]);
        worksheet_set_column(ws, 0, 0, 15, NULL);
        worksheet_set_column(ws, 1, 1, 12, NULL);
        worksheet_set_column(ws, 2, 2, 45, NULL);
        worksheet_set_column(ws, 3, 3, 16, NULL);

        worksheet_write_string(ws, 0, 0, "FECHA",    e.cabecera);
        worksheet_write_string(ws, 0, 1, "TIPO",     e.cabecera);
        worksheet_write_string(ws, 0, 2, "CONCEPTO", e.cabecera);
        worksheet_write_string(ws, 0, 3, "IMPORTE",  e.cabecera);

        double totIngMes = 0, totGasMes = 0;
        escribirTransacciones(ws, &e, items, n, 1);

        // Lo calculo inline para que no falle ninguna dependencia externa
        for(int i = 0; i < n; i++) {
            if(strncmp(items[i].tipo, "INGRESO", 7) == 0) {
                totIngMes += items[i].importe;
                int dia = obtenerDiaSemana(items[i].fecha);
                ingresosDia[dia] += items[i].importe;
            } else {
                totGasMes += items[i].importe;
            }
        }
        free(items);

        arrayMeses[numMeses].year = ano;
        arrayMeses[numMeses].month = mes + 1;
        arrayMeses[numMeses].ingresos = totIngMes;
        arrayMeses[numMeses].gastos = totGasMes;
        numMeses++;

        totIngAnual += totIngMes;
        totGasAnual += totGasMes;

        worksheet_autofilter(ws, 0, 0, n, 3);
        worksheet_freeze_panes(ws, 1, 0);
    }

    // Le pasamos el wsDash correcto
    dibujarDashboardBI(wb, wsDash, &e, totIngAnual, totGasAnual, arrayMeses, numMeses, ingresosDia);

    return workbook_close(wb) == LXW_NO_ERROR ? 0 : -1;
}

// REGISTRAR TRANSACCIÓN

int registrarTransaccion(const char* rutaCsv, const char* tipo, const char* concepto, double importe) {

    if (!rutaCsv || !tipo || !concepto) return -1;

    FILE* f = fopen(rutaCsv, "a");
    if (!f) return -1;

    time_t ahora = time(NULL);
    struct tm* tm = localtime(&ahora);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);

    fprintf(f, "%s;%s;%s;%.2f\n", timestamp, tipo, concepto, importe);
    fclose(f);
    return 0;

}
