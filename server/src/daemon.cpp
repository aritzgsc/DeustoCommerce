#include "server.h"
#include "daemon.h"
#include "mail_utils.h"
#include <fstream>
#include <time.h>
#include <stdio.h>

extern "C" {
    #include "almacenes_db.h"
    #include "usuario_db.h"
    #include "finanzas.h"
    #include "logistica.h"
    #include "log.h"
    #include "config.h"
}

using namespace std;

// DAEMON

Daemon::Daemon() {

    char path[256];
    configGet(CONFIG_PATH, "ACCIONES_PENDIENTES_PATH", path, sizeof(path));

    char dbPath[256];
    configGet(CONFIG_PATH, "DB_PATH", dbPath, sizeof(dbPath));

    ejecutando = false;
    ultimoDiaRestock = -1;
    ultimoMesBalance = -1;
    db = nullptr;

    if (sqlite3_open(dbPath, &db) != SQLITE_OK) {
        LOG_ERROR("No se pudo conectar a la Base de Datos: %s", sqlite3_errmsg(db));
        db = nullptr;
    } else {
        LOG_INFO("Conexión con BD establecida con éxito");
    }
}

Daemon::~Daemon() {

	detener();

    if (db != NULL) {
        sqlite3_close(db);
        LOG_INFO("Conexión con SQLite cerrada");
    }

}

bool Daemon::iniciar() {

    if (ejecutando) return false;
    if (!db) {
        LOG_WARN("No se puede iniciar sin conexión activa a la BD");
        return false;
    }

    ejecutando = true;

    hiloCron = thread(&Daemon::rutinaTareasPeriodicas, this);
    hiloCola = thread(&Daemon::rutinaGestorCola, this);

    LOG_INFO("Demonio iniciado en segundo plano");
    return true;

}

void Daemon::detener() {

	if (!ejecutando) return;
    ejecutando = false;

    if (hiloCron.joinable()) hiloCron.join();
    if (hiloCola.joinable()) hiloCola.join();

    LOG_INFO("Hilos detenidos limpiamente");

}

// HILO 1: TAREAS PERIÓDICAS

extern time_t offsetTiempoSegundos;

void Daemon::rutinaTareasPeriodicas() {

    while (ejecutando) {

        time_t tVirtual = time(NULL) + offsetTiempoSegundos;
        struct tm now = *localtime(&tVirtual);

        // RESTOCK SEMANAL (Lunes a las 00:00)
        if (now.tm_wday == 1 && now.tm_hour == 0) {
            if (ultimoDiaRestock != now.tm_yday) {

                int numAlmacenes = 0;
                Almacen* almacenes = getAlmacenes(db, &numAlmacenes);

                if (almacenes != nullptr && numAlmacenes > 0) {
                    for (int i = 0; i < numAlmacenes; i++) {

                        int ocupacion = getOcupacionAlmacen(db, almacenes[i].id);
                        double limite20 = almacenes[i].capacidad * 0.2;

                        // Si la ocupación actual es inferior al 20% de su capacidad
                        if (ocupacion < limite20) {

                            double precioEst = 0.0;
                            time_t duracionEst = 0;

                            // Calculamos cuánto va a tardar el restock
                            calcularCosteRestock(db, almacenes[i].id, &precioEst, &duracionEst);
                            time_t timestampLlegada = tVirtual + duracionEst;

                            double costeReal = 0.0;
                            int unidadesAnadidas = restock(db, almacenes[i].id, &costeReal, timestampLlegada);

                            if (unidadesAnadidas >= 0) {

                                char msg[64];
                                snprintf(msg, sizeof(msg), "Restock completado: %d unidades añadidas.", unidadesAnadidas);
                                LOG_INFO("Restock completado en en #%d — %s, %s || %d unidades añadidas || %.2f €", almacenes[i].id, almacenes[i].ubicacion.ciudad.nombre, almacenes[i].ubicacion.ciudad.pais.nombre, unidadesAnadidas, costeReal);

                                char regFinancieroPath[256];
                                configGet(CONFIG_PATH, "REG_FINANCIERO_PATH", regFinancieroPath, sizeof(regFinancieroPath));

                                char concepto[512];
                                snprintf(concepto, sizeof(concepto), "Restock completado: #%d — %s, %s || %d unidades añadidas", almacenes[i].id, almacenes[i].ubicacion.ciudad.nombre, almacenes[i].ubicacion.ciudad.pais.nombre, unidadesAnadidas);
                                registrarTransaccion(regFinancieroPath, "GASTO", "RESTOCK_ALMACEN", concepto, costeReal);

                            }

                        }

                    }

                    // Liberamos los bloques de memoria asignados por strdup en almacenDB
                    // No usamos liberarAlmacen() aquí porque intentaría hacer free() de elementos de un array contiguo
                    for (int i = 0; i < numAlmacenes; i++) {
                        free(almacenes[i].nombre);
                        free(almacenes[i].ubicacion.direccion);
                        free(almacenes[i].ubicacion.ciudad.nombre);
                        free(almacenes[i].ubicacion.ciudad.pais.id);
                        free(almacenes[i].ubicacion.ciudad.pais.nombre);
                    }
                    free(almacenes);
                }

                ultimoDiaRestock = now.tm_yday;

            }

        }

        // BALANCE MENSUAL / ANUARIO (Día 1 de cada mes a las 00:00)
        if (now.tm_mday == 1 && now.tm_hour == 0) {

            if (ultimoMesBalance != now.tm_mon) {

                int ano = now.tm_year + 1900;
                int mesMax = now.tm_mon;

                // Si es 1 de Enero, cerramos el histórico del año anterior completo (12 meses)
                if (mesMax == 0) {
                    ano--;
                    mesMax = 12;
                }

                char rutaCsv[256];
                configGet(CONFIG_PATH, "REG_FINANCIERO_PATH", rutaCsv, sizeof(rutaCsv));

                char rutaReports[256];
                configGet(CONFIG_PATH, "REPORTS_PATH", rutaReports, sizeof(rutaReports));

                char rutaSalida[256];
                snprintf(rutaSalida, sizeof(rutaSalida), (string(rutaReports) + "Anuario_Financiero_%d.xlsx").c_str(), ano);

                if (generarAnuarioFinanciero(ano, mesMax, rutaCsv, rutaSalida) == 0) {
                    LOG_INFO("Anuario financiero generado con éxito para el año %d (meses consolidados: %d) en %s", ano, mesMax, rutaSalida);
                } else {
                    LOG_ERROR("Error crítico al intentar generar el anuario financiero del año %d", ano);
                }

                ultimoMesBalance = now.tm_mon;

            }

        }

        // Pausa de control para el hilo de ejecución
        for (int i = 0; i < 60 && ejecutando; ++i) {
            Sleep(1000);
        }

    }

}

// HILO 2: GESTOR DE COLA DE TAREAS

void Daemon::rutinaGestorCola() {

	char csvPathC[256];
	configGet(CONFIG_PATH, "ACCIONES_PENDIENTES_PATH", csvPathC, sizeof(csvPathC));

	string csvPath = string(csvPathC);

	while (ejecutando) {

        HANDLE hMutex = CreateMutexA(NULL, FALSE, "Global\\DeustoCommerce_CSV_Mutex");

        if (hMutex != NULL) {

            WaitForSingleObject(hMutex, INFINITE);

            ifstream fileIn(csvPath);

            if (fileIn.is_open()) {

                string tmpPath = csvPath + ".tmp";
                ofstream fileOut(tmpPath);
                string line;
                time_t tVirtual = time(NULL) + offsetTiempoSegundos;

                int nLinea = 0;

                while (getline(fileIn, line)) {

                	nLinea++;
                	if (nLinea == 1) { fileOut << line << "\n"; continue; }
                    if (line.empty()) continue;

                    size_t p1 = line.find(';');
                    if (p1 == string::npos) { fileOut << line << "\n"; continue; }

                    size_t p2 = line.find(';', p1 + 1);
                    if (p2 == string::npos) { fileOut << line << "\n"; continue; }

                    string t_str = line.substr(0, p1);
                    string tipo = line.substr(p1 + 1, p2 - p1 - 1);
                    string restoDatos = line.substr(p2 + 1);

                    time_t timestampEjecucion = stoll(t_str);

                    if (tVirtual >= timestampEjecucion) {

                        bool mantenerLinea = false;

                        if (tipo == "ADD_STOCK") {

                            size_t d1 = restoDatos.find(',');
                            size_t d2 = restoDatos.find(',', d1 + 1);
                            size_t d3 = restoDatos.find(',', d2 + 1);

                            if (d1 != string::npos && d2 != string::npos) {

                                int idAlm = stoi(restoDatos.substr(0, d1));
                                int idPr = stoi(restoDatos.substr(d1 + 1, d2 - d1 - 1));
                                string variante = restoDatos.substr(d2 + 1, d3 - d2 - 1);
                                int cant = stoi(restoDatos.substr(d3 + 1));

                                actualizarEstadoStock(db, idAlm, idPr, variante.c_str(), cant);

                            }

                        } else if (tipo == "COMPRA_PEDIDO") {

                            size_t d1 = restoDatos.find(',');
                            size_t d2 = restoDatos.find(',', d1 + 1);
                            size_t d3 = restoDatos.find(',', d2 + 1);
                            size_t d4 = restoDatos.find(',', d3 + 1);

                            if (d1 != string::npos) {

                                int idPed = stoi(restoDatos.substr(0, d1));
                                int nuevoIdEstado = stoi(restoDatos.substr(d1 + 1, d2 - d1 - 1));
                                string correo = restoDatos.substr(d2 + 1, d3 - d2 - 1);
                                string nombre = restoDatos.substr(d3 + 1, d4 - d3 - 1);
                                string apellido = restoDatos.substr(d4 + 1);

                                int res = actualizarEstadoPedido(db, idPed, nuevoIdEstado);

                                if (res == 0 && nuevoIdEstado == 3) {

                                	Pedido* pedido = getPedidoPorId(db, idPed);

                                	if (enviarMailPedidoEntregado(correo.c_str(), nombre.c_str(), apellido.c_str(), *pedido)) {

                                		LOG_INFO("Correo de confirmación de entrega #%d enviado a %s", idPed, correo.c_str());

                                	} else {

                                		LOG_ERROR("Error al enviar el correo de confirmación de entrega #%d a %s", idPed, correo.c_str());

                                	}

                                }

                            }
                        }

                        if (mantenerLinea) {

                            fileOut << line << "\n";

                        }

                    } else {

                        fileOut << line << "\n";

                    }

                }

                fileIn.close();
                fileOut.close();

                remove(csvPath.c_str());
                rename(tmpPath.c_str(), csvPath.c_str());

            }

            ReleaseMutex(hMutex);
            CloseHandle(hMutex);

        }

        for (int i = 0; i < 60 && ejecutando; ++i) {
            Sleep(1000);
        }

    }

}
