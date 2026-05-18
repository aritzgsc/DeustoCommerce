#include <iostream>
#include <string>
#include <cstdlib>
#include <winsock2.h>
#include <windows.h>
#include <thread>
#include "server.h"
#include "daemon.h"
#include "server_ui.h"

extern "C" {
	#include "sodium.h"
	#include "config.h"
	#include "log.h"
}

using namespace std;

Server* serverGlobal = nullptr;
Daemon* daemonGlobal = nullptr;

time_t offsetTiempoSegundos = 0;

BOOL WINAPI manejarSenal(DWORD senal) {
	if (senal == CTRL_C_EVENT || senal == CTRL_CLOSE_EVENT) {
		if (daemonGlobal != nullptr) {
			daemonGlobal->detener();
		}
		if (serverGlobal != nullptr) {
			serverGlobal->detener();
		}
		Sleep(500);
		return TRUE;
	}
	return FALSE;
}

int main() {

    char logPath[256];
    char puertoStr[6];
    int puerto;

    configGet(CONFIG_PATH, "LOG_SERVER_PATH", logPath, sizeof(logPath));
    configGet(CONFIG_PATH, "PORT", puertoStr, sizeof(puertoStr));
    puerto = atoi(puertoStr);

    logInit(logPath, LOG_DEBUG);

    if (sodium_init() < 0) {
        LOG_FATAL("No se pudo inicializar libsodium");
        return 1;
    }

    Server server(puerto);
    serverGlobal = &server;

    Daemon daemon;
    daemonGlobal = &daemon;

    SetConsoleCtrlHandler(manejarSenal, TRUE);

    if (server.iniciar()) {

			if (daemon.iniciar()) {

			thread hiloServidor(&Server::escuchar, &server);

			ejecutarDashboardRealTime(server);

			hiloServidor.join();

			LOG_INFO("Servidor detenido correctamente.");

    	} else {

    		LOG_FATAL("El demonio de tareas periódicas no pudo iniciar.");
    		server.detener();

    	}

    } else {

        LOG_FATAL("El servidor no pudo iniciar");

    }

    logClose();
    return 0;

}
