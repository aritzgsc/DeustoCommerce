#include <client_ui.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <winsock2.h>
#include <windows.h>
#include "client.h"

extern "C" {
	#include <utils_ui.h>
	#include "log.h"
	#include "config.h"
}

using namespace std;

Client* clienteGlobal = nullptr;

BOOL WINAPI manejarSenalCliente(DWORD senal) {
	if (senal == CTRL_C_EVENT || senal == CTRL_CLOSE_EVENT) {

        if (clienteGlobal != nullptr) {
            LOG_INFO("Cierre forzado detectado. Avisando al servidor.");
			clienteGlobal->enviar("EXIT");
		}

		Sleep(500);
		clienteGlobal->desconectar();
		logClose();
		exit(0);
		return TRUE;
	}

	return FALSE;

}

int main() {

	char logPath[256];
	char serverIp[16];
	char puertoStr[6];
	int puerto;

	configGet(CONFIG_PATH, "LOG_CLIENT_PATH", logPath, sizeof(logPath));
	configGet(CONFIG_PATH, "SERVER_IP", serverIp, sizeof(serverIp));
	configGet(CONFIG_PATH, "PORT", puertoStr, sizeof(puertoStr));
	puerto = atoi(puertoStr);

	logInit(logPath, LOG_DEBUG);

	Client cliente(serverIp, puerto);
	clienteGlobal = &cliente;

	SetConsoleCtrlHandler(manejarSenalCliente, TRUE);

	if (!cliente.conectar()) {
		LOG_ERROR("No se pudo conectar con el servidor de DeustoCommerce");
		limpiarPantalla();
		cout << "\n  " << ESTILO_SUBTITULO << "Servidor desconectado o fuera de alcance...\n\n" << RESET;
		return 1;
	}

	bucleCliente(cliente);

	cliente.desconectar();
	logClose();

	return 0;

}
