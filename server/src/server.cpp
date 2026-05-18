#include "server.h"
#include "sesion.h"
#include "protocolo.h"
#include <iostream>
#include <thread>
#include <ws2tcpip.h>

extern "C" {
	#include "log.h"
	#include "config.h"
	#include "sqlite3.h"
}

using namespace std;

#define BUFFER_SIZE 8192

Server::Server(int puerto) {

	this->puerto = puerto;
	socketEscucha = INVALID_SOCKET;
	ctx = nullptr;
	enEjecucion = false;

	clientesConectados = 0;
	peticionesTotales = 0;
	tiempoInicio = time(NULL);

}

Server::~Server() {
	detener();
}

// Configuración OpenSSL

void Server::inicializarSSL() {

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

}

void Server::configurarContextoSSL() {

    const SSL_METHOD* method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        LOG_FATAL("No se pudo crear el contexto SSL.");
        exit(EXIT_FAILURE);
    }

    char certificado[256];
    char key[256];

    configGet(CONFIG_PATH, "CRT_PATH", certificado, sizeof(certificado));
    configGet(CONFIG_PATH, "KEY_PATH", key, sizeof(key));

    // Cargamos los certificados (generados previamente)
    if (SSL_CTX_use_certificate_file(ctx, certificado, SSL_FILETYPE_PEM) <= 0 || SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM) <= 0) {
        LOG_FATAL("Error al cargar certificados TLS (%s/%s)", certificado, key);
        exit(EXIT_FAILURE);
    }

}

// Getters para el dashboard

int Server::getPuerto() const { return puerto; }
bool Server::isEnEjecucion() const { return enEjecucion; }
int Server::getClientesConectados() const { return clientesConectados; }
int Server::getPeticionesTotales() const { return peticionesTotales; }
time_t Server::getTiempoActivo() const {
	time_t ahora = time(NULL);
	return ahora - tiempoInicio;
}

// Modificadores de stats

void Server::sumarCliente() { clientesConectados++; }
void Server::restarCliente() { clientesConectados--; }
void Server::sumarPeticion() { peticionesTotales++; }

// Acciones principales

bool Server::iniciar() {

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        LOG_FATAL("Fallo en WSAStartup.");
        return false;
    }

    inicializarSSL();
    configurarContextoSSL();

    socketEscucha = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(puerto);

    if (bind(socketEscucha, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        LOG_FATAL("Error en bind.");
        return false;
    }

    listen(socketEscucha, SOMAXCONN);
    enEjecucion = true;

    tiempoInicio = time(NULL);

    LOG_INFO("Servidor TLS iniciado en puerto %d", puerto);
    return true;

}

void Server::escuchar() {

    while (enEjecucion) {

        sockaddr_in cliente;
        int tamanoCliente = sizeof(cliente);
        SOCKET socketCliente = accept(socketEscucha, (struct sockaddr*)&cliente, &tamanoCliente);

        if (socketCliente != INVALID_SOCKET) {

            thread(Server::handleClient, this, socketCliente, ctx).detach();

        }
    }

}

// Gestión individual de clientes

void Server::handleClient(Server* server, SOCKET socketCliente, SSL_CTX* ctx) {

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, socketCliente);

    if (SSL_accept(ssl) <= 0) {

        LOG_ERROR("Fallo en el handshake TLS");
        ERR_print_errors_fp(stderr);

    } else {

        LOG_INFO("Conexión segura establecida");

        server->sumarCliente();

        char dbPath[256];
        configGet(CONFIG_PATH, "DB_PATH", dbPath, sizeof(dbPath));

        sqlite3* db;
        if (sqlite3_open(dbPath, &db) != SQLITE_OK) {

        	LOG_ERROR("No se pudo abrir la DB para el cliente");

        } else {

			Sesion sesion(ssl);
			char buffer[BUFFER_SIZE];

			while (server->isEnEjecucion()) {

				ZeroMemory(buffer, BUFFER_SIZE);
				int bytes = SSL_read(ssl, buffer, BUFFER_SIZE - 1);

				if (bytes > 0) {

					server->sumarPeticion();
					string msg(buffer, bytes);
					msg.erase(msg.find_last_not_of("\r\n") + 1);
					procesarPeticion(ssl, db, msg, sesion);

				} else {

					break; // Conexión cerrada o error

				}

			}

			sqlite3_close(db);

        }

        server->restarCliente();

    }

    SSL_shutdown(ssl);
    SSL_free(ssl);
    closesocket(socketCliente);
    LOG_INFO("Cliente desconectado");

}

void Server::detener() {

    enEjecucion = false;
    if (socketEscucha != INVALID_SOCKET) closesocket(socketEscucha);
    if (ctx) SSL_CTX_free(ctx);
    WSACleanup();

}
