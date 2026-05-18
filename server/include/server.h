#ifndef INCLUDE_SERVER_H_
#define INCLUDE_SERVER_H_

#include "openssl/ssl.h"
#include "openssl/err.h"
#include <time.h>
#include <winsock2.h>
#include <string>

#define CONFIG_PATH "../data/config/server_config.ini"
#define ENV_PATH "env/.env"

class Server {
private:

	SOCKET socketEscucha;
	SSL_CTX* ctx;
	int puerto;
	bool enEjecucion;

	// Estadísticas del dashboard

	int clientesConectados;
	int peticionesTotales;
	time_t tiempoInicio;

	// Configuración OpenSSL

	void inicializarSSL();
	void configurarContextoSSL();

public:

	Server(int puerto);
	virtual ~Server();

	// Getters para el dashboard
	int getPuerto() const;
	bool isEnEjecucion() const;
	int getClientesConectados() const;
	int getPeticionesTotales() const;
	time_t getTiempoActivo() const;

	// Modificadores de stats

	void sumarCliente();
	void restarCliente();
	void sumarPeticion();

	// Acciones principales

	bool iniciar();
	void escuchar();
	void detener();

	// Gestión individual

	static void handleClient(Server* server, SOCKET socketCliente, SSL_CTX* ctx);

};

#endif /* INCLUDE_SERVER_H_ */
