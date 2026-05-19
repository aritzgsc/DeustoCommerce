#include "server.h"
#include "server_ui.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>

extern "C" {
	#include "utils_ui.h"
	#include "config.h"
}

using namespace std;

// Función y diseño hechos con ayuda de Gémini

extern time_t offsetTiempoSegundos;

string inputBuffer = "";
string tabPrefix = "";
int tabIndex = -1;
bool lastWasTab = false;

// Lista maestra de comandos (asegúrate de que los espacios/guiones coinciden con tus IFs)
static const string cmdsDashboard[] = {

		"EXIT", "TIME_SET ", "TIME_SYNC", "PROBAR_RESTOCK", "PROBAR_INFORME", "AVANZAR_DIAS "

};
#define N_COMANDOS_DASHBOARD 6

void ejecutarDashboardRealTime(Server& server) {

    activarColores();
    string inputBuffer = "";
    limpiarPantalla();

    while (server.isEnEjecucion()) {

    	ostringstream frame;

        frame << "\033[?25l\033[H";

        time_t ahoraVirtual = time(nullptr) + offsetTiempoSegundos;
        struct tm* tmActual = localtime(&ahoraVirtual);
        char fechaStr[32];
        strftime(fechaStr, sizeof(fechaStr), "%d/%m/%Y  %H:%M:%S", tmActual);

        frame << "\n" ESTILO_TITULO
               "  ╔══════════════════════════════════════════════════════════════════════════════════════════════════════════════╗\n"
               "  ║                                                                                                              ║\n"
               "  ║" RESET C_BLANCO NEGRITA
                  "                                     DEUSTO" RESET C_CYAN NEGRITA
    			                                             "COMMERCE" RESET C_BLANCO NEGRITA
                                                                     "  —  Dashboard Maestro                                     " RESET ESTILO_TITULO
    								                                                                                            "║\n"
               "  ║                                                                                                              ║\n"
               "  ╚══════════════════════════════════════════════════════════════════════════════════════════════════════════════╝\n"
               RESET;

        char* sec1 = getSeccion((char*)"INFORMACION DE RED");
        frame << sec1;
        free(sec1);

        frame << "  " << C_GRIS << left << setw(32) << "Fecha y hora virtual:" << RESET << C_AMARILLO << "  " << left << setw(30) << fechaStr << RESET << "\n";

        string txtPuerto = "ESCUCHANDO EN PUERTO " + to_string(server.getPuerto());
        frame << "  " << C_GRIS << left << setw(32) << "Estado del servicio:" << RESET << C_VERDE << "  " << left << setw(30) << txtPuerto << RESET << "\n";

        char* sec2 = getSeccion((char*)"METRICAS EN TIEMPO REAL");
        frame << sec2;
        free(sec2);

        int clientes = server.getClientesConectados();
        frame << "  " << C_GRIS << left << setw(32) << "Clientes conectados:" << RESET;
        if (clientes > 0) {
            string txtClientes = to_string(clientes) + " sesion(es) activa(s)";
            frame << C_CYAN << "  " << left << setw(30) << txtClientes << RESET << "\n";
        } else {
            frame << C_GRIS << "  " << left << setw(30) << "0 (Esperando conexiones)" << RESET << "\n";
        }

        string txtPeticiones = to_string(server.getPeticionesTotales()) + " procesadas";
        frame << "  " << C_GRIS << left << setw(32) << "Peticiones totales:" << RESET << C_BLANCO << "  " << left << setw(30) << txtPeticiones << RESET << "\n";

        frame << "  " << C_GRIS << left << setw(32) << "Tiempo de actividad:" << RESET << ESTILO_HINT << "  ";

        char* dur = getDuracion(server.getTiempoActivo());
        string durStr = "";
        if (dur) {
            durStr = dur;
            if (!durStr.empty() && durStr.back() == '\n') durStr.pop_back();
            free(dur);
        }
        frame << left << setw(30) << durStr << RESET << "\n";

        char* secTesting = getSeccion((char*)"HERRAMIENTAS DE TESTING");
        frame << secTesting;
        free(secTesting);

        frame << "  " << ESTILO_CMD << "PROBAR_RESTOCK                    " << RESET << "Viajar de inmediato al proximo domingo (23:59:50) para simular Restock\n";
        frame << "  " << ESTILO_CMD << "PROBAR_INFORME                    " << RESET << "Viajar de inmediato al cierre del mes actual (23:59:50) para simular Anuario\n";
        frame << "  " << ESTILO_CMD << "AVANZAR_DIAS [DIAS]               " << RESET << "Adelantar el reloj del sistema un numero de dias\n";

        char* sec3 = getSeccion((char*)"CONTROL DE SERVIDOR");
        frame << sec3;
        free(sec3);

        frame << "  " << ESTILO_CMD << "TIME_SET [DD-MM-AAAA] [HH:MM:SS]  " << RESET << "Viajar de forma manual a cualquier fecha/hora\n";
        frame << "  " << ESTILO_CMD << "TIME_SYNC                         " << RESET << "Restablecer la sincronizacion automatica de Windows\n";
        frame << "  " << ESTILO_CMD << "EXIT                              " << RESET << "Apagar el servidor de forma segura\n\n";

        frame << ESTILO_CMD "\033[2K\r  > " << inputBuffer << "\033[?25h";

        cout << frame.str();
        cout.flush();

        for (int i = 0; i < 10 && server.isEnEjecucion(); i++) {
            if (_kbhit()) {
                int c = _getch();

                if (c == '\t') {

                	if (!lastWasTab) {

                        tabPrefix = inputBuffer;
                        tabIndex = -1;

                    }

                    for (int i = 1; i <= N_COMANDOS_DASHBOARD ; i++) {

                        int nextIndex = (tabIndex + i) % N_COMANDOS_DASHBOARD;

                        // Si el comando empieza por lo que habíamos escrito
                        if (cmdsDashboard[nextIndex].find(tabPrefix) == 0) {
                            tabIndex = nextIndex;
                            inputBuffer = cmdsDashboard[tabIndex];
                            break;
                        }

                    }

                    lastWasTab = true; // Marcamos que la última tecla fue un TAB
                    break;

                } else {

                	lastWasTab = false;

                	if (c == '\r' || c == '\n') {

						if (inputBuffer == "EXIT") {

							server.detener();

						} else if (inputBuffer == "TIME_SYNC") {

							offsetTiempoSegundos = 0;

						} else if (inputBuffer.find("TIME_SET ") == 0) {

							string args = inputBuffer.substr(9);
							size_t espacioPos = args.find(' ');
							if (espacioPos != string::npos) {

								string f = args.substr(0, espacioPos);
								string h = args.substr(espacioPos + 1);

								struct tm tTarget = {0};
								int dia = 0, mes = 0, anio = 0, hora = 0, min = 0, seg = 0;
								if (sscanf(f.c_str(), "%d-%d-%d", &dia, &mes, &anio) == 3 && sscanf(h.c_str(), "%d:%d:%d", &hora, &min, &seg) == 3) {

									tTarget.tm_mday = dia;
									tTarget.tm_mon = mes - 1;

                                    if (anio >= 1900) tTarget.tm_year = anio - 1900;
                                    else if (anio < 70) tTarget.tm_year = anio + 100;
                                    else tTarget.tm_year = anio;

									tTarget.tm_hour = hora;
									tTarget.tm_min = min;
									tTarget.tm_sec = seg;
									tTarget.tm_isdst = -1;

									time_t timeTarget = mktime(&tTarget);
									if (timeTarget != -1) {
										offsetTiempoSegundos = timeTarget - time(nullptr);
									}
								} else {
									printf("\a");
								}
							} else {
								printf("\a");
							}

						} else if (inputBuffer == "PROBAR_RESTOCK") {

							time_t tVirtual = time(nullptr) + offsetTiempoSegundos;
							struct tm* now = localtime(&tVirtual);

							int diasFaltantes = (7 - now->tm_wday) % 7;
							if (diasFaltantes == 0 && (now->tm_hour > 23 || (now->tm_hour == 23 && now->tm_min >= 59))) {
								diasFaltantes = 7;
							}

							tVirtual += (diasFaltantes * 24 * 60 * 60);
							struct tm* target = localtime(&tVirtual);
							target->tm_hour = 23;
							target->tm_min = 59;
							target->tm_sec = 50;

							time_t tFinal = mktime(target);
							offsetTiempoSegundos = tFinal - time(nullptr);

						} else if (inputBuffer == "PROBAR_INFORME") {

							time_t tVirtual = time(nullptr) + offsetTiempoSegundos;
							struct tm* now = localtime(&tVirtual);

							now->tm_mday = 1;
							now->tm_mon += 1;
							now->tm_hour = 0;
							now->tm_min = 0;
							now->tm_sec = 0;

							time_t tFinal = mktime(now) - 10; // 10 segundos antes del cambio de mes
							offsetTiempoSegundos = tFinal - time(nullptr);

						} else if (inputBuffer.find("AVANZAR_DIAS") == 0) {

							string diasStr;
							if (inputBuffer.size() > 13) diasStr = inputBuffer.substr(13);
							else diasStr = "1";
							int dias = atoi(diasStr.c_str());

							if (dias > 0) {
								// Sumamos los días directamente al offset existente
								offsetTiempoSegundos += (dias * 24 * 60 * 60);
							} else {
								printf("\a");
							}

						} else if (!inputBuffer.empty()) {
							printf("\a");
						}

						inputBuffer = "";
						break;

					} else if (c == '\b' || c == 8) {

						if (!inputBuffer.empty()) inputBuffer.pop_back();
						break;

					} else if (c >= 32 && c <= 126) {

						inputBuffer += toupper((char)c);
						break;

					}

                }

            }

            Sleep(100);
        }
    }

    limpiarPantalla();
    cout << "\033[?25h\n  " << ESTILO_SUBTITULO << "Apagando el servidor de DeustoCommerce...\n\n" << RESET;

}
