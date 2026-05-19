#ifndef INCLUDE_CLIENT_H_
#define INCLUDE_CLIENT_H_

#include "openssl/ssl.h"
#include "openssl/err.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>

#define CONFIG_PATH "../client/config/client_config.ini"

// Auxiliar

// Utilidad para separar los argumentos en un array clásico.
// Devuelve el número de tokens encontrados.

int split(const std::string& s, char delimiter, std::string args[], int maxArgs);

class Client {
private:

    SOCKET sock;
    SSL_CTX* ctx;
    SSL* ssl;
    std::string ip;
    int puerto;

	bool autenticado;
	std::string correo;
	std::string nombre;
	std::string apellido;

public:

    Client(std::string ip, int puerto);
    ~Client();

    // Gestión de conexión

    bool conectar();
    void desconectar();

    // Enviar mensaje al servidor

    bool enviar(std::string mensaje);

    // Recibir respuesta del servidor

    std::string recibir();

    // Getters

	bool isAutenticado() const;
	std::string getCorreo() const;
	std::string getNombre() const;
	std::string getApellido() const;

	// Setters lógicos

	void login(const std::string& correo, const std::string& nombre, const std::string& apellido);
	void logout();

};

#endif /* INCLUDE_CLIENT_H_ */
